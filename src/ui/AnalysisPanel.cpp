#include "AnalysisPanel.h"
#include "Theme.h"
#include "../utils/TsanAnnotations.h"
#include "../parser/AssemblyParser.h"
#include "../utils/AssemblyDiffProcessor.h"

#include <QFutureWatcher>
#include <QHeaderView>
#include <QPromise>
#include <QThreadPool>
#include <QVBoxLayout>
#include <algorithm>

Q_DECLARE_METATYPE(std::shared_ptr<AssemblyModule>)

namespace {

class ParseTask : public QRunnable {
public:
    ParseTask(QPromise<std::shared_ptr<AssemblyModule>>&& p, QString text) : m_promise(std::move(p)), m_text(std::move(text)) {
        setAutoDelete(true);
    }
    void run() override {
        OCTOS_TSAN_ACQUIRE(this);
        m_promise.start();
        auto module = std::make_shared<AssemblyModule>(AssemblyParser::parse(m_text));
        if (!m_promise.isCanceled()) m_promise.addResult(module);
        m_promise.finish();
    }
private:
    QPromise<std::shared_ptr<AssemblyModule>> m_promise;
    QString m_text;
};

QString successorText(const Function& fn, const BasicBlock& bb) {
    QStringList parts;
    for (int ei : bb.successors) {
        const BlockEdge& e = fn.edges[ei];
        QString target;
        if (e.kind == EdgeKind::IndirectUnknown) target = "? (computed jump, targets unknown)";
        else if (e.to >= 0) target = fn.basicBlocks[e.to].name;
        else target = e.targetName + (e.leavesFunction ? " (leaves function)" : " (unresolved)");
        parts << QString("%1 → %2").arg(edgeKindToString(e.kind), target);
    }
    return parts.isEmpty() ? QString("— (no successor)") : parts.join("; ");
}

}

AnalysisPanel::AnalysisPanel(QWidget* parent) : QWidget(parent) {
    qRegisterMetaType<std::shared_ptr<AssemblyModule>>();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);

    auto* top = new QHBoxLayout();
    top->addWidget(new QLabel("Pane:"));
    m_paneCombo = new QComboBox();
    m_paneCombo->setMinimumWidth(240);
    connect(m_paneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AnalysisPanel::onPaneSelected);
    top->addWidget(m_paneCombo);
    m_summary = new QLabel();
    Theme::setRole(m_summary, "muted");
    top->addWidget(m_summary, 1);
    layout->addLayout(top);

    m_status = new QLabel();
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    auto* tabs = new QTabWidget();
    auto* splitter = new QSplitter(Qt::Horizontal);

    m_functions = new QTreeWidget();
    m_functions->setHeaderLabels({"Function", "Blocks", "Instr.", "Calls", "Branches", "Asm lines"});
    m_functions->setRootIsDecorated(false);
    m_functions->setAlternatingRowColors(true);
    m_functions->setUniformRowHeights(true);
    m_functions->setSortingEnabled(true);
    m_functions->header()->setStretchLastSection(false);
    m_functions->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    for (int c = 1; c <= 5; ++c) m_functions->header()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_functions->setColumnWidth(0, 190);
    m_functions->setMinimumWidth(380);
    m_functions->setToolTip("Double-click to jump to the function in the assembly output");
    connect(m_functions, &QTreeWidget::itemSelectionChanged, this, &AnalysisPanel::onFunctionSelected);
    connect(m_functions, &QTreeWidget::itemDoubleClicked, this, &AnalysisPanel::onFunctionActivated);

    m_blocks = new QTreeWidget();
    m_blocks->setHeaderLabels({"Basic block", "Instr.", "Asm lines", "Source lines", "Successors"});
    m_blocks->setRootIsDecorated(false);
    m_blocks->setAlternatingRowColors(true);
    m_blocks->setUniformRowHeights(true);
    m_blocks->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_blocks->header()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_blocks->setToolTip("Double-click: assembly line (column 1-3) or source line (column 4)");
    connect(m_blocks, &QTreeWidget::itemDoubleClicked, this, &AnalysisPanel::onBlockActivated);

    splitter->addWidget(m_functions);
    splitter->addWidget(m_blocks);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({440, 660});
    tabs->addTab(splitter, "Functions && Blocks");

    m_frequencies = new QTableWidget(0, 2);
    m_frequencies->setHorizontalHeaderLabels({"Mnemonic", "Count"});
    m_frequencies->horizontalHeader()->setStretchLastSection(true);
    m_frequencies->verticalHeader()->setVisible(false);
    m_frequencies->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_frequencies->setSortingEnabled(true);
    auto* statsWidget = new QWidget();
    auto* statsLayout = new QHBoxLayout(statsWidget);
    m_metrics = new QLabel();
    m_metrics->setTextFormat(Qt::RichText);
    m_metrics->setWordWrap(true);
    m_metrics->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_metrics->setMinimumWidth(280);
    statsLayout->addWidget(m_metrics, 1);
    statsLayout->addWidget(m_frequencies, 1);
    tabs->addTab(statsWidget, "Statistics");

    layout->addWidget(tabs, 1);

    m_watcher = new QFutureWatcher<std::shared_ptr<AssemblyModule>>(this);
    connect(m_watcher, &QFutureWatcherBase::finished, this, [this]() {
        // Only the newest request may update the view.
        if (m_activeJob == 0) return;
        const quint64 job = m_watcher->property("job").toULongLong();
        if (job != m_activeJob) return;
        m_activeJob = 0;
        if (m_watcher->future().resultCount() > 0) showModule(m_watcher->future().result());
    });
}

AnalysisPanel::~AnalysisPanel() {
    cancelJob();
    QThreadPool::globalInstance()->waitForDone(3000);
}

void AnalysisPanel::cancelJob() {
    m_activeJob = 0;
    if (m_watcher && m_watcher->isRunning()) m_watcher->cancel();
}

void AnalysisPanel::clear() {
    cancelJob();
    m_entries.clear();
    m_paneCombo->blockSignals(true);
    m_paneCombo->clear();
    m_paneCombo->blockSignals(false);
    m_module.reset();
    m_functions->clear();
    m_blocks->clear();
    m_frequencies->setRowCount(0);
    m_summary->clear();
    m_status->clear();
    m_metrics->clear();
}

void AnalysisPanel::removePane(quint64 paneId) {
    if (!m_entries.contains(paneId)) return;
    m_entries.remove(paneId);
    const int idx = m_paneCombo->findData(QVariant::fromValue<quint64>(paneId));
    if (idx >= 0) m_paneCombo->removeItem(idx);   // triggers onPaneSelected for the new current item
    if (m_entries.isEmpty()) clear();
}

void AnalysisPanel::setPaneAssembly(quint64 paneId, const QString& title, const QString& rawAssembly) {
    if (rawAssembly.isEmpty()) {
        removePane(paneId);
        return;
    }
    const bool isNew = !m_entries.contains(paneId);
    m_entries[paneId] = {title, rawAssembly};
    if (isNew) {
        m_paneCombo->blockSignals(true);
        m_paneCombo->addItem(title, QVariant::fromValue<quint64>(paneId));
        m_paneCombo->blockSignals(false);
        if (m_paneCombo->count() == 1) m_paneCombo->setCurrentIndex(0);
    } else {
        const int idx = m_paneCombo->findData(QVariant::fromValue<quint64>(paneId));
        if (idx >= 0) m_paneCombo->setItemText(idx, title);
    }
    if (m_currentPane == paneId || m_paneCombo->count() == 1 || m_currentPane == 0) {
        m_currentPane = paneId;
        const int idx = m_paneCombo->findData(QVariant::fromValue<quint64>(paneId));
        m_paneCombo->blockSignals(true);
        m_paneCombo->setCurrentIndex(idx);
        m_paneCombo->blockSignals(false);
        startAnalysis();
    }
}

void AnalysisPanel::onPaneSelected(int index) {
    if (index < 0) return;
    m_currentPane = m_paneCombo->itemData(index).value<quint64>();
    startAnalysis();
}

void AnalysisPanel::startAnalysis() {
    const auto it = m_entries.constFind(m_currentPane);
    if (it == m_entries.constEnd()) return;

    cancelJob();
    m_status->setText("Analyzing…");
    m_status->setProperty("role", "muted");

    QPromise<std::shared_ptr<AssemblyModule>> promise;
    QFuture<std::shared_ptr<AssemblyModule>> future = promise.future();
    const quint64 job = m_nextJob++;
    m_activeJob = job;
    m_watcher->setProperty("job", QVariant::fromValue<quint64>(job));
    m_watcher->setFuture(future);
    auto* task = new ParseTask(std::move(promise), it->text);
    OCTOS_TSAN_RELEASE(task);
    QThreadPool::globalInstance()->start(task);
}

void AnalysisPanel::showModule(std::shared_ptr<AssemblyModule> module) {
    m_module = module;
    m_functions->setSortingEnabled(false);
    m_functions->clear();
    m_blocks->clear();

    for (int i = 0; i < module->functions.size(); ++i) {
        const Function& fn = module->functions[i];
        int calls = 0, branches = 0;
        for (const BasicBlock& bb : fn.basicBlocks)
            for (const Instruction& in : bb.instructions) {
                if (in.kind == InstructionKind::Call || in.kind == InstructionKind::IndirectCall) ++calls;
                else if (in.kind == InstructionKind::ConditionalBranch || in.kind == InstructionKind::UnconditionalBranch ||
                         in.kind == InstructionKind::IndirectBranch) ++branches;
            }
        auto* item = new QTreeWidgetItem(m_functions);
        item->setText(0, fn.name);
        item->setText(1, QString::number(fn.basicBlocks.size()));
        item->setText(2, QString::number(fn.totalInstructions()));
        item->setText(3, QString::number(calls));
        item->setText(4, QString::number(branches));
        item->setText(5, QString("%1–%2").arg(fn.startLineNumber).arg(fn.endLineNumber));
        item->setData(0, Qt::UserRole, i);
        for (int c = 1; c <= 4; ++c) {
            item->setTextAlignment(c, Qt::AlignRight | Qt::AlignVCenter);
            item->setData(c, Qt::DisplayRole, item->text(c).toInt());   // numeric sorting
        }
        item->setToolTip(0, fn.declaredAsFunction ? "Declared with .type @function" : "Function inferred from labels (no .type information)");
    }
    m_functions->setSortingEnabled(true);
    m_functions->sortByColumn(5, Qt::AscendingOrder);
    if (m_functions->topLevelItemCount() > 0) m_functions->setCurrentItem(m_functions->topLevelItem(0));

    const AssemblyMetrics mt = computeAssemblyMetrics(*module);
    m_summary->setText(QString("%1 · %2 functions · %3 blocks · %4 instructions").arg(architectureToString(module->architecture)).arg(mt.functionCount).arg(mt.basicBlockCount).arg(mt.instructionCount));

    QStringList notes;
    if (module->truncated) notes << "The listing exceeded the parser limits: the analysis is incomplete.";
    if (!module->warnings.isEmpty()) notes << QString("%1 parser note(s): %2").arg(module->warnings.size()).arg(module->warnings.first());
    if (module->functions.isEmpty())
        notes << "No functions found. The analysis understands x86-64 and AArch64 GNU assembler output.";
    m_status->setText(notes.join(" "));
    Theme::setRole(m_status, notes.isEmpty() ? "muted" : "warning");

    populateMetrics();
    onFunctionSelected();
}

void AnalysisPanel::populateMetrics() {
    if (!m_module) return;
    const AssemblyMetrics m = computeAssemblyMetrics(*m_module);
    QString html = "<table cellspacing='4'>";
    auto row = [&html](const QString& k, const QString& v) { html += QString("<tr><td><b>%1</b></td><td align='right'>%2</td></tr>").arg(k, v); };
    row("Functions", QString::number(m.functionCount));
    row("Basic blocks", QString::number(m.basicBlockCount));
    row("CFG edges", QString::number(m.edgeCount));
    row("Instructions", QString::number(m.instructionCount));
    row("Conditional jumps", QString::number(m.conditionalJumps));
    row("Unconditional jumps", QString::number(m.unconditionalJumps));
    row("Indirect jumps", QString::number(m.indirectJumps));
    row("Calls", QString::number(m.calls));
    row("Returns", QString::number(m.returns));
    row("Tail calls (jmp out)", QString::number(m.tailCalls));
    row("Code size", m.codeSizeKnown ? QString("%1 B").arg(m.codeSizeBytes) : QString("unknown (not derivable from assembly text)"));
    html += "</table><p><i>Counts describe the listing, not runtime behaviour: fewer instructions do not imply faster code.</i></p>";
    m_metrics->setText(html);

    m_frequencies->setSortingEnabled(false);
    m_frequencies->setRowCount(0);
    QVector<QPair<QString, int>> freq;
    for (auto it = m.instructionFrequencies.constBegin(); it != m.instructionFrequencies.constEnd(); ++it) freq.append({it.key(), it.value()});
    std::sort(freq.begin(), freq.end(), [](const auto& a, const auto& b) { return a.second != b.second ? a.second > b.second : a.first < b.first; });
    m_frequencies->setRowCount(freq.size());
    for (int i = 0; i < freq.size(); ++i) {
        m_frequencies->setItem(i, 0, new QTableWidgetItem(freq[i].first));
        auto* cnt = new QTableWidgetItem();
        cnt->setData(Qt::DisplayRole, freq[i].second);
        m_frequencies->setItem(i, 1, cnt);
    }
    m_frequencies->setSortingEnabled(true);
}

void AnalysisPanel::onFunctionSelected() {
    m_blocks->clear();
    if (!m_module) return;
    auto sel = m_functions->selectedItems();
    if (sel.isEmpty()) return;
    const int fi = sel.first()->data(0, Qt::UserRole).toInt();
    if (fi < 0 || fi >= m_module->functions.size()) return;
    const Function& fn = m_module->functions[fi];
    for (const BasicBlock& bb : fn.basicBlocks) {
        int srcMin = 0, srcMax = 0;
        for (const Instruction& in : bb.instructions) {
            if (!in.sourceLoc.isValid()) continue;
            srcMin = srcMin == 0 ? in.sourceLoc.line : qMin(srcMin, in.sourceLoc.line);
            srcMax = qMax(srcMax, in.sourceLoc.line);
        }
        auto* item = new QTreeWidgetItem(m_blocks);
        item->setText(0, QString("bb%1  %2").arg(bb.id).arg(bb.name));
        item->setText(1, QString::number(bb.instructions.size()));
        item->setText(2, QString("%1–%2").arg(bb.startLineNumber).arg(bb.endLineNumber));
        item->setText(3, srcMin == 0 ? QString("unknown") : (srcMin == srcMax ? QString::number(srcMin) : QString("%1–%2").arg(srcMin).arg(srcMax)));
        item->setText(4, successorText(fn, bb));
        item->setData(0, Qt::UserRole, bb.startLineNumber);
        item->setData(3, Qt::UserRole, srcMin);
        item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    }
}

void AnalysisPanel::onFunctionActivated(QTreeWidgetItem* item, int) {
    if (!m_module || !item) return;
    const int fi = item->data(0, Qt::UserRole).toInt();
    if (fi >= 0 && fi < m_module->functions.size())
        emit assemblyLineActivated(m_currentPane, m_module->functions[fi].startLineNumber);
}

void AnalysisPanel::onBlockActivated(QTreeWidgetItem* item, int column) {
    if (!item) return;
    if (column == 3) {
        const int line = item->data(3, Qt::UserRole).toInt();
        if (line > 0) emit sourceLineActivated(line);   // unknown mappings are not guessed
        return;
    }
    emit assemblyLineActivated(m_currentPane, item->data(0, Qt::UserRole).toInt());
}

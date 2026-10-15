#include "DiffDialog.h"
#include "Theme.h"
#include "../utils/TsanAnnotations.h"

#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPromise>
#include <QRunnable>
#include <QSaveFile>
#include <QShortcut>
#include <QVBoxLayout>

namespace {

// Runs one diff on a pool thread and reports through the promise.
class DiffRunnable : public QRunnable {
public:
    DiffRunnable(QPromise<DiffResult>&& promise, QString a, QString b, DiffOptions opt)
        : m_promise(std::move(promise)), m_a(std::move(a)), m_b(std::move(b)), m_opt(opt) {
        setAutoDelete(true);
    }
    void run() override {
        OCTOS_TSAN_ACQUIRE(this);   // pairs with the release in DiffDialog::onRecomputeDiff()
        m_promise.start();
        auto cancelled = [this]() { return m_promise.isCanceled(); };
        DiffResult r;
        try {
            r = AssemblyDiffProcessor::computeDiff(m_a, m_b, m_opt, cancelled);
        } catch (const std::exception& e) {
            r.isTruncated = true;
            r.statusMessage = QString("Internal error while computing the diff: %1").arg(e.what());
        }
        m_promise.addResult(std::move(r));
        m_promise.finish();
    }
private:
    QPromise<DiffResult> m_promise;
    QString m_a, m_b;
    DiffOptions m_opt;
};

QString paneLabel(const DiffPaneInfo& p, int i) {
    return p.title.isEmpty() ? QString("Pane %1").arg(i + 1) : p.title;
}

} // namespace

DiffDialog::DiffDialog(const QVector<DiffPaneInfo>& panes,
                       int initialIndexA, int initialIndexB,
                       QWidget* parent)
    : QDialog(parent),
      m_panes(panes) {
    setWindowTitle("Assembly & Output Diff Viewer");
    resize(1100, 750);
    m_pool.setMaxThreadCount(1);
    setupUi();

    if (m_comboA && m_comboB) {
        // Block the signals: the selection must trigger exactly one computation.
        QSignalBlocker ba(m_comboA), bb(m_comboB);
        if (initialIndexA >= 0 && initialIndexA < m_comboA->count()) m_comboA->setCurrentIndex(initialIndexA);
        if (initialIndexB >= 0 && initialIndexB < m_comboB->count()) m_comboB->setCurrentIndex(initialIndexB);
    }
    updatePaneInfoLabels();
    onRecomputeDiff();
}

DiffDialog::DiffDialog(const QString& nameA, const QString& asmA,
                       const QString& nameB, const QString& asmB,
                       QWidget* parent)
    : QDialog(parent) {
    DiffPaneInfo pA;
    pA.title = nameA; pA.compilerName = nameA; pA.content = asmA;
    DiffPaneInfo pB;
    pB.title = nameB; pB.compilerName = nameB; pB.content = asmB;
    m_panes.append(pA);
    m_panes.append(pB);

    setWindowTitle("Assembly & Output Diff Viewer");
    resize(1100, 750);
    m_pool.setMaxThreadCount(1);
    setupUi();
    updatePaneInfoLabels();
    onRecomputeDiff();
}

DiffDialog::~DiffDialog() {
    cancelAllJobs();
    m_pool.waitForDone();  // cancellation is polled inside the algorithm, so this returns promptly
}

void DiffDialog::done(int r) {
    // Closing the dialog while a computation runs: stop it, never leave a worker behind a dead dialog.
    m_closing = true;
    cancelAllJobs();
    QDialog::done(r);
}

void DiffDialog::cancelAllJobs() {
    for (Job& j : m_jobs) {
        if (j.watcher) {
            j.watcher->disconnect(this);   // late results must never reach the UI
            j.watcher->cancel();
        }
    }
    // Watchers are children of the dialog; they finish and are deleted with it or below.
    for (Job& j : m_jobs) {
        if (j.watcher) j.watcher->deleteLater();
    }
    m_jobs.clear();
    m_activeJob = 0;
}

void DiffDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    auto* headerLayout = new QHBoxLayout();
    m_comboA = new QComboBox(this);
    m_comboB = new QComboBox(this);
    for (int i = 0; i < m_panes.size(); ++i) {
        m_comboA->addItem(paneLabel(m_panes[i], i));
        m_comboB->addItem(paneLabel(m_panes[i], i));
    }
    if (m_comboB->count() > 1) m_comboB->setCurrentIndex(1);
    connect(m_comboA, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DiffDialog::onPaneSelectionChanged);
    connect(m_comboB, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DiffDialog::onPaneSelectionChanged);

    headerLayout->addWidget(new QLabel("<b>Pane A:</b>", this));
    headerLayout->addWidget(m_comboA, 1);
    headerLayout->addWidget(new QLabel("<b>vs</b>", this));
    headerLayout->addWidget(new QLabel("<b>Pane B:</b>", this));
    headerLayout->addWidget(m_comboB, 1);
    mainLayout->addLayout(headerLayout);

    auto* infoLayout = new QHBoxLayout();
    m_infoLabelA = new QLabel(this);
    m_infoLabelB = new QLabel(this);
    infoLayout->addWidget(m_infoLabelA, 1);
    infoLayout->addWidget(m_infoLabelB, 1);
    mainLayout->addLayout(infoLayout);

    m_typeWarningLabel = new QLabel(this);
    m_typeWarningLabel->setWordWrap(true);
    m_typeWarningLabel->setVisible(false);
    Theme::setRole(m_typeWarningLabel, "noticeWarning");
    mainLayout->addWidget(m_typeWarningLabel);

    auto* optionsLayout = new QHBoxLayout();
    m_hideUnchangedCheck = new QCheckBox("Show Only Differences", this);
    connect(m_hideUnchangedCheck, &QCheckBox::toggled, this, [this](bool on) { m_proxy->setOnlyDifferences(on); });
    m_ignoreWhitespaceCheck = new QCheckBox("Ignore Whitespace", this);
    connect(m_ignoreWhitespaceCheck, &QCheckBox::toggled, this, &DiffDialog::onRecomputeDiff);
    m_ignoreCommentsCheck = new QCheckBox("Ignore Comments", this);
    connect(m_ignoreCommentsCheck, &QCheckBox::toggled, this, &DiffDialog::onRecomputeDiff);
    m_normalizeLabelsCheck = new QCheckBox("Normalize Local Labels", this);
    m_normalizeLabelsCheck->setToolTip("Renames compiler-generated labels by order of first appearance, so jump targets stay distinguishable.");
    connect(m_normalizeLabelsCheck, &QCheckBox::toggled, this, &DiffDialog::onRecomputeDiff);
    optionsLayout->addWidget(m_hideUnchangedCheck);
    optionsLayout->addWidget(m_ignoreWhitespaceCheck);
    optionsLayout->addWidget(m_ignoreCommentsCheck);
    optionsLayout->addWidget(m_normalizeLabelsCheck);
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);

    auto* navLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Search in diff...");
    m_searchInput->setMaximumWidth(220);
    connect(m_searchInput, &QLineEdit::textChanged, this, &DiffDialog::onFindText);
    m_searchStatusLabel = new QLabel(this);
    m_prevSearchBtn = new QPushButton("▲", this);
    m_prevSearchBtn->setToolTip("Find Previous");
    m_prevSearchBtn->setFixedWidth(28);
    connect(m_prevSearchBtn, &QPushButton::clicked, this, &DiffDialog::onFindPrev);
    m_nextSearchBtn = new QPushButton("▼", this);
    m_nextSearchBtn->setToolTip("Find Next");
    m_nextSearchBtn->setFixedWidth(28);
    connect(m_nextSearchBtn, &QPushButton::clicked, this, &DiffDialog::onFindNext);
    navLayout->addWidget(new QLabel("Find:", this));
    navLayout->addWidget(m_searchInput);
    navLayout->addWidget(m_prevSearchBtn);
    navLayout->addWidget(m_nextSearchBtn);
    navLayout->addWidget(m_searchStatusLabel);
    navLayout->addSpacing(20);

    m_prevDiffBtn = new QPushButton("← Prev Change", this);
    connect(m_prevDiffBtn, &QPushButton::clicked, this, &DiffDialog::onPrevDiff);
    m_nextDiffBtn = new QPushButton("Next Change →", this);
    connect(m_nextDiffBtn, &QPushButton::clicked, this, &DiffDialog::onNextDiff);
    navLayout->addWidget(m_prevDiffBtn);
    navLayout->addWidget(m_nextDiffBtn);
    navLayout->addStretch();

    m_copyBtn = new QPushButton("Copy Patch", this);
    connect(m_copyBtn, &QPushButton::clicked, this, &DiffDialog::onCopyUnifiedDiff);
    m_exportBtn = new QPushButton("Export Patch...", this);
    connect(m_exportBtn, &QPushButton::clicked, this, &DiffDialog::onExportUnifiedDiff);
    navLayout->addWidget(m_copyBtn);
    navLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(navLayout);

    // Progress / status row
    auto* progLayout = new QHBoxLayout();
    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 0);  // indeterminate
    m_progress->setMaximumHeight(12);
    m_progress->setTextVisible(false);
    m_progress->setVisible(false);
    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setVisible(false);
    connect(m_cancelBtn, &QPushButton::clicked, this, &DiffDialog::onCancelCompute);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    progLayout->addWidget(m_progress, 1);
    progLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(progLayout);
    mainLayout->addWidget(m_statusLabel);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(m_summaryLabel);

    m_model = new DiffTableModel(this);
    m_proxy = new DiffFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_table = new QTableView(this);
    m_table->setModel(m_proxy);
    m_table->horizontalHeader()->setSectionResizeMode(DiffTableModel::LineA, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(DiffTableModel::TextA, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(DiffTableModel::LineB, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(DiffTableModel::TextB, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(m_table->fontMetrics().height() + 6);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    mainLayout->addWidget(m_table, 1);

    auto* copySc = new QShortcut(QKeySequence::Copy, m_table);
    connect(copySc, &QShortcut::activated, this, &DiffDialog::onCopySelection);

    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);

    updateControlsEnabled();
}

void DiffDialog::updateControlsEnabled() {
    const DiffResult* r = currentResult();
    const bool busy = isComputing();
    const bool haveEntries = r && !r->entries.isEmpty();
    m_prevDiffBtn->setEnabled(!busy && haveEntries);
    m_nextDiffBtn->setEnabled(!busy && haveEntries);
    m_copyBtn->setEnabled(!busy && r);
    m_exportBtn->setEnabled(!busy && r);
    m_progress->setVisible(busy);
    m_cancelBtn->setVisible(busy);
}

void DiffDialog::updatePaneInfoLabels() {
    const int idxA = m_comboA ? m_comboA->currentIndex() : -1;
    const int idxB = m_comboB ? m_comboB->currentIndex() : -1;
    auto describe = [](const DiffPaneInfo& p) {
        return QString("Compiler: %1 | Flags: %2 | Type: %3")
            .arg(p.compilerName.isEmpty() ? "N/A" : p.compilerName,
                 p.compilerFlags.isEmpty() ? "(none)" : p.compilerFlags,
                 artifactTypeToString(p.artifactType));
    };
    m_infoLabelA->setText(idxA >= 0 && idxA < m_panes.size() ? describe(m_panes[idxA]) : QString());
    m_infoLabelB->setText(idxB >= 0 && idxB < m_panes.size() ? describe(m_panes[idxB]) : QString());

    if (idxA >= 0 && idxA < m_panes.size() && idxB >= 0 && idxB < m_panes.size() &&
        m_panes[idxA].artifactType != m_panes[idxB].artifactType) {
        m_typeWarningLabel->setText(
            QString("⚠ Comparing different artifact formats (%1 vs %2). Line-by-line comparison may not be semantically equivalent.")
                .arg(artifactTypeToString(m_panes[idxA].artifactType),
                     artifactTypeToString(m_panes[idxB].artifactType)));
        m_typeWarningLabel->setVisible(true);
    } else {
        m_typeWarningLabel->setVisible(false);
    }
}

AsmDialect DiffDialog::dialectFor(const DiffPaneInfo& pa, const DiffPaneInfo& pb) const {
    auto one = [](const DiffPaneInfo& p) {
        switch (p.artifactType) {
            case ArtifactType::JVM_Bytecode: return AsmDialect::JvmBytecode;
            case ArtifactType::CPython_Bytecode: return AsmDialect::CPythonBytecode;
            case ArtifactType::Assembly: {
                const QString arch = p.architecture.toLower();
                if (arch.contains("aarch64") || arch.contains("arm64")) return AsmDialect::ARM64;
                if (arch.contains("x86") || arch.contains("amd64")) {
                    return p.syntaxFlavor == SyntaxFlavor::Intel ? AsmDialect::X86Intel : AsmDialect::X86ATT;
                }
                return AssemblyDiffProcessor::detectNativeDialect(p.content, p.syntaxFlavor == SyntaxFlavor::Intel);
            }
            default: return AsmDialect::Unknown;
        }
    };
    const AsmDialect a = one(pa), b = one(pb);
    // Only trust the dialect if both sides agree up to the x86 syntax variant.
    auto isX86 = [](AsmDialect d) { return d == AsmDialect::X86ATT || d == AsmDialect::X86Intel; };
    if (a == b) return a;
    if (isX86(a) && isX86(b)) return AsmDialect::X86ATT;  // both use '#' comments
    return AsmDialect::Unknown;
}

void DiffDialog::onPaneSelectionChanged() {
    updatePaneInfoLabels();
    onRecomputeDiff();
}

void DiffDialog::onRecomputeDiff() {
    if (m_closing) return;
    const int idxA = m_comboA ? m_comboA->currentIndex() : -1;
    const int idxB = m_comboB ? m_comboB->currentIndex() : -1;
    if (idxA < 0 || idxA >= m_panes.size() || idxB < 0 || idxB >= m_panes.size()) return;

    // Invalidate everything that is still running: older results must never overwrite newer ones.
    cancelAllJobs();

    const DiffPaneInfo& pa = m_panes[idxA];
    const DiffPaneInfo& pb = m_panes[idxB];

    DiffOptions options;
    options.ignoreWhitespace = m_ignoreWhitespaceCheck->isChecked();
    options.ignoreComments = m_ignoreCommentsCheck->isChecked();
    options.normalizeLabels = m_normalizeLabelsCheck->isChecked();
    options.dialect = dialectFor(pa, pb);

    QPromise<DiffResult> promise;
    QFuture<DiffResult> future = promise.future();
    auto* watcher = new QFutureWatcher<DiffResult>(this);
    const quint64 jobId = m_nextJobId++;
    connect(watcher, &QFutureWatcherBase::finished, this, [this, jobId]() { onJobFinished(jobId); });
    watcher->setFuture(future);
    m_jobs.append({jobId, watcher});
    m_activeJob = jobId;

    m_statusLabel->setText("Computing diff…");
    m_summaryLabel->clear();
    updateControlsEnabled();

    auto* runnable = new DiffRunnable(std::move(promise), pa.content, pb.content, options);
    OCTOS_TSAN_RELEASE(runnable);
    m_pool.start(runnable);
}

void DiffDialog::onCancelCompute() {
    cancelAllJobs();
    m_statusLabel->setText("Diff computation cancelled.");
    updateControlsEnabled();
}

void DiffDialog::onJobFinished(quint64 jobId) {
    // Defense in depth on top of disconnect(): only the newest request may update the UI.
    if (m_closing || jobId != m_activeJob) return;
    QFutureWatcher<DiffResult>* w = nullptr;
    for (const Job& j : m_jobs) if (j.id == jobId) w = j.watcher;
    if (!w) return;

    DiffResult result;
    if (w->future().resultCount() > 0) {
        result = w->future().result();
    } else {
        result.isCancelled = true;
        result.statusMessage = "Diff computation was cancelled";
    }
    m_activeJob = 0;
    w->deleteLater();
    m_jobs.clear();
    showResult(std::move(result));
}

void DiffDialog::setSummary(const DiffResult& r) {
    const ThemeTokens& tk = Theme::currentTokens();
    QString summary = QString("Additions: <span style='color: %5;'>+%1</span> | "
                              "Deletions: <span style='color: %6;'>-%2</span> | "
                              "Unchanged: %3 | Time: %4 ms")
                          .arg(r.additions).arg(r.deletions).arg(r.unchanged).arg(r.durationMs)
                          .arg(tk.success.name(), tk.error.name());
    m_summaryLabel->setText(summary);
}

void DiffDialog::showResult(DiffResult result) {
    auto shared = std::make_shared<const DiffResult>(std::move(result));
    m_model->setResult(shared);

    m_diffRows.clear();
    m_currentDiffIndex = -1;
    for (int i = 0; i < shared->entries.size(); ++i) {
        if (shared->entries[i].type != DiffLineType::Unchanged) m_diffRows.append(i);
    }

    setSummary(*shared);
    if (shared->isCancelled) {
        m_statusLabel->setText("Diff computation cancelled.");
    } else if (!shared->isComplete()) {
        m_statusLabel->setText("⚠ INCOMPLETE: " + shared->statusMessage);
    } else if (!shared->hasDifferences()) {
        m_statusLabel->setText("The compared texts are identical (under the selected options).");
    } else {
        m_statusLabel->clear();
    }

    updateControlsEnabled();
    if (!m_searchInput->text().isEmpty()) onFindText(m_searchInput->text());
}

void DiffDialog::onFindText(const QString& query) {
    m_searchMatchingRows.clear();
    m_currentSearchIndex = -1;
    QSet<int> rows;

    const DiffResult* r = currentResult();
    if (!r || query.trimmed().isEmpty()) {
        m_searchStatusLabel->clear();
        m_model->setSearchMatches(rows);
        return;
    }
    for (int i = 0; i < r->entries.size(); ++i) {
        const auto& e = r->entries[i];
        if (e.textA.contains(query, Qt::CaseInsensitive) || e.textB.contains(query, Qt::CaseInsensitive)) {
            m_searchMatchingRows.append(i);
            rows.insert(i);
        }
    }
    m_model->setSearchMatches(rows);
    if (m_searchMatchingRows.isEmpty()) {
        m_searchStatusLabel->setText("0 matches");
    } else {
        m_currentSearchIndex = 0;
        m_searchStatusLabel->setText(QString("1 of %1").arg(m_searchMatchingRows.size()));
        scrollToSourceRow(m_searchMatchingRows[0]);
    }
}

void DiffDialog::onFindNext() {
    if (m_searchMatchingRows.isEmpty()) return;
    m_currentSearchIndex = (m_currentSearchIndex + 1) % m_searchMatchingRows.size();
    m_searchStatusLabel->setText(QString("%1 of %2").arg(m_currentSearchIndex + 1).arg(m_searchMatchingRows.size()));
    scrollToSourceRow(m_searchMatchingRows[m_currentSearchIndex]);
}

void DiffDialog::onFindPrev() {
    if (m_searchMatchingRows.isEmpty()) return;
    m_currentSearchIndex = (m_currentSearchIndex - 1 + m_searchMatchingRows.size()) % m_searchMatchingRows.size();
    m_searchStatusLabel->setText(QString("%1 of %2").arg(m_currentSearchIndex + 1).arg(m_searchMatchingRows.size()));
    scrollToSourceRow(m_searchMatchingRows[m_currentSearchIndex]);
}

void DiffDialog::onNextDiff() {
    if (m_diffRows.isEmpty()) return;
    m_currentDiffIndex = (m_currentDiffIndex + 1) % m_diffRows.size();
    scrollToSourceRow(m_diffRows[m_currentDiffIndex]);
}

void DiffDialog::onPrevDiff() {
    if (m_diffRows.isEmpty()) return;
    m_currentDiffIndex = (m_currentDiffIndex - 1 + m_diffRows.size()) % m_diffRows.size();
    scrollToSourceRow(m_diffRows[m_currentDiffIndex]);
}

void DiffDialog::scrollToSourceRow(int sourceRow) {
    QModelIndex proxyIdx = m_proxy->mapFromSource(m_model->index(sourceRow, DiffTableModel::TextA));
    if (!proxyIdx.isValid()) {
        // Filtered out (unchanged line while "only differences" is on): show everything again.
        m_hideUnchangedCheck->setChecked(false);
        proxyIdx = m_proxy->mapFromSource(m_model->index(sourceRow, DiffTableModel::TextA));
    }
    if (!proxyIdx.isValid()) return;
    m_table->selectRow(proxyIdx.row());
    m_table->scrollTo(proxyIdx, QAbstractItemView::PositionAtCenter);
}

void DiffDialog::onCopySelection() {
    const auto rows = m_table->selectionModel()->selectedRows();
    QVector<int> srcRows;
    for (const QModelIndex& i : rows) srcRows.append(m_proxy->mapToSource(i).row());
    std::sort(srcRows.begin(), srcRows.end());
    QStringList out;
    for (int r : srcRows) out << m_model->rowText(r);
    if (!out.isEmpty()) QGuiApplication::clipboard()->setText(out.join('\n'));
}

QString DiffDialog::patchOrAsk(bool* ok) {
    *ok = false;
    const DiffResult* r = currentResult();
    if (!r) return {};
    const QString labelA = m_comboA ? m_comboA->currentText() : "Pane A";
    const QString labelB = m_comboB ? m_comboB->currentText() : "Pane B";
    QString err;
    QString patch = AssemblyDiffProcessor::toUnifiedDiff(*r, labelA, labelB, &err);
    if (!err.isEmpty()) {
        const auto answer = QMessageBox::warning(
            this, "Incomplete diff",
            err + "\n\nA complete patch cannot be created. Export the incomplete diff anyway? "
                  "It is clearly marked as incomplete and cannot be applied with patch(1).",
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (answer != QMessageBox::Yes) return {};
        patch = AssemblyDiffProcessor::toUnifiedDiff(*r, labelA, labelB, nullptr, true);
    }
    *ok = true;
    return patch;
}

void DiffDialog::onCopyUnifiedDiff() {
    bool ok = false;
    const QString patch = patchOrAsk(&ok);
    if (!ok) return;
    QGuiApplication::clipboard()->setText(patch);
    m_statusLabel->setText("Unified diff copied to the clipboard.");
}

void DiffDialog::onExportUnifiedDiff() {
    bool ok = false;
    const QString patch = patchOrAsk(&ok);
    if (!ok) return;

    const QString fileName = QFileDialog::getSaveFileName(
        this, "Export Unified Diff", "", "Patch Files (*.patch *.diff);;All Files (*)");
    if (fileName.isEmpty()) return;

    QSaveFile file(fileName);  // atomic: a failed write cannot destroy an existing file
    if (!file.open(QIODevice::WriteOnly) || file.write(patch.toUtf8()) < 0 || !file.commit()) {
        QMessageBox::warning(this, "Export Error", "Could not write to file: " + fileName + "\n" + file.errorString());
        return;
    }
    m_statusLabel->setText("Unified diff saved to " + fileName);
}

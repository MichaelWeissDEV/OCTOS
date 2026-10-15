#include "CompilerOutputPane.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QVariant>
#include "CodeEditor.h"
#include "Theme.h"
#include "../strategies/LanguageStrategyRegistry.h"
#include "../core/CompilerRegistry.h"
#include "../highlighters/AsmHighlighter.h"
#include "../highlighters/ArtifactHighlighter.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>

Q_DECLARE_METATYPE(Language)
Q_DECLARE_METATYPE(SyntaxFlavor)

quint64 CompilerOutputPane::s_nextPaneId = 0;

CompilerOutputPane::CompilerOutputPane(DockerCompilerManager* dockerManager, QWidget* parent)
    : QWidget(parent),
      m_dockerManager(dockerManager),
      m_paneId(++s_nextPaneId),
      m_currentJobId(-1),
      m_currentLanguage(Language::Cpp),
      m_currentSyntax(SyntaxFlavor::Intel) {
    setupUi();
    updateCompatibilityState();
}

void CompilerOutputPane::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_headerWidget = new QWidget();
    Theme::setRole(m_headerWidget, "toolbar");

    QGridLayout* headerLayout = new QGridLayout(m_headerWidget);
    headerLayout->setContentsMargins(6, 4, 6, 4);
    headerLayout->setHorizontalSpacing(6);
    headerLayout->setVerticalSpacing(4);

    QLabel* langLabel = new QLabel("Language:");
    Theme::setRole(langLabel, "toolbarLabel");

    m_languageCombo = new QComboBox();
    m_languageCombo->addItem("C++", QVariant::fromValue(Language::Cpp));
    m_languageCombo->addItem("C", QVariant::fromValue(Language::C));
    m_languageCombo->addItem("Rust", QVariant::fromValue(Language::Rust));
    m_languageCombo->addItem("Java", QVariant::fromValue(Language::Java));
    m_languageCombo->addItem("Python", QVariant::fromValue(Language::Python));
    m_languageCombo->addItem("C#", QVariant::fromValue(Language::CSharp));
    m_languageCombo->addItem("Ada", QVariant::fromValue(Language::Ada));
    m_languageCombo->setMinimumWidth(70);
    m_languageCombo->setMaximumWidth(80);

    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CompilerOutputPane::onLanguageChanged);

    QLabel* syntaxLabel = new QLabel("Syntax:");
    Theme::setRole(syntaxLabel, "toolbarLabel");

    m_syntaxCombo = new QComboBox();
    m_syntaxCombo->addItem("Intel", QVariant::fromValue(SyntaxFlavor::Intel));
    m_syntaxCombo->addItem("AT&T", QVariant::fromValue(SyntaxFlavor::ATT));
    m_syntaxCombo->setMinimumWidth(60);
    m_syntaxCombo->setMaximumWidth(70);

    connect(m_syntaxCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CompilerOutputPane::onSyntaxChanged);

    QLabel* compilerLabel = new QLabel("Compiler:");
    Theme::setRole(compilerLabel, "toolbarLabel");

    m_compilerCombo = new QComboBox();
    m_compilerCombo->setMinimumWidth(120);

    connect(m_compilerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CompilerOutputPane::onCompilerChanged);

    QLabel* flagsLabel = new QLabel("Flags:");
    Theme::setRole(flagsLabel, "toolbarLabel");

    m_flagsInput = new QLineEdit();
    m_flagsInput->setPlaceholderText("e.g., -O3 -march=native");
    m_flagsInput->setMinimumWidth(100);

    connect(m_flagsInput, &QLineEdit::textChanged, this, &CompilerOutputPane::onFlagsChanged);

    m_closeButton = new QPushButton("✕");
    m_closeButton->setFixedSize(24, 24);
    Theme::setRole(m_closeButton, "subtle");
    connect(m_closeButton, &QPushButton::clicked, this, &CompilerOutputPane::onCloseClicked);

    QHBoxLayout* titleLayout = new QHBoxLayout();
    m_titleLabel = new QLabel("Compiler Output");
    Theme::setRole(m_titleLabel, "title");
    m_statusLabel = new QLabel("");
    Theme::setRole(m_statusLabel, "muted");

    titleLayout->addWidget(m_titleLabel);
    titleLayout->addWidget(m_statusLabel);
    titleLayout->addStretch();

    headerLayout->addLayout(titleLayout, 0, 0, 1, 6);
    headerLayout->addWidget(m_closeButton, 0, 6, 1, 1, Qt::AlignTop | Qt::AlignRight);

    headerLayout->addWidget(langLabel, 1, 0);
    headerLayout->addWidget(m_languageCombo, 1, 1);
    headerLayout->addWidget(syntaxLabel, 1, 2);
    headerLayout->addWidget(m_syntaxCombo, 1, 3);
    headerLayout->addWidget(compilerLabel, 1, 4);
    headerLayout->addWidget(m_compilerCombo, 1, 5, 1, 2);

    headerLayout->addWidget(flagsLabel, 2, 0);
    headerLayout->addWidget(m_flagsInput, 2, 1, 1, 6);

    headerLayout->setColumnStretch(5, 1);

    m_editor = new CodeEditor();
    m_editor->setReadOnly(true);
    m_editor->enableOpcodeTooltips(true);


    // Row 3: what was compiled how, and which result artifact is shown
    m_infoLabel = new QLabel();
    Theme::setRole(m_infoLabel, "muted");
    m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_infoLabel->setWordWrap(true);

    QLabel* artifactLabel = new QLabel("Artifact:");
    Theme::setRole(artifactLabel, "toolbarLabel");
    m_artifactCombo = new QComboBox();
    m_artifactCombo->setMinimumWidth(140);
    m_artifactCombo->setToolTip("Choose which result to view: assembly / IR / bytecode, compiler diagnostics or raw output");
    connect(m_artifactCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { emit artifactSelectionChanged(); });
    m_cancelButton = new QPushButton("Cancel");
    Theme::setRole(m_cancelButton, "danger");
    m_cancelButton->setToolTip("Stop this compilation");
    m_cancelButton->setVisible(false);
    connect(m_cancelButton, &QPushButton::clicked, this, &CompilerOutputPane::cancelRequested);

    headerLayout->addWidget(artifactLabel, 3, 0);
    headerLayout->addWidget(m_artifactCombo, 3, 1, 1, 3);
    m_saveButton = new QPushButton("Save…");
    Theme::setRole(m_saveButton, "subtle");
    m_saveButton->setToolTip("Save the shown artifact to a file (the temporary compiler sandbox is gone after a compilation)");
    m_saveButton->setVisible(false);
    connect(m_saveButton, &QPushButton::clicked, this, &CompilerOutputPane::onSaveArtifact);
    headerLayout->addWidget(m_saveButton, 3, 4);
    headerLayout->addWidget(m_cancelButton, 3, 5, 1, 2, Qt::AlignRight);
    headerLayout->addWidget(m_infoLabel, 4, 0, 1, 7);
    artifactLabel->setVisible(false);
    m_artifactCombo->setVisible(false);
    m_artifactCombo->setProperty("labelWidget", QVariant::fromValue<QObject*>(artifactLabel));
    m_infoLabel->setVisible(false);

    // Structured diagnostics instead of a wall of compiler text
    m_diagnosticsTable = new QTableWidget(0, 5);
    m_diagnosticsTable->setHorizontalHeaderLabels({"Severity", "File", "Line", "Col", "Message"});
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_diagnosticsTable->verticalHeader()->setVisible(false);
    m_diagnosticsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_diagnosticsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_diagnosticsTable->setShowGrid(false);
    m_diagnosticsTable->setToolTip("Double-click a diagnostic to jump to the source line");
    connect(m_diagnosticsTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        auto* fileItem = m_diagnosticsTable->item(row, 1);
        auto* lineItem = m_diagnosticsTable->item(row, 2);
        auto* colItem = m_diagnosticsTable->item(row, 3);
        emit diagnosticActivated(fileItem ? fileItem->text() : QString(),
                                 lineItem ? lineItem->text().toInt() : 0, colItem ? colItem->text().toInt() : 0);
    });

    m_viewStack = new QStackedWidget();
    m_viewStack->addWidget(m_editor);
    m_viewStack->addWidget(m_diagnosticsTable);

    m_asmHighlighter = new AsmHighlighter(m_editor->document());

    m_noticeLabel = new QLabel();
    m_noticeLabel->setWordWrap(true);
    m_noticeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_noticeLabel->setVisible(false);
    Theme::setRole(m_noticeLabel, "noticeWarning");

    mainLayout->addWidget(m_headerWidget);
    mainLayout->addWidget(m_noticeLabel);
    mainLayout->addWidget(m_viewStack, 1);
    updateTitle();
}

void CompilerOutputPane::setStatus(const QString& status, bool isError) {
    if (status.isEmpty()) {
        m_statusLabel->setText("");
    } else if (isError) {
        Theme::setRole(m_statusLabel, "error");
        m_statusLabel->setText("[" + status + "]");
    } else {
        Theme::setRole(m_statusLabel, "ok");
        m_statusLabel->setText("[" + status + "]");
    }
}

void CompilerOutputPane::setStatusWarning(const QString& status) {
    Theme::setRole(m_statusLabel, "warning");
    m_statusLabel->setText(status.isEmpty() ? QString() : "[" + status + "]");
}

void CompilerOutputPane::setNotice(const QString& text, bool isError) {
    m_noticeLabel->setVisible(!text.trimmed().isEmpty());
    m_noticeLabel->setText(text);
    Theme::setRole(m_noticeLabel, isError ? "noticeError" : "noticeWarning");
}

void CompilerOutputPane::setOutputText(const QString& text, bool isError) {
    m_editor->setPlainText(text);
    m_editor->setVisible(true);
    // Error text is colored by the stylesheet through this property (never by a hard-coded palette).
    m_editor->setProperty("state", isError ? "error" : "ok");
    m_editor->style()->unpolish(m_editor);
    m_editor->style()->polish(m_editor);
    m_editor->update();
}

void CompilerOutputPane::setCompilerList(const QStringList& compilers) {
    QString current = m_compilerCombo->currentText();

    // Block signals to prevent spurious cascading compilations
    m_compilerCombo->blockSignals(true);
    m_compilerCombo->clear();
    m_compilerCombo->addItems(compilers);

    int index = m_compilerCombo->findText(current);
    if (index >= 0) {
        m_compilerCombo->setCurrentIndex(index);
    } else if (!compilers.isEmpty()) {
        m_compilerCombo->setCurrentIndex(0);
    }
    m_compilerCombo->blockSignals(false);

    updateTitle();
}

void CompilerOutputPane::setSelectedCompiler(const QString& compilerName) {
    int idx = m_compilerCombo->findText(compilerName);
    if (idx >= 0) {
        m_compilerCombo->setCurrentIndex(idx);
    }
}

void CompilerOutputPane::setLanguage(Language lang) {
    for (int i = 0; i < m_languageCombo->count(); ++i) {
        if (m_languageCombo->itemData(i).value<Language>() == lang) {
            m_languageCombo->setCurrentIndex(i);
            break;
        }
    }
}

void CompilerOutputPane::setCompilerFlags(const QString& flags) { m_flagsInput->setText(flags); }

void CompilerOutputPane::setSyntaxFlavor(SyntaxFlavor flavor) {
    for (int i = 0; i < m_syntaxCombo->count(); ++i) {
        if (m_syntaxCombo->itemData(i).value<SyntaxFlavor>() == flavor) { m_syntaxCombo->setCurrentIndex(i); break; }
    }
}

bool CompilerOutputPane::hasCompiler(const QString& compilerName) const {
    return m_compilerCombo->findText(compilerName) >= 0;
}

QString CompilerOutputPane::getSelectedCompiler() const { return m_compilerCombo->currentText(); }
QString CompilerOutputPane::getCompilerFlags() const { return m_flagsInput->text(); }
Language CompilerOutputPane::getLanguage() const { return m_currentLanguage; }
SyntaxFlavor CompilerOutputPane::getSyntaxFlavor() const { return m_currentSyntax; }
void CompilerOutputPane::setCloseButtonVisible(bool visible) { m_closeButton->setVisible(visible); }

void CompilerOutputPane::setAssemblyLineMapping(const QMap<int, QVector<int>>& mapping) {
    m_lineMapping = mapping;
}

void CompilerOutputPane::onCompilerChanged(int index) {
    if (index >= 0) {
        updateTitle();
        emit compilerChanged(m_compilerCombo->currentText());
    }
}

void CompilerOutputPane::onLanguageChanged(int index) {
    if (index >= 0) {
        m_currentLanguage = m_languageCombo->itemData(index).value<Language>();
        updateCompatibilityState();
        updateTitle();
        emit languageChanged(m_currentLanguage);
    }
}

void CompilerOutputPane::onSyntaxChanged(int index) {
    if (index >= 0) {
        m_currentSyntax = m_syntaxCombo->itemData(index).value<SyntaxFlavor>();
        updateTitle();
        emit syntaxChanged(m_currentSyntax);
    }
}

void CompilerOutputPane::onFlagsChanged() {
    updateTitle();
    emit flagsChanged(m_flagsInput->text());
}

void CompilerOutputPane::onCloseClicked() {
    emit closeRequested();
}

void CompilerOutputPane::updateCompatibilityState() {
    bool supportsSyntax = LanguageStrategyRegistry::instance().supportsSyntaxFlavor(m_currentLanguage);
    m_syntaxCombo->setEnabled(supportsSyntax);
    if (!supportsSyntax) {
        m_syntaxCombo->setToolTip("Syntax flavor (Intel/AT&T) is not applicable to " + languageToString(m_currentLanguage));
    } else {
        m_syntaxCombo->setToolTip("Select assembly syntax flavor (Intel / AT&T)");
    }
}

void CompilerOutputPane::updateTitle() {
    QString lang = m_languageCombo->currentText();
    QString comp = m_compilerCombo->currentText();
    if (comp.isEmpty()) comp = "No Compiler Selected";
    QString syn = m_syntaxCombo->isEnabled() ? (" | " + m_syntaxCombo->currentText()) : "";
    QString flags = m_flagsInput->text();
    if (!flags.isEmpty()) flags = " | " + flags;
    m_titleLabel->setText(QString("%1 | %2%3%4").arg(lang, comp, syn, flags));
}


// ---------------------------------------------------------------------------------------------
// Result / artifact viewer
// ---------------------------------------------------------------------------------------------

namespace {
bool isCodeArtifact(ArtifactType t) {
    return t == ArtifactType::Assembly || t == ArtifactType::LLVM_IR || t == ArtifactType::JVM_Bytecode ||
           t == ArtifactType::CPython_Bytecode || t == ArtifactType::CIL;
}
}

void CompilerOutputPane::setResult(const CompilationResult& result) {
    m_result = result;

    QSignalBlocker blocker(m_artifactCombo);
    m_artifactCombo->clear();
    int preferred = -1;
    for (int i = 0; i < result.artifacts.size(); ++i) {
        const CompilationArtifact& a = result.artifacts[i];
        // The structured diagnostics list below replaces the textual Diagnostics artifact.
        if (a.type == ArtifactType::Diagnostics && !result.diagnostics.isEmpty()) continue;
        QString label = a.name.isEmpty() ? artifactTypeToString(a.type) : QString("%1 – %2").arg(artifactTypeToString(a.type), a.name);
        if (a.truncated) label += " (incomplete)";
        m_artifactCombo->addItem(label, i);
        if (preferred < 0 && isCodeArtifact(a.type)) preferred = m_artifactCombo->count() - 1;
    }
    if (!result.diagnostics.isEmpty()) {
        m_artifactCombo->addItem(QString("Diagnostics (%1)").arg(result.diagnostics.size()), -1);
        if (preferred < 0 || !result.success()) preferred = m_artifactCombo->count() - 1;
    }
    if (preferred < 0 && m_artifactCombo->count() > 0) preferred = 0;
    if (preferred >= 0) m_artifactCombo->setCurrentIndex(preferred);

    const bool multiple = m_artifactCombo->count() > 1;
    m_artifactCombo->setVisible(multiple);
    m_saveButton->setVisible(!result.artifacts.isEmpty());
    if (auto* lbl = qobject_cast<QWidget*>(m_artifactCombo->property("labelWidget").value<QObject*>()))
        lbl->setVisible(multiple);

    // Diagnostics table content
    m_diagnosticsTable->setRowCount(0);
    for (const Diagnostic& d : result.diagnostics) {
        const int row = m_diagnosticsTable->rowCount();
        m_diagnosticsTable->insertRow(row);
        QString sev;
        QColor color = Theme::currentTokens().text;
        switch (d.severity) {
            case DiagnosticSeverity::Error: sev = "Error"; color = Theme::currentTokens().error; break;
            case DiagnosticSeverity::Warning: sev = "Warning"; color = Theme::currentTokens().warning; break;
            case DiagnosticSeverity::Note: sev = "Note"; color = Theme::currentTokens().info; break;
            default: sev = "Info"; break;
        }
        auto* sevItem = new QTableWidgetItem(sev);
        sevItem->setForeground(color);
        m_diagnosticsTable->setItem(row, 0, sevItem);
        m_diagnosticsTable->setItem(row, 1, new QTableWidgetItem(d.fileName));
        m_diagnosticsTable->setItem(row, 2, new QTableWidgetItem(d.line > 0 ? QString::number(d.line) : QString()));
        m_diagnosticsTable->setItem(row, 3, new QTableWidgetItem(d.column > 0 ? QString::number(d.column) : QString()));
        m_diagnosticsTable->setItem(row, 4, new QTableWidgetItem(d.message));
    }

    // Info line: compiler (+ verified version), flags, architecture, runtime
    QStringList info;
    if (!result.compilerImage.isEmpty()) {
        const CompilerDefinition def = CompilerRegistry::instance().compilerForImage(result.compilerImage);
        QString c = result.compilerImage;
        if (!def.actualVersion().isEmpty()) c += " (" + def.actualVersion().left(60) + ")";
        info << c;
        if (!def.architecture.isEmpty() && def.verificationState() == VerificationState::Verified) info << def.architecture;
    }
    if (result.durationMs > 0) info << QString("%1 ms").arg(result.durationMs);
    if (!result.artifacts.isEmpty()) {
        qint64 bytes = 0;
        for (const auto& a : result.artifacts) if (isCodeArtifact(a.type)) bytes += a.sizeBytes;
        if (bytes > 0) info << QString("output %1 B").arg(bytes);
    }
    m_infoLabel->setText(info.join("  ·  "));
    m_infoLabel->setToolTip(result.compilerCommand.isEmpty() ? QString() : "Command inside the container:\n" + result.compilerCommand);
    m_infoLabel->setVisible(!info.isEmpty());
}

int CompilerOutputPane::currentArtifactIndex() const {
    return m_artifactCombo->currentIndex() >= 0 ? m_artifactCombo->currentData().toInt() : -1;
}

bool CompilerOutputPane::showsDiagnostics() const {
    return m_viewStack->currentWidget() == m_diagnosticsTable;
}

void CompilerOutputPane::showDiagnosticsView() {
    m_viewStack->setCurrentWidget(m_diagnosticsTable);
}

void CompilerOutputPane::showArtifactText(const QString& text, ArtifactType type, bool isError) {
    // Highlighting follows the artifact: x86 assembly rules must not recolor JVM or Python bytecode.
    if (type == ArtifactType::Assembly || type == ArtifactType::RawOutput || type == ArtifactType::Other ||
        type == ArtifactType::Diagnostics || type == ArtifactType::CompilerMetadata) {
        if (m_artifactHighlighter) { delete m_artifactHighlighter; m_artifactHighlighter = nullptr; }
        if (!m_asmHighlighter) m_asmHighlighter = new AsmHighlighter(m_editor->document());
        m_asmHighlighter->setDocument(type == ArtifactType::Assembly ? m_editor->document() : nullptr);
    } else {
        if (m_asmHighlighter) m_asmHighlighter->setDocument(nullptr);
        if (!m_artifactHighlighter || m_artifactHighlighter->artifactType() != type) {
            delete m_artifactHighlighter;
            m_artifactHighlighter = new ArtifactHighlighter(m_editor->document(), type);
        }
    }
    m_viewStack->setCurrentWidget(m_editor);
    setOutputText(text, isError);
}

void CompilerOutputPane::setBusy(bool busy) {
    m_cancelButton->setVisible(busy);
}

void CompilerOutputPane::applyTheme() {
    if (m_asmHighlighter) m_asmHighlighter->applyTheme();
    if (m_artifactHighlighter) m_artifactHighlighter->applyTheme();
    m_editor->applyTheme();
    // The diagnostics colors come from the tokens at fill time: refill them for the new theme.
    if (!m_result.diagnostics.isEmpty()) {
        const int idx = m_artifactCombo->currentIndex();
        setResult(m_result);
        if (idx >= 0 && idx < m_artifactCombo->count()) m_artifactCombo->setCurrentIndex(idx);
    }
}

void CompilerOutputPane::onSaveArtifact() {
    const int idx = currentArtifactIndex();
    if (idx < 0 || idx >= m_result.artifacts.size()) return;
    CompilationArtifact art = m_result.artifacts[idx];
    QString suffix = art.type == ArtifactType::LLVM_IR ? "ll" : art.type == ArtifactType::CompilerMetadata ? "json"
                   : art.type == ArtifactType::Assembly ? "s" : "txt";
    const QString path = QFileDialog::getSaveFileName(this, "Save Artifact", QString("%1.%2").arg(QFileInfo(art.name).completeBaseName().isEmpty() ? "artifact" : QFileInfo(art.name).completeBaseName(), suffix));
    if (path.isEmpty()) return;
    QString err;
    if (!art.saveTo(path, &err)) QMessageBox::warning(this, "Save Artifact", err);
    else if (art.truncated) QMessageBox::information(this, "Save Artifact", "The saved artifact is incomplete: the compiler output exceeded the size limit.");
}

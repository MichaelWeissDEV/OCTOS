#include "MainWindow.h"
#include "backend/JobManager.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolTip>
#include <QToolBar>
#include <QVBoxLayout>

#include "core/AppSettings.h"
#include "core/CompilerRegistry.h"
#include "highlighters/RustHighlighter.h"
#include "highlighters/PythonHighlighter.h"
#include "managers/RecentFilesManager.h"
#include "managers/SnippetManager.h"
#include "utils/OpcodeReference.h"
#include "strategies/LanguageStrategyRegistry.h"
#include "ui/AnalysisPanel.h"
#include "ui/DiffDialog.h"
#include "core/ResultPresentation.h"
#include "ui/Theme.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_currentTheme(ThemeType::Dark) {
    setAcceptDrops(true);

    m_dockerManager = new DockerCompilerManager(this);
    connect(m_dockerManager, &DockerCompilerManager::compilationResultReceived, this,
            &MainWindow::onCompilationResultReceived);
    // The image list arrives asynchronously; nothing in the GUI path blocks on `docker images`.
    connect(m_dockerManager, &DockerCompilerManager::installedImagesRefreshed, this,
            [this](const QVector<DockerImage>&) { refreshDockerCompilerList(); });

    m_project = new ProjectController(this);
    setupUi();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(AppSettings::instance().debounceIntervalMs());

    setActiveEditor(m_inputEditor);
    connect(m_debounceTimer, &QTimer::timeout, this, &MainWindow::doCompile);

    qApp->setStyle("Fusion");
    int savedTheme = AppSettings::instance().themeIndex();
    onChangeTheme(savedTheme);

    refreshDockerCompilerList();
    updateWindowTitle();
    restoreLayout();
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    const bool projectOk = !m_project->isOpen() || maybeSaveProject();
    if (projectOk && (m_project->isOpen() || maybeSave())) {
        saveLayout();
        if (m_dockerManager && m_dockerManager->jobManager()) {
            m_dockerManager->jobManager()->shutdown();
        }
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // File Menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* newAction = fileMenu->addAction("&New");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onNew);

    QAction* openAction = fileMenu->addAction("&Open...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onLoad);

    m_recentMenu = fileMenu->addMenu("Recent Files");
    updateRecentFilesMenu();

    fileMenu->addSeparator();

    QAction* saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSave);

    QAction* saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAs);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // Filters Menu
    QMenu* filtersMenu = menuBar()->addMenu("F&ilters");

    m_filterShowSegmentDirectives = new QAction("Show Segment Directives", this);
    m_filterShowSegmentDirectives->setCheckable(true);
    m_filterShowSegmentDirectives->setChecked(m_filterSettings.showSegmentDirectives);
    connect(m_filterShowSegmentDirectives, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterShowSegmentDirectives);

    m_filterShowDataDirectives = new QAction("Show Data Directives", this);
    m_filterShowDataDirectives->setCheckable(true);
    m_filterShowDataDirectives->setChecked(m_filterSettings.showDataDirectives);
    connect(m_filterShowDataDirectives, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterShowDataDirectives);

    m_filterShowCfiDirectives = new QAction("Show CFI Directives", this);
    m_filterShowCfiDirectives->setCheckable(true);
    m_filterShowCfiDirectives->setChecked(m_filterSettings.showCfiDirectives);
    connect(m_filterShowCfiDirectives, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterShowCfiDirectives);

    m_filterShowMetadataLabels = new QAction("Show Metadata Labels", this);
    m_filterShowMetadataLabels->setCheckable(true);
    m_filterShowMetadataLabels->setChecked(m_filterSettings.showMetadataLabels);
    connect(m_filterShowMetadataLabels, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterShowMetadataLabels);

    m_filterShowUnusedLabels = new QAction("Keep Unused Labels", this);
    m_filterShowUnusedLabels->setCheckable(true);
    m_filterShowUnusedLabels->setChecked(m_filterSettings.showUnusedLabels);
    connect(m_filterShowUnusedLabels, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterShowUnusedLabels);

    m_filterHideEmptyLabels = new QAction("Hide Empty Labels", this);
    m_filterHideEmptyLabels->setCheckable(true);
    m_filterHideEmptyLabels->setChecked(m_filterSettings.hideEmptyLabels);
    connect(m_filterHideEmptyLabels, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterHideEmptyLabels);

    m_filterShowDebugInfo = new QAction("Show Debug Info (.loc)", this);
    m_filterShowDebugInfo->setCheckable(true);
    m_filterShowDebugInfo->setChecked(m_filterSettings.showDebugInfo);
    connect(m_filterShowDebugInfo, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterShowDebugInfo);

    m_filterShowComments = new QAction("Show Comments", this);
    m_filterShowComments->setCheckable(true);
    m_filterShowComments->setChecked(m_filterSettings.showComments);
    connect(m_filterShowComments, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterShowComments);

    m_filterDemangle = new QAction("Demangle Identifiers", this);
    m_filterDemangle->setCheckable(true);
    m_filterDemangle->setChecked(m_filterSettings.demangleIdentifiers);
    connect(m_filterDemangle, &QAction::triggered, this, &MainWindow::onRefilterOutput);
    filtersMenu->addAction(m_filterDemangle);

    // Tools Menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    QAction* dockerCompilersAction = toolsMenu->addAction("Docker &Compiler Manager...");
    connect(dockerCompilersAction, &QAction::triggered, this, &MainWindow::onManageDockerCompilers);

    toolsMenu->addSeparator();
    QMenu* snippetsMenu = toolsMenu->addMenu("Insert &Snippet");
    QStringList snippetNames = m_snippetManager.getAllSnippetNames();
    for (const QString& name : snippetNames) {
        QAction* snippetAction = snippetsMenu->addAction(name);
        connect(snippetAction, &QAction::triggered, this, &MainWindow::onInsertSnippet);
    }

    toolsMenu->addSeparator();
    QMenu* themeMenu = toolsMenu->addMenu("&Theme");
    QStringList themes = Theme::getAvailableThemes();
    for (int i = 0; i < themes.size(); ++i) {
        QAction* themeAction = themeMenu->addAction(themes[i]);
        themeAction->setData(i);
        connect(themeAction, &QAction::triggered, this, [this, i]() { onChangeTheme(i); });
    }

    toolsMenu->addSeparator();
    m_compileNowAction = toolsMenu->addAction("Compile &Now");
    m_compileNowAction->setShortcuts({QKeySequence(Qt::Key_F5), QKeySequence(Qt::CTRL | Qt::Key_Return)});
    m_compileNowAction->setToolTip("Compile with the current settings (F5), also when auto-compile is off");
    connect(m_compileNowAction, &QAction::triggered, this, &MainWindow::onCompileNow);
    QAction* autoCompileAction = toolsMenu->addAction("Auto-Compile on change");
    autoCompileAction->setCheckable(true);
    autoCompileAction->setChecked(AppSettings::instance().autoCompile());
    connect(autoCompileAction, &QAction::toggled, this, [](bool enabled) {
        AppSettings::instance().setAutoCompile(enabled);
    });

    QMenu* viewMenu = menuBar()->addMenu("&View");
    setupProjectAndProfileMenus();

    m_splitter = new QSplitter(Qt::Horizontal);

    m_sourceEditorContainer = new QWidget();
    QVBoxLayout* sourceLayout = new QVBoxLayout(m_sourceEditorContainer);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    sourceLayout->setSpacing(0);

    QWidget* sourceToolbar = new QWidget();
    Theme::setRole(sourceToolbar, "toolbar");
    QHBoxLayout* sourceToolbarLayout = new QHBoxLayout(sourceToolbar);
    sourceToolbarLayout->setContentsMargins(8, 4, 8, 4);
    sourceToolbarLayout->setSpacing(8);

    QLabel* sourceLabel = new QLabel("Source Code");
    Theme::setRole(sourceLabel, "toolbarLabel");
    sourceToolbarLayout->addWidget(sourceLabel);
    sourceToolbarLayout->addStretch();

    m_highlightToggle = new QPushButton("Highlight: Off");
    m_highlightToggle->setToolTip("Toggle highlighting: Off → Highlight All → Highlight Selection");
    Theme::setRole(m_highlightToggle, "subtle");
    connect(m_highlightToggle, &QPushButton::clicked, this, &MainWindow::onToggleHighlightMode);
    sourceToolbarLayout->addWidget(m_highlightToggle);

    auto* compileButton = new QPushButton("Compile (F5)");
    compileButton->setToolTip("Compile now, also when auto-compile is turned off");
    Theme::setRole(compileButton, "success");
    connect(compileButton, &QPushButton::clicked, this, &MainWindow::onCompileNow);
    sourceToolbarLayout->addWidget(compileButton);

    m_addPaneButton = new QPushButton("+ Compare");
    Theme::setRole(m_addPaneButton, "accent");
    connect(m_addPaneButton, &QPushButton::clicked, this, &MainWindow::onAddComparisonPane);
    sourceToolbarLayout->addWidget(m_addPaneButton);

    m_compareDiffButton = new QPushButton("Diff View");
    m_compareDiffButton->setToolTip("View side-by-side assembly diff between panels");
    m_compareDiffButton->setEnabled(false);
    Theme::setRole(m_compareDiffButton, "success");
    connect(m_compareDiffButton, &QPushButton::clicked, this, &MainWindow::onCompareDiff);
    sourceToolbarLayout->addWidget(m_compareDiffButton);

    m_closeOthersButton = new QPushButton("Close Others");
    Theme::setRole(m_closeOthersButton, "subtle");
    connect(m_closeOthersButton, &QPushButton::clicked, this, &MainWindow::onCloseOtherPanes);
    sourceToolbarLayout->addWidget(m_closeOthersButton);

    sourceLayout->addWidget(sourceToolbar);

    // Editor tabs: a stand-alone file uses one tab (no tab bar is shown); project mode opens one tab per file.
    m_editorTabs = new QTabWidget();
    m_editorTabs->setTabsClosable(true);
    m_editorTabs->setDocumentMode(true);
    m_editorTabs->setMovable(true);
    m_editorTabs->tabBar()->setAutoHide(true);
    connect(m_editorTabs, &QTabWidget::currentChanged, this, &MainWindow::onEditorTabChanged);
    connect(m_editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onEditorTabCloseRequested);
    m_inputEditor = createEditorTab("Untitled", QString(), Language::Cpp);
    m_inputEditor->setPlaceholderText("\n Enter your code here...");

    sourceLayout->addWidget(m_editorTabs);

    m_outputSplitter = new QSplitter(Qt::Horizontal);

    CompilerOutputPane* initialPane = new CompilerOutputPane(m_dockerManager);
    wirePane(initialPane);

    m_outputPanes.append(initialPane);
    m_paneLookup[initialPane->paneId()] = initialPane;
    m_outputSplitter->addWidget(initialPane);

    initialPane->setCloseButtonVisible(false); // First pane cannot be closed if alone

    m_splitter->addWidget(m_sourceEditorContainer);
    m_splitter->addWidget(m_outputSplitter);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({600, 600});

    mainLayout->addWidget(m_splitter, 1);

    setupDocks(viewMenu);

    onRefilterOutput();
}

void MainWindow::wirePane(CompilerOutputPane* pane) {
    connect(pane, &CompilerOutputPane::closeRequested, this, [this, pane]() { onRemovePane(pane); });
    connect(pane, &CompilerOutputPane::compilerChanged, this, &MainWindow::onPaneSettingsChanged);
    connect(pane, &CompilerOutputPane::flagsChanged, this, &MainWindow::onPaneSettingsChanged);
    connect(pane, &CompilerOutputPane::languageChanged, this, [this, pane](Language lang) {
        onPaneLanguageChanged(lang, pane);
        onPaneSettingsChanged();
    });
    connect(pane, &CompilerOutputPane::syntaxChanged, this, &MainWindow::onPaneSettingsChanged);
    connect(pane, &CompilerOutputPane::outputKindChanged, this, &MainWindow::onPaneSettingsChanged);
    connect(pane, &CompilerOutputPane::cancelRequested, this, [this, pane]() {
        if (pane->getCurrentJobId() > 0) m_dockerManager->cancelJob(pane->getCurrentJobId());
    });
    connect(pane, &CompilerOutputPane::artifactSelectionChanged, this, [this, pane]() { showSelectedArtifact(pane); });
    connect(pane, &CompilerOutputPane::diagnosticActivated, this, [this](const QString&, int line, int) {
        if (line > 0 && m_inputEditor) m_inputEditor->goToLine(line);
    });
}

void MainWindow::onAddComparisonPane() {
    CompilerOutputPane* newPane = new CompilerOutputPane(m_dockerManager);
    wirePane(newPane);

    m_outputPanes.append(newPane);
    m_paneLookup[newPane->paneId()] = newPane;
    m_outputSplitter->addWidget(newPane);

    refreshDockerCompilerList();

    newPane->setCloseButtonVisible(true);
    if (m_outputPanes.size() > 1) {
        m_outputPanes.first()->setCloseButtonVisible(true);
    }
    if (m_compareDiffButton) {
        m_compareDiffButton->setEnabled(m_outputPanes.size() >= 2);
    }

    QList<int> sizes;
    int totalWidth = m_outputSplitter->width();
    int paneWidth = qMax(100, totalWidth / m_outputPanes.size());
    for (int i = 0; i < m_outputPanes.size(); ++i) {
        sizes.append(paneWidth);
    }
    m_outputSplitter->setSizes(sizes);

    doCompile();
}

void MainWindow::onCloseOtherPanes() {
    if (m_outputPanes.isEmpty()) {
        return;
    }

    while (m_outputPanes.size() > 1) {
        CompilerOutputPane* pane = m_outputPanes.last();
        onRemovePane(pane);
    }

    if (!m_outputPanes.isEmpty()) {
        m_outputPanes.first()->setCloseButtonVisible(false);
    }
    if (m_compareDiffButton) {
        m_compareDiffButton->setEnabled(false);
    }
}

void MainWindow::onRemovePane(CompilerOutputPane* pane) {
    int index = m_outputPanes.indexOf(pane);
    if (index >= 0) {
        quint64 paneId = pane->paneId();
        m_dockerManager->jobManager()->invalidatePane(paneId);

        m_outputPanes.removeAt(index);
        m_paneLookup.remove(paneId);

        if (m_outputPanes.size() == 1) {
            m_outputPanes.first()->setCloseButtonVisible(false);
        }
        if (m_compareDiffButton) {
            m_compareDiffButton->setEnabled(m_outputPanes.size() >= 2);
        }

        pane->deleteLater();
        updateEditorMarkers();
    }
}

void MainWindow::refreshDockerCompilerList() {
    QVector<DockerImage> installedImages = m_dockerManager->listInstalledImages();

    for (CompilerOutputPane* pane : m_outputPanes) {
        QStringList compilers = getCompilersForLanguage(pane->getLanguage());
        pane->setCompilerList(compilers);
    }
}

QStringList MainWindow::getCompilersForLanguage(Language lang) {
    QVector<DockerImage> installedImages = m_dockerManager->listInstalledImages();
    QStringList filtered;

    for (const DockerImage& image : installedImages) {
        if (CompilerRegistry::instance().isImageCompatibleWithLanguage(image.fullName, lang)) {
            filtered.append(image.fullName);
        }
    }

    return filtered;
}

void MainWindow::updateSourceHighlighter(Language lang) {
    applyHighlighter(m_inputEditor, lang);
    m_sourceHighlighter = m_highlighters.value(m_inputEditor, nullptr);
}

void MainWindow::applyHighlighter(CodeEditor* editor, Language lang) {
    if (!editor) return;
    delete m_highlighters.take(editor);

    QSyntaxHighlighter* h = nullptr;
    switch (lang) {
        case Language::Rust: h = new RustHighlighter(editor->document()); break;
        case Language::Python: h = new PythonHighlighter(editor->document()); break;
        case Language::Cpp:
        case Language::C:
        case Language::Java:
        case Language::CSharp:
        case Language::Ada:
        default: h = new CppHighlighter(editor->document()); break;
    }
    m_highlighters[editor] = h;
}

void MainWindow::onPaneLanguageChanged(Language lang, CompilerOutputPane* pane) {
    QStringList compilers = getCompilersForLanguage(lang);
    pane->setCompilerList(compilers);

    // If the primary pane (first pane) changed language, adapt the source highlighter
    if (!m_outputPanes.isEmpty() && m_outputPanes.first() == pane) {
        updateSourceHighlighter(lang);
    }
}

void MainWindow::updateCompilerLists() { refreshDockerCompilerList(); }

void MainWindow::onInputTextChanged() {
    m_inputRevision++;
    if (AppSettings::instance().autoCompile()) {
        m_debounceTimer->start(AppSettings::instance().debounceIntervalMs());
    }
}

void MainWindow::onPaneSettingsChanged() {
    m_inputRevision++;
    if (AppSettings::instance().autoCompile()) {
        m_debounceTimer->start(AppSettings::instance().debounceIntervalMs());
    }
}

void MainWindow::onDocumentModificationChanged(bool /*modified*/) {
    updateWindowTitle();
}

void MainWindow::doCompile() { compileAllPanes(); }

void MainWindow::onCompileNow() {
    m_debounceTimer->stop();   // an explicit request replaces a pending debounced one
    compileAllPanes();
}

void MainWindow::compileAllPanes() {
    QString source = m_inputEditor->toPlainText();

    if (source.trimmed().isEmpty()) {
        for (CompilerOutputPane* pane : m_outputPanes) {
            pane->setOutputText("No source code to compile");
            pane->setStatus("");
        }
        return;
    }

    for (CompilerOutputPane* pane : m_outputPanes) {
        QString compiler = pane->getSelectedCompiler();
        QString flags = pane->getCompilerFlags();
        Language lang = pane->getLanguage();
        SyntaxFlavor syntax = pane->getSyntaxFlavor();

        if (compiler.isEmpty()) {
            pane->setOutputText("No Docker Images installed. Please open Tools > Docker Compiler Manager to pull a compiler image.", true);
            pane->setStatus("No Image", true);
            continue;
        }

        CompilationRequest req;
        req.paneId = pane->paneId();
        req.revision = m_inputRevision;
        req.sourceCode = source;
        req.compilerImage = compiler;
        req.language = lang;
        req.syntaxFlavor = syntax;
        req.userArgs = flags;
        req.outputKind = pane->getOutputKind();
        req.timeoutMs = 30000;
        {   // The probed architecture is exact; otherwise images are native to the Docker daemon.
            const CompilerDefinition def = CompilerRegistry::instance().compilerForImage(compiler);
            req.targetArchitecture = def.verificationState() == VerificationState::Verified
                                         ? def.architecture : m_dockerManager->daemonArchitecture();
        }

        // Project mode: every other file of the project travels along (headers, modules).
        const QString mainPath = m_tabPath.value(m_inputEditor);
        if (m_project->isOpen() && !mainPath.isEmpty()) {
            req.mainFilePath = mainPath;
            for (const SourceFile& f : m_project->workspace().sourceFiles())
                if (f.path != mainPath) req.extraFiles.append({f.path, f.content});
        }

        pane->setStatus("Compiling...");
        pane->setNotice(QString());
        pane->setBusy(true);
        quint64 jobId = m_dockerManager->submitRequest(req);
        pane->setCurrentJobId(static_cast<int>(jobId));
    }
}

void MainWindow::onCompilationResultReceived(const CompilationResult& incoming) {
    CompilerOutputPane* pane = m_paneLookup.value(incoming.paneId, nullptr);
    if (!pane) {
        return;
    }

    // Check if pane still expects this exact job ID
    if (pane->getCurrentJobId() != static_cast<int>(incoming.jobId)) {
        return;
    }

    // Verify revision: discard outdated job results
    if (incoming.revision < m_inputRevision) {
        return;
    }

    CompilationResult result = incoming;
    pane->setBusy(false);
    {   // Version / architecture / digest are only claimed if the image was actually probed.
        const CompilerDefinition def = CompilerRegistry::instance().compilerForImage(result.compilerImage);
        if (def.verificationState() == VerificationState::Verified) {
            result.compilerVersion = def.actualVersion();
            result.architecture = def.architecture;
            result.imageDigest = def.imageDigest;
        }
    }
    // Results from backends that only fill assemblyOutput still get a typed artifact.
    if (result.artifacts.isEmpty() && !result.assemblyOutput.isEmpty()) {
        CompilationArtifact a;
        a.type = (result.compilerImage.isEmpty() || pane->getLanguage() == Language::Cpp || pane->getLanguage() == Language::C ||
                  pane->getLanguage() == Language::Rust || pane->getLanguage() == Language::Ada)
                     ? ArtifactType::Assembly
                     : (pane->getLanguage() == Language::Java ? ArtifactType::JVM_Bytecode
                        : pane->getLanguage() == Language::Python ? ArtifactType::CPython_Bytecode : ArtifactType::CIL);
        a.name = "output";
        a.content = result.assemblyOutput;
        a.sizeBytes = a.content.toUtf8().size();
        result.artifacts.append(a);
    }

    // Status badge and notice are decided by presentResult(): compiler vs. Docker vs. internal
    // errors, truncation and cleanup warnings are never hidden from the user.
    const ResultPresentation presentation = presentResult(result);
    if (presentation.level == PresentationLevel::Warning) pane->setStatusWarning(presentation.status);
    else pane->setStatus(presentation.status, presentation.level == PresentationLevel::Error);
    pane->setNotice(presentation.notice, presentation.noticeLevel == PresentationLevel::Error);

    pane->setRawAssembly(QString());
    pane->setAssemblyLineMapping({});

    if (result.state == JobState::Cancelled) {
        pane->setResult(CompilationResult());
        updateEditorMarkers();
        return;
    }

    pane->setResult(result);
    showSelectedArtifact(pane);
    updateEditorMarkers();
}

void MainWindow::showSelectedArtifact(CompilerOutputPane* pane) {
    const CompilationResult& result = pane->result();
    const int idx = pane->currentArtifactIndex();

    // A failed job without any artifact or diagnostic: show the raw error text.
    if (result.artifacts.isEmpty() && result.diagnostics.isEmpty()) {
        QString text = result.errorOutput;
        if (text.trimmed().isEmpty()) text = result.rawOutput;
        if (text.trimmed().isEmpty() && !result.success())
            text = "Compilation failed with exit code: " + QString::number(result.exitCode);
        pane->showArtifactText(text, ArtifactType::RawOutput, !result.success());
        return;
    }

    if (idx < 0) {   // diagnostics selected
        pane->showDiagnosticsView();
        return;
    }
    if (idx >= result.artifacts.size()) return;

    const CompilationArtifact& art = result.artifacts[idx];
    if (art.type == ArtifactType::Assembly) {
        pane->setRawAssembly(art.content);

        m_filterSettings.showSegmentDirectives = m_filterShowSegmentDirectives->isChecked();
        m_filterSettings.showDataDirectives = m_filterShowDataDirectives->isChecked();
        m_filterSettings.showCfiDirectives = m_filterShowCfiDirectives->isChecked();
        m_filterSettings.showMetadataLabels = m_filterShowMetadataLabels->isChecked();
        m_filterSettings.showUnusedLabels = m_filterShowUnusedLabels->isChecked();
        m_filterSettings.hideEmptyLabels = m_filterHideEmptyLabels->isChecked();
        m_filterSettings.showDebugInfo = m_filterShowDebugInfo->isChecked();
        m_filterSettings.showComments = m_filterShowComments->isChecked();
        m_filterSettings.demangleIdentifiers = m_filterDemangle->isChecked();

        ProcessedAssemblyResult processed = AssemblyTextProcessor::processAssembly(art.content, m_filterSettings);
        pane->setAssemblyLineMapping(processed.sourceToDisplay);
        pane->setRawToDisplayMapping(processed.rawToDisplay);
        pane->showArtifactText(processed.filteredText, ArtifactType::Assembly, false);
        m_analysisPanel->setPaneAssembly(pane->paneId(), paneTitle(pane), art.content);

        if (m_highlightMode > 0) applyHighlighting();
    } else {
        // Bytecode / IR / raw output: shown as-is, never run through the x86 assembly filters.
        pane->setRawAssembly(QString());
        pane->setAssemblyLineMapping({});
        pane->setRawToDisplayMapping({});
        m_analysisPanel->removePane(pane->paneId());
        const bool isError = !result.success() && art.type == ArtifactType::RawOutput;
        pane->showArtifactText(art.content, art.type, isError);
    }
}

void MainWindow::updateEditorMarkers() {
    if (!m_inputEditor) return;
    QVector<EditorMarker> markers;
    QSet<QString> seen;
    for (CompilerOutputPane* pane : std::as_const(m_outputPanes)) {
        for (const Diagnostic& d : pane->result().diagnostics) {
            if (d.line <= 0) continue;
            EditorMarker m;
            m.line = d.line;
            m.column = d.column;
            m.message = d.message;
            m.severity = d.severity == DiagnosticSeverity::Error ? EditorMarker::Error
                       : d.severity == DiagnosticSeverity::Warning ? EditorMarker::Warning : EditorMarker::Note;
            const QString key = QString("%1:%2:%3").arg(m.line).arg(m.column).arg(m.message);
            if (seen.contains(key)) continue;   // several panes often report the same problem
            seen.insert(key);
            markers.append(m);
        }
    }
    m_inputEditor->setMarkers(markers);
}

bool MainWindow::maybeSave() {
    if (!m_inputEditor->document()->isModified()) {
        return true;
    }

    QString docName = m_currentFile.isEmpty() ? "Untitled" : QFileInfo(m_currentFile).fileName();
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, "Save Changes",
        QString("The document '%1' has unsaved changes.\nDo you want to save your changes?").arg(docName),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (reply == QMessageBox::Save) {
        if (m_currentFile.isEmpty()) {
            onSaveAs();
        } else {
            saveFile(m_currentFile);
        }
        return !m_inputEditor->document()->isModified();
    } else if (reply == QMessageBox::Cancel) {
        return false;
    }

    return true;
}

void MainWindow::onNew() {
    if (!leaveProjectForStandalone()) return;
    if (!maybeSave()) return;
    m_inputEditor->clear();
    m_currentFile.clear();
    m_inputEditor->document()->setModified(false);
    updateWindowTitle();
}

void MainWindow::onSave() {
    if (m_project->isOpen()) {
        const QString path = m_tabPath.value(m_inputEditor);
        if (path.isEmpty()) return;
        if (m_project->projectFilePath().isEmpty()) { onSaveProjectAs(); return; }
        QString err;
        if (!m_project->saveFile(path, &err)) QMessageBox::warning(this, "Save failed", err);
        else { m_inputEditor->document()->setModified(false); updateWindowTitle(); }
        return;
    }
    if (m_currentFile.isEmpty()) {
        onSaveAs();
    } else {
        saveFile(m_currentFile);
    }
}

void MainWindow::onSaveAs() {
    if (m_project->isOpen()) { onSaveProjectAs(); return; }   // "Save As" in a project saves the project
    QString startDir = m_currentFile.isEmpty()
                           ? AppSettings::instance().lastDirectory()
                           : m_currentFile;

    QString fileName = QFileDialog::getSaveFileName(
        this, "Save File", startDir,
        "C++ Files (*.cpp *.h);;C Files (*.c *.h);;Rust Files (*.rs);;Java Files (*.java);;Python Files (*.py);;C# Files (*.cs);;Ada Files (*.adb *.ads);;All Files (*)");

    if (!fileName.isEmpty()) {
        saveFile(fileName);
    }
}

void MainWindow::onLoad() {
    if (!leaveProjectForStandalone()) return;
    if (!maybeSave()) return;

    QString startDir = m_currentFile.isEmpty()
                           ? AppSettings::instance().lastDirectory()
                           : m_currentFile;

    QString fileName = QFileDialog::getOpenFileName(
        this, "Open File", startDir,
        "All Supported (*.cpp *.c *.h *.rs *.java *.py *.cs *.adb *.ads);;C++ Files (*.cpp *.h);;C Files (*.c *.h);;Rust Files (*.rs);;Java Files (*.java);;Python Files (*.py);;C# Files (*.cs);;All Files (*)");

    if (!fileName.isEmpty()) {
        loadFile(fileName);
    }
}

bool MainWindow::saveFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Save Error", "Could not save file: " + filePath);
        return false;
    }

    QTextStream out(&file);
    out << m_inputEditor->toPlainText();
    file.close();

    m_currentFile = filePath;
    m_inputEditor->document()->setModified(false);
    m_recentFilesManager.addRecentFile(filePath);
    AppSettings::instance().setLastDirectory(QFileInfo(filePath).absolutePath());
    updateRecentFilesMenu();
    updateWindowTitle();
    return true;
}

void MainWindow::loadFile(const QString& filePath) {
    // A stand-alone file must never be loaded into the tab of a project file.
    if (!leaveProjectForStandalone()) return;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Open Error", "Could not open file: " + filePath);
        m_recentFilesManager.removeRecentFile(filePath);
        updateRecentFilesMenu();
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    m_inputEditor->setPlainText(content);
    m_currentFile = filePath;
    m_inputEditor->document()->setModified(false);
    m_recentFilesManager.addRecentFile(filePath);
    AppSettings::instance().setLastDirectory(QFileInfo(filePath).absolutePath());
    updateRecentFilesMenu();
    updateWindowTitle();

    // Automatically adapt pane language to file extension if recognized
    QString ext = QFileInfo(filePath).suffix().toLower();
    Language detectedLang = Language::Cpp;
    if (ext == "c") detectedLang = Language::C;
    else if (ext == "rs") detectedLang = Language::Rust;
    else if (ext == "java") detectedLang = Language::Java;
    else if (ext == "py") detectedLang = Language::Python;
    else if (ext == "cs") detectedLang = Language::CSharp;
    else if (ext == "adb" || ext == "ads") detectedLang = Language::Ada;

    if (!m_outputPanes.isEmpty()) {
        m_outputPanes.first()->setLanguage(detectedLang);
    }

    doCompile();
}

void MainWindow::updateRecentFilesMenu() {
    if (!m_recentMenu) return;

    m_recentMenu->clear();
    QStringList recentFiles = m_recentFilesManager.getRecentFiles();

    if (recentFiles.isEmpty()) {
        QAction* noRecent = m_recentMenu->addAction("(no recent files)");
        noRecent->setEnabled(false);
    } else {
        for (const QString& file : recentFiles) {
            QAction* act = m_recentMenu->addAction(file);
            connect(act, &QAction::triggered, this, [this, file]() {
                if (leaveProjectForStandalone() && maybeSave()) {
                    loadFile(file);
                }
            });
        }
        m_recentMenu->addSeparator();
        QAction* clearAct = m_recentMenu->addAction("Clear Recent Files");
        connect(clearAct, &QAction::triggered, this, &MainWindow::onClearRecentFiles);
    }
}

void MainWindow::updateWindowTitle() {
    QString docName;
    if (m_project && m_project->isOpen()) {
        const QString file = m_tabPath.value(m_inputEditor);
        docName = m_project->workspace().name() + (file.isEmpty() ? QString() : " – " + file);
        if (m_project->isDirty()) docName += "*";
    } else {
        docName = m_currentFile.isEmpty() ? "Untitled" : QFileInfo(m_currentFile).fileName();
        if (m_inputEditor && m_inputEditor->document()->isModified()) docName += "*";
    }
    setWindowTitle(QString("OCTOS - %1 - Offline Compiler Tool / Assembly Analyzer").arg(docName));
    if (m_inputEditor) updateTabTitle(m_inputEditor);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            if (leaveProjectForStandalone() && maybeSave()) {
                loadFile(filePath);
            }
        }
    }
}

void MainWindow::onManageDockerCompilers() {
    CompilerManagerDialog dialog(m_dockerManager, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshDockerCompilerList();
    }
}

void MainWindow::onInsertSnippet() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;

    QString snippetName = action->text();
    CodeSnippet snippet = m_snippetManager.getSnippet(snippetName);

    if (!snippet.code.isEmpty()) {
        if (m_inputEditor->toPlainText().trimmed().isEmpty()) {
            m_inputEditor->setPlainText(snippet.code);
        } else {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Insert Snippet",
                "Do you want to replace current code (Yes) or append snippet (No)?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (reply == QMessageBox::Yes) {
                m_inputEditor->setPlainText(snippet.code);
            } else if (reply == QMessageBox::No) {
                QTextCursor cursor = m_inputEditor->textCursor();
                cursor.movePosition(QTextCursor::End);
                cursor.insertText("\n\n" + snippet.code);
                m_inputEditor->setTextCursor(cursor);
            }
        }

        doCompile();
    }
}

void MainWindow::onChangeTheme(int index) {
    if (index < 0 || index >= Theme::getAvailableThemes().size()) index = 0;
    m_currentTheme = static_cast<ThemeType>(index);
    AppSettings::instance().setThemeIndex(index);
    Theme::setCurrent(m_currentTheme);
    qApp->setStyleSheet(Theme::getStylesheet(m_currentTheme));

    // Everything that does not follow the stylesheet reads its colors from the theme tokens.
    for (auto it = m_highlighters.constBegin(); it != m_highlighters.constEnd(); ++it) {
        if (auto* h = dynamic_cast<CppHighlighter*>(it.value())) h->applyTheme();
        else if (auto* r = dynamic_cast<RustHighlighter*>(it.value())) r->applyTheme();
        else if (auto* p = dynamic_cast<PythonHighlighter*>(it.value())) p->applyTheme();
        it.key()->applyTheme();
    }
    if (m_analysisPanel) m_analysisPanel->setProperty("themeGeneration", Theme::generation());
    for (CompilerOutputPane* pane : std::as_const(m_outputPanes)) pane->applyTheme();
    updateEditorMarkers();
}

void MainWindow::onClearRecentFiles() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clear Recent Files", "Are you sure you want to clear all recent files?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_recentFilesManager.clearRecentFiles();
        updateRecentFilesMenu();
    }
}

void MainWindow::onRefilterOutput() {
    m_filterSettings.showSegmentDirectives = m_filterShowSegmentDirectives->isChecked();
    m_filterSettings.showDataDirectives = m_filterShowDataDirectives->isChecked();
    m_filterSettings.showCfiDirectives = m_filterShowCfiDirectives->isChecked();
    m_filterSettings.showMetadataLabels = m_filterShowMetadataLabels->isChecked();
    m_filterSettings.showUnusedLabels = m_filterShowUnusedLabels->isChecked();
    m_filterSettings.hideEmptyLabels = m_filterHideEmptyLabels->isChecked();
    m_filterSettings.showDebugInfo = m_filterShowDebugInfo->isChecked();
    m_filterSettings.showComments = m_filterShowComments->isChecked();
    m_filterSettings.demangleIdentifiers = m_filterDemangle->isChecked();

    for (CompilerOutputPane* pane : m_outputPanes) {
        const QString rawAssembly = pane->getRawAssembly();
        if (rawAssembly.isEmpty()) {
            continue;
        }

        ProcessedAssemblyResult processed =
            AssemblyTextProcessor::processAssembly(rawAssembly, m_filterSettings);
        pane->setAssemblyLineMapping(processed.sourceToDisplay);
        pane->setOutputText(processed.filteredText, false);
    }

    if (m_highlightMode > 0) {
        applyHighlighting();
    }
}

void MainWindow::onToggleHighlightMode() {
    m_highlightMode = (m_highlightMode + 1) % 3;

    switch (m_highlightMode) {
        case 0:
            m_highlightToggle->setText("Highlight: Off");
            break;
        case 1:
            m_highlightToggle->setText("Highlight: All");
            break;
        case 2:
            m_highlightToggle->setText("Highlight: Selection");
            break;
    }

    applyHighlighting();
}

void MainWindow::onSourceCursorPositionChanged() {
    if (m_highlightMode == 2) {
        applyHighlighting();
    }
}

void MainWindow::applyHighlighting() {
    if (m_highlightMode == 0) {
        for (CompilerOutputPane* pane : m_outputPanes) {
            QList<QTextEdit::ExtraSelection> selections;
            pane->getEditor()->setExtraSelections(selections);
        }
        return;
    }

    int currentSourceLine = m_inputEditor->textCursor().blockNumber() + 1;

    for (CompilerOutputPane* pane : m_outputPanes) {
        QList<QTextEdit::ExtraSelection> selections;
        QMap<int, QVector<int>> lineMapping = pane->getLineMapping();

        if (m_highlightMode == 1) {
            QList<int> sourceLines = lineMapping.keys();
            int totalLines = sourceLines.size();

            for (int i = 0; i < sourceLines.size(); ++i) {
                int sourceLine = sourceLines[i];
                QVector<int> asmLines = lineMapping[sourceLine];
                QColor highlightColor = getHighlightColor(i, totalLines);

                for (int asmLine : asmLines) {
                    QTextDocument* doc = pane->getEditor()->document();
                    if (asmLine >= 0 && asmLine < doc->blockCount()) {
                        QTextBlock block = doc->findBlockByLineNumber(asmLine);
                        if (block.isValid()) {
                            QTextEdit::ExtraSelection selection;
                            selection.format.setBackground(highlightColor);
                            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
                            selection.cursor = QTextCursor(block);
                            selection.cursor.clearSelection();
                            selections.append(selection);
                        }
                    }
                }
            }
        } else if (m_highlightMode == 2) {
            if (lineMapping.contains(currentSourceLine)) {
                QVector<int> asmLines = lineMapping[currentSourceLine];
                QColor highlightColor = Theme::currentTokens().sourceHighlight;

                for (int asmLine : asmLines) {
                    QTextDocument* doc = pane->getEditor()->document();
                    if (asmLine >= 0 && asmLine < doc->blockCount()) {
                        QTextBlock block = doc->findBlockByLineNumber(asmLine);
                        if (block.isValid()) {
                            QTextEdit::ExtraSelection selection;
                            selection.format.setBackground(highlightColor);
                            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
                            selection.cursor = QTextCursor(block);
                            selection.cursor.clearSelection();
                            selections.append(selection);
                        }
                    }
                }
            }
        }

        pane->getEditor()->setExtraSelections(selections);
    }
}

QColor MainWindow::getHighlightColor(int index, int total) {
    if (total <= 0) {
        total = 1;
    }
    float hue = static_cast<float>(index) / static_cast<float>(total);
    return QColor::fromHsvF(hue, 0.6, 0.9, 0.3);
}

void MainWindow::onCompareDiff() {
    if (m_outputPanes.size() < 2) {
        return;
    }

    QVector<DiffPaneInfo> paneInfos;
    for (int i = 0; i < m_outputPanes.size(); ++i) {
        CompilerOutputPane* pane = m_outputPanes[i];
        DiffPaneInfo info;
        info.paneId = pane->paneId();
        info.compilerName = pane->getSelectedCompiler();
        info.compilerFlags = pane->getCompilerFlags();
        info.language = pane->getLanguage();
        info.syntaxFlavor = pane->getSyntaxFlavor();
        info.content = pane->getEditor()->toPlainText();
        // Architecture is not tracked per pane yet: the dialog detects it from the content.

        Language lang = pane->getLanguage();
        if (lang == Language::Java) {
            info.artifactType = ArtifactType::JVM_Bytecode;
        } else if (lang == Language::Python) {
            info.artifactType = ArtifactType::CPython_Bytecode;
        } else if (lang == Language::CSharp) {
            info.artifactType = ArtifactType::CIL;
        } else {
            info.artifactType = ArtifactType::Assembly;
        }

        QString syn = LanguageStrategyRegistry::instance().supportsSyntaxFlavor(lang)
                          ? (info.syntaxFlavor == SyntaxFlavor::Intel ? "Intel" : "AT&T")
                          : "";
        info.title = QString("Pane %1: %2 (%3 %4)")
                         .arg(QString::number(i + 1),
                              info.compilerName.isEmpty() ? "No Compiler" : info.compilerName,
                              languageToString(lang),
                              syn)
                         .trimmed();

        paneInfos.append(info);
    }

    DiffDialog dialog(paneInfos, 0, 1, this);
    dialog.exec();
}

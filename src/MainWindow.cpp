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
#include "highlighters/RustHighlighter.h"
#include "highlighters/PythonHighlighter.h"
#include "managers/RecentFilesManager.h"
#include "managers/SnippetManager.h"
#include "utils/OpcodeReference.h"
#include "strategies/LanguageStrategyRegistry.h"
#include "ui/DiffDialog.h"
#include "ui/Theme.h"

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
    m_lineNumberArea = new LineNumberArea(this);

    QFont monoFont("Monospace", 11);
    monoFont.setStyleHint(QFont::TypeWriter);
    setFont(monoFont);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(40);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    return 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void CodeEditor::enableOpcodeTooltips(bool enable) {
    m_opcodeTooltipsEnabled = enable;
    setMouseTracking(enable);
    if (viewport()) {
        viewport()->setMouseTracking(enable);
    }
}

bool CodeEditor::event(QEvent* event) {
    if (m_opcodeTooltipsEnabled && event->type() == QEvent::ToolTip) {
        auto* helpEvent = static_cast<QHelpEvent*>(event);
        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        cursor.select(QTextCursor::WordUnderCursor);
        QString word = cursor.selectedText().trimmed().toLower();

        if (!word.isEmpty() && OpcodeReference::instance().hasOpcode(word)) {
            OpcodeInfo info = OpcodeReference::instance().getOpcodeInfo(word);
            QToolTip::showText(helpEvent->globalPos(), info.toFormattedTooltip(), this);
            return true;
        } else {
            QToolTip::hideText();
            event->ignore();
            return true;
        }
    }
    return QPlainTextEdit::event(event);
}

void CodeEditor::keyPressEvent(QKeyEvent* event) {
    const QChar ch = event->text().isEmpty() ? QChar() : event->text().at(0);
    const QMap<QChar, QChar> pairs = {{'(', ')'}, {'[', ']'}, {'{', '}'}, {'"', '"'}, {'\'', '\''}};

    if (pairs.contains(ch)) {
        QPlainTextEdit::keyPressEvent(event);
        QChar closing = pairs.value(ch);
        insertPlainText(QString(closing));
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::Left);
        setTextCursor(c);
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor("#2d2d30");
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#1e1e1e"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#858585"));
            painter.drawText(0, top, m_lineNumberArea->width() - 3, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        ++blockNumber;
    }
}

QSize LineNumberArea::sizeHint() const { return QSize(m_codeEditor->lineNumberAreaWidth(), 0); }

void LineNumberArea::paintEvent(QPaintEvent* event) {
    m_codeEditor->lineNumberAreaPaintEvent(event);
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_currentTheme(ThemeType::Dark) {
    setAcceptDrops(true);

    m_dockerManager = new DockerCompilerManager(this);
    connect(m_dockerManager, &DockerCompilerManager::compilationResultReceived, this,
            &MainWindow::onCompilationResultReceived);

    setupUi();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(AppSettings::instance().debounceIntervalMs());

    connect(m_inputEditor, &QPlainTextEdit::textChanged, this, &MainWindow::onInputTextChanged);
    connect(m_inputEditor->document(), &QTextDocument::modificationChanged, this,
            &MainWindow::onDocumentModificationChanged);
    connect(m_inputEditor, &QPlainTextEdit::cursorPositionChanged, this,
            &MainWindow::onSourceCursorPositionChanged);
    connect(m_debounceTimer, &QTimer::timeout, this, &MainWindow::doCompile);

    qApp->setStyle("Fusion");
    int savedTheme = AppSettings::instance().themeIndex();
    onChangeTheme(savedTheme);

    refreshDockerCompilerList();
    updateWindowTitle();
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
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
    QAction* autoCompileAction = toolsMenu->addAction("Auto-Compile on change");
    autoCompileAction->setCheckable(true);
    autoCompileAction->setChecked(AppSettings::instance().autoCompile());
    connect(autoCompileAction, &QAction::toggled, this, [](bool enabled) {
        AppSettings::instance().setAutoCompile(enabled);
    });

    m_splitter = new QSplitter(Qt::Horizontal);

    m_sourceEditorContainer = new QWidget();
    QVBoxLayout* sourceLayout = new QVBoxLayout(m_sourceEditorContainer);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    sourceLayout->setSpacing(0);

    QWidget* sourceToolbar = new QWidget();
    sourceToolbar->setStyleSheet(
        "QWidget { background-color: #2d2d30; border-bottom: 1px solid #3e3e42; }");
    QHBoxLayout* sourceToolbarLayout = new QHBoxLayout(sourceToolbar);
    sourceToolbarLayout->setContentsMargins(8, 4, 8, 4);
    sourceToolbarLayout->setSpacing(8);

    QLabel* sourceLabel = new QLabel("Source Code");
    sourceLabel->setStyleSheet("color: #cccccc; font-weight: bold;");
    sourceToolbarLayout->addWidget(sourceLabel);
    sourceToolbarLayout->addStretch();

    m_highlightToggle = new QPushButton("Highlight: Off");
    m_highlightToggle->setToolTip("Toggle highlighting: Off → Highlight All → Highlight Selection");
    m_highlightToggle->setStyleSheet(R"(
        QPushButton {
            padding: 4px 12px;
            background-color: #3e3e42;
            border: 1px solid #555;
            border-radius: 3px;
            color: #ccc;
        }
        QPushButton:hover {
            background-color: #4e4e52;
        }
    )");
    connect(m_highlightToggle, &QPushButton::clicked, this, &MainWindow::onToggleHighlightMode);
    sourceToolbarLayout->addWidget(m_highlightToggle);

    m_addPaneButton = new QPushButton("+ Compare");
    m_addPaneButton->setStyleSheet(R"(
        QPushButton {
            padding: 4px 12px;
            background-color: #0e639c;
            border: none;
            border-radius: 3px;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1177bb;
        }
    )");
    connect(m_addPaneButton, &QPushButton::clicked, this, &MainWindow::onAddComparisonPane);
    sourceToolbarLayout->addWidget(m_addPaneButton);

    m_compareDiffButton = new QPushButton("Diff View");
    m_compareDiffButton->setToolTip("View side-by-side assembly diff between panels");
    m_compareDiffButton->setEnabled(false);
    m_compareDiffButton->setStyleSheet(R"(
        QPushButton {
            padding: 4px 12px;
            background-color: #2e6c52;
            border: none;
            border-radius: 3px;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #388463;
        }
        QPushButton:disabled {
            background-color: #333333;
            color: #777777;
        }
    )");
    connect(m_compareDiffButton, &QPushButton::clicked, this, &MainWindow::onCompareDiff);
    sourceToolbarLayout->addWidget(m_compareDiffButton);

    m_closeOthersButton = new QPushButton("Close Others");
    m_closeOthersButton->setStyleSheet(R"(
        QPushButton {
            padding: 4px 12px;
            background-color: #444;
            border: 1px solid #555;
            border-radius: 3px;
            color: #ccc;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #555;
        }
    )");
    connect(m_closeOthersButton, &QPushButton::clicked, this, &MainWindow::onCloseOtherPanes);
    sourceToolbarLayout->addWidget(m_closeOthersButton);

    sourceLayout->addWidget(sourceToolbar);

    m_inputEditor = new CodeEditor();
    m_inputEditor->setPlaceholderText("\n Enter your code here...");

    QFont monoFont("Monospace", 11);
    monoFont.setStyleHint(QFont::TypeWriter);
    m_inputEditor->setFont(monoFont);
    m_inputEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_inputEditor->setTabStopDistance(40);
    updateSourceHighlighter(Language::Cpp);

    sourceLayout->addWidget(m_inputEditor);

    m_outputSplitter = new QSplitter(Qt::Horizontal);

    CompilerOutputPane* initialPane = new CompilerOutputPane(m_dockerManager);
    connect(initialPane, &CompilerOutputPane::closeRequested, this,
            [this, initialPane]() { onRemovePane(initialPane); });
    connect(initialPane, &CompilerOutputPane::compilerChanged, this,
            &MainWindow::onPaneSettingsChanged);
    connect(initialPane, &CompilerOutputPane::flagsChanged, this,
            &MainWindow::onPaneSettingsChanged);
    connect(initialPane, &CompilerOutputPane::languageChanged, this,
            [this, initialPane](Language lang) {
                onPaneLanguageChanged(lang, initialPane);
                onPaneSettingsChanged();
            });
    connect(initialPane, &CompilerOutputPane::syntaxChanged, this,
            &MainWindow::onPaneSettingsChanged);

    m_outputPanes.append(initialPane);
    m_paneLookup[initialPane->paneId()] = initialPane;
    m_outputSplitter->addWidget(initialPane);

    AsmHighlighter* highlighter = new AsmHighlighter(initialPane->getEditor()->document());
    m_asmHighlighters.append(highlighter);

    initialPane->setCloseButtonVisible(false); // First pane cannot be closed if alone

    m_splitter->addWidget(m_sourceEditorContainer);
    m_splitter->addWidget(m_outputSplitter);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({600, 600});

    mainLayout->addWidget(m_splitter, 1);

    onRefilterOutput();
}

void MainWindow::onAddComparisonPane() {
    CompilerOutputPane* newPane = new CompilerOutputPane(m_dockerManager);
    connect(newPane, &CompilerOutputPane::closeRequested, this,
            [this, newPane]() { onRemovePane(newPane); });
    connect(newPane, &CompilerOutputPane::compilerChanged, this,
            &MainWindow::onPaneSettingsChanged);
    connect(newPane, &CompilerOutputPane::flagsChanged, this, &MainWindow::onPaneSettingsChanged);
    connect(newPane, &CompilerOutputPane::languageChanged, this, [this, newPane](Language lang) {
        onPaneLanguageChanged(lang, newPane);
        onPaneSettingsChanged();
    });
    connect(newPane, &CompilerOutputPane::syntaxChanged, this, &MainWindow::onPaneSettingsChanged);

    m_outputPanes.append(newPane);
    m_paneLookup[newPane->paneId()] = newPane;
    m_outputSplitter->addWidget(newPane);

    refreshDockerCompilerList();

    AsmHighlighter* highlighter = new AsmHighlighter(newPane->getEditor()->document());
    m_asmHighlighters.append(highlighter);

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

        if (index < m_asmHighlighters.size()) {
            delete m_asmHighlighters[index];
            m_asmHighlighters.removeAt(index);
        }

        if (m_outputPanes.size() == 1) {
            m_outputPanes.first()->setCloseButtonVisible(false);
        }
        if (m_compareDiffButton) {
            m_compareDiffButton->setEnabled(m_outputPanes.size() >= 2);
        }

        pane->deleteLater();
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
        QString imageName = image.fullName.toLower();

        switch (lang) {
            case Language::Cpp:
            case Language::C:
                if (imageName.contains("gcc") || imageName.contains("clang")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::Rust:
                if (imageName.contains("rust")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::Java:
                if (imageName.contains("openjdk") || imageName.contains("eclipse-temurin") ||
                    imageName.contains("temurin") || imageName.contains("java")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::Python:
                if (imageName.contains("python")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::CSharp:
                if (imageName.contains("mono") || imageName.contains("csharp")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::Ada:
                if (imageName.contains("gcc")) {
                    filtered.append(image.fullName);
                }
                break;
        }
    }

    if (filtered.isEmpty()) {
        for (const DockerImage& image : installedImages) {
            filtered.append(image.fullName);
        }
    }

    return filtered;
}

void MainWindow::updateSourceHighlighter(Language lang) {
    if (m_sourceHighlighter) {
        delete m_sourceHighlighter;
        m_sourceHighlighter = nullptr;
    }

    if (!m_inputEditor) {
        return;
    }

    switch (lang) {
        case Language::Rust:
            m_sourceHighlighter = new RustHighlighter(m_inputEditor->document());
            break;
        case Language::Python:
            m_sourceHighlighter = new PythonHighlighter(m_inputEditor->document());
            break;
        case Language::Cpp:
        case Language::C:
        case Language::Java:
        case Language::CSharp:
        case Language::Ada:
        default:
            m_sourceHighlighter = new CppHighlighter(m_inputEditor->document());
            break;
    }
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
        req.timeoutMs = 30000;

        pane->setStatus("Compiling...");
        quint64 jobId = m_dockerManager->submitRequest(req);
        pane->setCurrentJobId(static_cast<int>(jobId));
    }
}

void MainWindow::onCompilationResultReceived(const CompilationResult& result) {
    CompilerOutputPane* pane = m_paneLookup.value(result.paneId, nullptr);
    if (!pane) {
        return;
    }

    // Verify revision: discard outdated job results
    if (result.revision < m_inputRevision) {
        return;
    }

    if (result.state == JobState::Cancelled) {
        pane->setStatus("Cancelled");
        return;
    }

    if (result.state == JobState::TimedOut) {
        pane->setStatus("Timeout", true);
        pane->setRawAssembly(QString());
        pane->setAssemblyLineMapping({});
        pane->setOutputText(result.errorOutput, true);
        return;
    }

    if (result.success()) {
        pane->setStatus("Success");
        QString rawAssembly = result.assemblyOutput;
        pane->setRawAssembly(rawAssembly);

        m_filterSettings.showSegmentDirectives = m_filterShowSegmentDirectives->isChecked();
        m_filterSettings.showDataDirectives = m_filterShowDataDirectives->isChecked();
        m_filterSettings.showCfiDirectives = m_filterShowCfiDirectives->isChecked();
        m_filterSettings.showMetadataLabels = m_filterShowMetadataLabels->isChecked();
        m_filterSettings.showUnusedLabels = m_filterShowUnusedLabels->isChecked();
        m_filterSettings.hideEmptyLabels = m_filterHideEmptyLabels->isChecked();
        m_filterSettings.showDebugInfo = m_filterShowDebugInfo->isChecked();
        m_filterSettings.showComments = m_filterShowComments->isChecked();
        m_filterSettings.demangleIdentifiers = m_filterDemangle->isChecked();

        ProcessedAssemblyResult processed =
            AssemblyTextProcessor::processAssembly(rawAssembly, m_filterSettings);
        pane->setAssemblyLineMapping(processed.sourceToDisplay);
        pane->setOutputText(processed.filteredText, false);

        if (m_highlightMode > 0) {
            applyHighlighting();
        }
    } else {
        pane->setStatus("Failed", true);
        QString errorText = result.errorOutput;
        if (errorText.trimmed().isEmpty()) {
            errorText = result.rawOutput;
        }
        if (errorText.trimmed().isEmpty()) {
            errorText = "Compilation failed with exit code: " + QString::number(result.exitCode);
        }

        pane->setRawAssembly(QString());
        pane->setAssemblyLineMapping({});
        pane->setOutputText(errorText, true);
    }
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
    if (!maybeSave()) return;
    m_inputEditor->clear();
    m_currentFile.clear();
    m_inputEditor->document()->setModified(false);
    updateWindowTitle();
}

void MainWindow::onSave() {
    if (m_currentFile.isEmpty()) {
        onSaveAs();
    } else {
        saveFile(m_currentFile);
    }
}

void MainWindow::onSaveAs() {
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
                if (maybeSave()) {
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
    QString docName = m_currentFile.isEmpty() ? "Untitled" : QFileInfo(m_currentFile).fileName();
    if (m_inputEditor && m_inputEditor->document()->isModified()) {
        docName += "*";
    }
    setWindowTitle(QString("OCTOS - %1 - Offline Compiler Tool / Assembly Analyzer").arg(docName));
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
            if (maybeSave()) {
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
    m_currentTheme = static_cast<ThemeType>(index);
    AppSettings::instance().setThemeIndex(index);
    QString stylesheet = Theme::getStylesheet(m_currentTheme);
    qApp->setStyleSheet(stylesheet);
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
                QColor highlightColor = QColor(14, 99, 156, 80);

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

    CompilerOutputPane* paneA = m_outputPanes[0];
    CompilerOutputPane* paneB = m_outputPanes[1];

    QString nameA = QString("Pane 1 (%1 - %2)")
                        .arg(paneA->getSelectedCompiler(), paneA->getCompilerFlags());
    QString nameB = QString("Pane 2 (%1 - %2)")
                        .arg(paneB->getSelectedCompiler(), paneB->getCompilerFlags());

    QString asmA = paneA->getEditor()->toPlainText();
    QString asmB = paneB->getEditor()->toPlainText();

    DiffDialog dialog(nameA, asmA, nameB, asmB, this);
    dialog.exec();
}

# 1 "./src/MainWindow.cpp"
#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
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
#include <QSpacerItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

#include "managers/RecentFilesManager.h"
#include "managers/SnippetManager.h"
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
      m_inputEditor(nullptr),
      m_sourceEditorContainer(nullptr),
      m_splitter(nullptr),
      m_outputSplitter(nullptr),
      m_addPaneButton(nullptr),
      m_highlightToggle(nullptr),
      m_highlightMode(0),
      m_filterShowSegmentDirectives(nullptr),
      m_filterShowDataDirectives(nullptr),
      m_filterShowCfiDirectives(nullptr),
      m_filterShowMetadataLabels(nullptr),
      m_filterShowUnusedLabels(nullptr),
      m_filterShowDebugInfo(nullptr),
      m_filterShowComments(nullptr),
      m_filterDemangle(nullptr),
      m_debounceTimer(nullptr),
      m_dockerManager(nullptr),
      m_cppHighlighter(nullptr),
      m_currentTheme(ThemeType::Dark) {
    setAcceptDrops(true);

    m_dockerManager = new DockerCompilerManager(this);
    connect(m_dockerManager, &DockerCompilerManager::compilationFinished, this,
            &MainWindow::onCompilationFinished);

    setupUi();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(300);

    connect(m_inputEditor, &QPlainTextEdit::textChanged, this, &MainWindow::onInputTextChanged);
    connect(m_inputEditor, &QPlainTextEdit::cursorPositionChanged, this,
            &MainWindow::onSourceCursorPositionChanged);
    connect(m_debounceTimer, &QTimer::timeout, this, &MainWindow::doCompile);

    qApp->setStyle("Fusion");
    qApp->setStyleSheet(Theme::getDarkStylesheet());

    refreshDockerCompilerList();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QToolBar* toolBar = addToolBar("Main Toolbar");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(16, 16));
    toolBar->setStyleSheet("QToolBar { border-bottom: 1px solid #3e3e42; padding: 4px; }");

    QMenu* toolsMenu = new QMenu("Tools", this);
    QAction* dockerCompilersAction = toolsMenu->addAction("Docker Compiler Manager");
    connect(dockerCompilersAction, &QAction::triggered, this, &MainWindow::onManageDockerCompilers);

    toolsMenu->addSeparator();
    QMenu* snippetsMenu = toolsMenu->addMenu("Insert Snippet");
    QStringList snippetNames = m_snippetManager.getAllSnippetNames();
    for (const QString& name : snippetNames) {
        QAction* snippetAction = snippetsMenu->addAction(name);
        connect(snippetAction, &QAction::triggered, this, [this]() { onInsertSnippet(); });
    }

    toolsMenu->addSeparator();
    QMenu* themeMenu = toolsMenu->addMenu("Theme");
    QStringList themes = Theme::getAvailableThemes();
    for (int i = 0; i < themes.size(); ++i) {
        QAction* themeAction = themeMenu->addAction(themes[i]);
        themeAction->setData(i);
        connect(themeAction, &QAction::triggered, this, [this, i]() { onChangeTheme(i); });
    }



    QMenu* filtersMenu = new QMenu("Filters", this);

    m_filterShowSegmentDirectives = new QAction("Show Segment Directives", this);
    m_filterShowSegmentDirectives->setCheckable(true);
    m_filterShowSegmentDirectives->setChecked(m_filterSettings.showSegmentDirectives);
    connect(m_filterShowSegmentDirectives, &QAction::triggered, this,
            &MainWindow::onRefilterOutput);
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



    QMenu* fileMenu = new QMenu("File", this);
    QAction* newAction = fileMenu->addAction("New");
    newAction->setShortcut(Qt::CTRL | Qt::Key_N);
    connect(newAction, &QAction::triggered, this, [this]() {
        m_inputEditor->clear();
        m_currentFile = "";
    });

    QAction* openAction = fileMenu->addAction("Open");
    openAction->setShortcut(Qt::CTRL | Qt::Key_O);
    connect(openAction, &QAction::triggered, this, &MainWindow::onLoad);

    fileMenu->addSeparator();
    QMenu* recentMenu = fileMenu->addMenu("Recent Files");
    QStringList recentFiles = m_recentFilesManager.getRecentFiles();
    if (recentFiles.isEmpty()) {
        QAction* noRecentAction = recentMenu->addAction("(no recent files)");
        noRecentAction->setEnabled(false);
    } else {
        for (const QString& file : recentFiles) {
            QAction* fileAction = recentMenu->addAction(file);
            connect(fileAction, &QAction::triggered, this, [this, file]() { loadFile(file); });
        }
        recentMenu->addSeparator();
        QAction* clearAction = recentMenu->addAction("Clear Recent Files");
        connect(clearAction, &QAction::triggered, this, &MainWindow::onClearRecentFiles);
    }

    fileMenu->addSeparator();
    QAction* saveAction = fileMenu->addAction("Save");
    saveAction->setShortcut(Qt::CTRL | Qt::Key_S);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSave);
    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(filtersMenu);
    menuBar()->addMenu(toolsMenu);

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
    m_cppHighlighter = new CppHighlighter(m_inputEditor->document());

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
    m_outputSplitter->addWidget(initialPane);

    AsmHighlighter* highlighter = new AsmHighlighter(initialPane->getEditor()->document());
    m_asmHighlighters.append(highlighter);

    initialPane->setCloseButtonVisible(true);

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
    m_outputSplitter->addWidget(newPane);

    refreshDockerCompilerList();

    AsmHighlighter* highlighter = new AsmHighlighter(newPane->getEditor()->document());
    m_asmHighlighters.append(highlighter);

    newPane->setCloseButtonVisible(true);

    QList<int> sizes;
    int totalWidth = m_outputSplitter->width();
    int paneWidth = totalWidth / m_outputPanes.size();
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
}

void MainWindow::onRemovePane(CompilerOutputPane* pane) {
    int index = m_outputPanes.indexOf(pane);
    if (index >= 0) {
        m_outputPanes.removeAt(index);

        if (index < m_asmHighlighters.size()) {
            delete m_asmHighlighters[index];
            m_asmHighlighters.removeAt(index);
        }

        for (auto it = m_jobToPaneMap.begin(); it != m_jobToPaneMap.end();) {
            if (it.value() == pane) {
                m_dockerManager->cancelJob(it.key());
                it = m_jobToPaneMap.erase(it);
            } else {
                ++it;
            }
        }

        pane->deleteLater();
    }
}

void MainWindow::refreshDockerCompilerList() {
    QVector<DockerImage> installedImages = m_dockerManager->listInstalledImages();

    QStringList compilerList;
    for (const DockerImage& image : installedImages) {
        compilerList.append(image.fullName);
    }

    for (CompilerOutputPane* pane : m_outputPanes) {
        pane->setCompilerList(compilerList);
    }
}

QStringList MainWindow::getCompilersForLanguage(Language lang) {
    QVector<DockerImage> installedImages = m_dockerManager->listInstalledImages();
    QStringList filtered;

    for (const DockerImage& image : installedImages) {
        QString imageName = image.fullName.toLower();

        // Map languages to compiler image keywords
        switch (lang) {
            case Language::Cpp:
            case Language::C:
                // C/C++ compilers: gcc, clang, g++
                if (imageName.contains("gcc") || imageName.contains("clang")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::Rust:
                // Rust compiler: rustc
                if (imageName.contains("rust")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::Java:
                // Java compilers: openjdk, eclipse-temurin
                if (imageName.contains("openjdk") || imageName.contains("eclipse-temurin") ||
                    imageName.contains("temurin") || imageName.contains("java")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::Python:
                // Python interpreter
                if (imageName.contains("python")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::CSharp:
                // C# compiler: mono, csc
                if (imageName.contains("mono") || imageName.contains("csharp")) {
                    filtered.append(image.fullName);
                }
                break;
            case Language::Ada:
                // Ada compiler: gcc (has gnat built-in)
                if (imageName.contains("gcc")) {
                    filtered.append(image.fullName);
                }
                break;
        }
    }

    // If no specific compilers found, return all (fallback)
    if (filtered.isEmpty()) {
        for (const DockerImage& image : installedImages) {
            filtered.append(image.fullName);
        }
    }

    return filtered;
}

void MainWindow::onPaneLanguageChanged(Language lang, CompilerOutputPane* pane) {
    // Update compiler list for this pane based on selected language
    QStringList compilers = getCompilersForLanguage(lang);
    pane->setCompilerList(compilers);
}

void MainWindow::updateCompilerLists() { refreshDockerCompilerList(); }

void MainWindow::onInputTextChanged() { m_debounceTimer->start(); }

void MainWindow::onPaneSettingsChanged() { m_debounceTimer->start(); }

void MainWindow::doCompile() { compileAllPanes(); }

void MainWindow::compileAllPanes() {
    QString source = m_inputEditor->toPlainText();

    if (source.trimmed().isEmpty()) {
        for (CompilerOutputPane* pane : m_outputPanes) {
            pane->setOutputText("No source code to compile");
        }
        return;
    }

    int jobsSubmitted = 0;
    for (CompilerOutputPane* pane : m_outputPanes) {
        QString compiler = pane->getSelectedCompiler();
        QString flags = pane->getCompilerFlags();
        Language lang = pane->getLanguage();
        SyntaxFlavor syntax = pane->getSyntaxFlavor();

        if (compiler.isEmpty()) {
            pane->setOutputText("No Docker Images found - Please open Compiler Manager");
            continue;
        }

        QString language = "cpp";
        switch (lang) {
            case Language::C:
                language = "c";
                break;
            case Language::Cpp:
                language = "cpp";
                break;
            case Language::Rust:
                language = "rust";
                break;
            case Language::Java:
                language = "java";
                break;
            case Language::Python:
                language = "python";
                break;
            case Language::CSharp:
                language = "csharp";
                break;
            case Language::Ada:
                language = "ada";
                break;
        }

        QString fullFlags = flags;
        if (syntax == SyntaxFlavor::Intel) {
            if (!fullFlags.contains("-masm=")) {
                fullFlags += " -masm=intel";
            }
        } else if (syntax == SyntaxFlavor::ATT) {
            if (!fullFlags.contains("-masm=")) {
                fullFlags += " -masm=att";
            }
        }

        if (!fullFlags.contains("-g")) {
            fullFlags += " -g";
        }

        int jobId = m_dockerManager->submitCompilationJob(source, compiler, fullFlags, language);
        m_jobToPaneMap[jobId] = pane;
        pane->setCurrentJobId(jobId);
        pane->setOutputText("Compiling...");

        jobsSubmitted++;
    }
}

void MainWindow::onCompilationFinished(int jobId, const CompilationOutput& output) {
    if (!m_jobToPaneMap.contains(jobId)) {
        return;
    }

    CompilerOutputPane* pane = m_jobToPaneMap[jobId];
    m_jobToPaneMap.remove(jobId);

    if (output.success) {
        QString rawAssembly = output.stdout_;

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
        QString errorText;

        if (!output.stderr_.isEmpty()) {
            errorText = output.stderr_;
        }

        if (!output.stdout_.isEmpty()) {
            if (!errorText.isEmpty()) {
                errorText += "\n";
            }
            errorText += output.stdout_;
        }

        if (errorText.isEmpty()) {
            errorText = "Compilation failed with exit code: " + QString::number(output.exitCode);
        }

        pane->setRawAssembly(QString());
        pane->setAssemblyLineMapping({});
        pane->setOutputText(errorText, false);
    }
}

void MainWindow::onSave() {
    QString fileName =
        QFileDialog::getSaveFileName(this, "Save File", m_currentFile,
                                     "C++ Files (*.cpp *.h);;C Files (*.c *.h);;Rust Files "
                                     "(*.rs);;C# Files (*.cs);;Java Files (*.java);;All Files (*)");

    if (!fileName.isEmpty()) {
        saveFile(fileName);
    }
}

void MainWindow::onLoad() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open File", m_currentFile,
        "All Files (*);;C++ Files (*.cpp *.h);;C Files (*.c *.h);;Rust Files (*.rs);;C# Files "
        "(*.cs);;Java Files (*.java)");

    if (!fileName.isEmpty()) {
        loadFile(fileName);
    }
}

void MainWindow::saveFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not save file: " + filePath);
        return;
    }

    QTextStream out(&file);
    out << m_inputEditor->toPlainText();
    file.close();

    m_currentFile = filePath;
    QFileInfo fileInfo(filePath);
}

void MainWindow::loadFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file: " + filePath);
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    m_inputEditor->setPlainText(content);
    m_currentFile = filePath;
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
            loadFile(filePath);
        }
    }
}

void MainWindow::onManageDockerCompilers() {
    CompilerManagerDialog dialog(m_dockerManager, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshDockerCompilerList();
    }
}

void MainWindow::onOpenRecentFile() {}

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
                "Do you want to replace the current code or append the snippet?",
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
    QString stylesheet = Theme::getStylesheet(m_currentTheme);
    qApp->setStyleSheet(stylesheet);
}

void MainWindow::onClearRecentFiles() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clear Recent Files", "Are you sure you want to clear all recent files?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_recentFilesManager.clearRecentFiles();
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

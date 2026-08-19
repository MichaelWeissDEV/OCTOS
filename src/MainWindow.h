# 1 "./src/MainWindow.h"
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <memory>

#include "core/CompilerEnums.h"
#include "backend/DockerCompilerManager.h"
#include "highlighters/AsmHighlighter.h"
#include "highlighters/CppHighlighter.h"
#include "managers/PackageManager.h"
#include "managers/RecentFilesManager.h"
#include "managers/SnippetManager.h"
#include "ui/CompilerManagerDialog.h"
#include "ui/CompilerOutputPane.h"
#include "ui/Theme.h"
#include "utils/AssemblyTextProcessor.h"

class LineNumberArea;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

   public:
    explicit CodeEditor(QWidget* parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

   private slots:
    void updateLineNumberAreaWidth(int);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect&, int);

   private:
    LineNumberArea* m_lineNumberArea;
};

class LineNumberArea : public QWidget {
   public:
    explicit LineNumberArea(CodeEditor* editor) : QWidget(editor), m_codeEditor(editor) {}

    QSize sizeHint() const override;

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    CodeEditor* m_codeEditor;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

   private slots:
    void onInputTextChanged();
    void doCompile();
    void onSave();
    void onLoad();
    void onManageDockerCompilers();
    void onOpenRecentFile();
    void onInsertSnippet();
    void onChangeTheme(int index);
    void onClearRecentFiles();
    void onAddComparisonPane();
    void onRemovePane(CompilerOutputPane* pane);
    void onCloseOtherPanes();
    void onCompilationFinished(int jobId, const CompilationOutput& output);
    void onPaneSettingsChanged();
    void onToggleHighlightMode();
    void onSourceCursorPositionChanged();
    void onRefilterOutput();

   protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   private:
    void setupUi();
    void loadFile(const QString& filePath);
    void saveFile(const QString& filePath);
    void updateCompilerLists();
    void compileAllPanes();
    void refreshDockerCompilerList();
    QStringList getCompilersForLanguage(Language lang);
    void onPaneLanguageChanged(Language lang, CompilerOutputPane* pane);
    void applyHighlighting();
    QColor getHighlightColor(int index, int total);

    CodeEditor* m_inputEditor;
    QWidget* m_sourceEditorContainer;
    QSplitter* m_splitter;
    QSplitter* m_outputSplitter;
    QPushButton* m_addPaneButton;
    QPushButton* m_closeOthersButton;
    QPushButton* m_highlightToggle;

    int m_highlightMode;

    FilterSettings m_filterSettings;
    QAction* m_filterShowSegmentDirectives;
    QAction* m_filterShowDataDirectives;
    QAction* m_filterShowCfiDirectives;
    QAction* m_filterShowMetadataLabels;
    QAction* m_filterShowUnusedLabels;
    QAction* m_filterHideEmptyLabels;
    QAction* m_filterShowDebugInfo;
    QAction* m_filterShowComments;
    QAction* m_filterDemangle;

    QVector<CompilerOutputPane*> m_outputPanes;
    QMap<int, CompilerOutputPane*> m_jobToPaneMap;

    QTimer* m_debounceTimer;
    DockerCompilerManager* m_dockerManager;
    QVector<AsmHighlighter*> m_asmHighlighters;
    CppHighlighter* m_cppHighlighter;
    PackageManager m_packageManager;
    RecentFilesManager m_recentFilesManager;
    SnippetManager m_snippetManager;
    ThemeType m_currentTheme;

    QString m_currentFile;
};

#endif

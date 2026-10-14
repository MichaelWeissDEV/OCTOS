#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <memory>

#include "core/CompilerEnums.h"
#include "core/CompilationResult.h"
#include "backend/DockerCompilerManager.h"
#include "highlighters/AsmHighlighter.h"
#include "highlighters/CppHighlighter.h"
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
    ~MainWindow() override;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onInputTextChanged();
    void doCompile();
    void onNew();
    void onSave();
    void onSaveAs();
    void onLoad();
    void onManageDockerCompilers();
    void onInsertSnippet();
    void onChangeTheme(int index);
    void onClearRecentFiles();
    void onAddComparisonPane();
    void onRemovePane(CompilerOutputPane* pane);
    void onCloseOtherPanes();
    void onCompilationResultReceived(const CompilationResult& result);
    void onPaneSettingsChanged();
    void onToggleHighlightMode();
    void onSourceCursorPositionChanged();
    void onRefilterOutput();
    void onDocumentModificationChanged(bool modified);

private:
    void setupUi();
    bool maybeSave();
    void loadFile(const QString& filePath);
    bool saveFile(const QString& filePath);
    void updateRecentFilesMenu();
    void updateWindowTitle();
    void updateCompilerLists();
    void compileAllPanes();
    void refreshDockerCompilerList();
    QStringList getCompilersForLanguage(Language lang);
    void onPaneLanguageChanged(Language lang, CompilerOutputPane* pane);
    void applyHighlighting();
    QColor getHighlightColor(int index, int total);

    CodeEditor* m_inputEditor{nullptr};
    QWidget* m_sourceEditorContainer{nullptr};
    QSplitter* m_splitter{nullptr};
    QSplitter* m_outputSplitter{nullptr};
    QPushButton* m_addPaneButton{nullptr};
    QPushButton* m_closeOthersButton{nullptr};
    QPushButton* m_highlightToggle{nullptr};

    int m_highlightMode{0};
    quint64 m_inputRevision{0};

    FilterSettings m_filterSettings;
    QAction* m_filterShowSegmentDirectives{nullptr};
    QAction* m_filterShowDataDirectives{nullptr};
    QAction* m_filterShowCfiDirectives{nullptr};
    QAction* m_filterShowMetadataLabels{nullptr};
    QAction* m_filterShowUnusedLabels{nullptr};
    QAction* m_filterHideEmptyLabels{nullptr};
    QAction* m_filterShowDebugInfo{nullptr};
    QAction* m_filterShowComments{nullptr};
    QAction* m_filterDemangle{nullptr};

    QVector<CompilerOutputPane*> m_outputPanes;
    QMap<quint64, CompilerOutputPane*> m_paneLookup; // paneId -> pane

    QTimer* m_debounceTimer{nullptr};
    DockerCompilerManager* m_dockerManager{nullptr};
    QVector<AsmHighlighter*> m_asmHighlighters;
    CppHighlighter* m_cppHighlighter{nullptr};
    RecentFilesManager m_recentFilesManager;
    SnippetManager m_snippetManager;
    ThemeType m_currentTheme{ThemeType::Dark};

    QMenu* m_recentMenu{nullptr};
    QString m_currentFile;
};

#endif // MAINWINDOW_H

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
#include "managers/ProjectController.h"
#include "managers/RecentFilesManager.h"
#include "managers/SnippetManager.h"
#include "ui/CompilerManagerDialog.h"
#include "ui/CodeEditor.h"
#include "ui/CompilerOutputPane.h"
#include "ui/Theme.h"
#include "utils/AssemblyTextProcessor.h"

class AnalysisPanel;
class ProjectExplorerDock;
class QDockWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Accessors for the application shell and tests
    ProjectController* projectController() const { return m_project; }
    CodeEditor* activeEditor() const { return m_inputEditor; }
    QTabWidget* editorTabs() const { return m_editorTabs; }
    AnalysisPanel* analysisPanel() const { return m_analysisPanel; }
    ProjectExplorerDock* explorerDock() const { return m_explorerDock; }
    QVector<CompilerOutputPane*> outputPanes() const { return m_outputPanes; }
    // Opens (or focuses) a file of the open project in an editor tab.
    void openProjectFile(const QString& path);
    // Loads a stand-alone file (asks to close an open project first).
    void openStandaloneFile(const QString& path) { loadFile(path); }

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
    void onCompareDiff();
    void onCompileNow();

    // Project mode
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onCloseProject();
    void onAddProjectFile();
    void onRemoveProjectFile(const QString& path);
    void onSetMainFile(const QString& path);
    void onEditorTabChanged(int index);
    void onEditorTabCloseRequested(int index);

    // Profiles
    void onManageProfiles();
    void onSavePaneAsProfile();
    void onCompareProfiles();
    void onApplyProfile(const QString& id);

    void onResetLayout();

private:
    void setupUi();
    bool maybeSave();
    void loadFile(const QString& filePath);
    bool saveFile(const QString& filePath);
    void updateRecentFilesMenu();
    void updateWindowTitle();
    void updateCompilerLists();
    void compileAllPanes();
    void wirePane(CompilerOutputPane* pane);
    void showSelectedArtifact(CompilerOutputPane* pane);
    void updateEditorMarkers();
    void refreshDockerCompilerList();
    QStringList getCompilersForLanguage(Language lang);
    void onPaneLanguageChanged(Language lang, CompilerOutputPane* pane);
    void applyHighlighting();
    QColor getHighlightColor(int index, int total);

    void updateSourceHighlighter(Language lang);
    void applyHighlighter(CodeEditor* editor, Language lang);

    // project / tabs / docks (MainWindowProject.cpp)
    void setupDocks(QMenu* viewMenu);
    void setupProjectAndProfileMenus();
    CodeEditor* createEditorTab(const QString& title, const QString& projectPath, Language lang);
    void setActiveEditor(CodeEditor* editor);
    void rebuildProjectTabs();
    void updateTabTitle(CodeEditor* editor);
    bool maybeSaveProject();
    // Stand-alone file actions (New, Open, drag & drop, Save As) must not touch an open project.
    bool leaveProjectForStandalone();
    QVector<CompilerProfile> allProfiles() const;
    QString paneTitle(CompilerOutputPane* pane) const;
    CompilerOutputPane* ensurePaneCount(int n);
    void applyProfileToPane(const CompilerProfile& profile, CompilerOutputPane* pane);
    void saveLayout();
    void restoreLayout();
    void rebuildApplyProfileMenu();
    QStringList installedCompilerImages() const;

    CodeEditor* m_inputEditor{nullptr};
    QTabWidget* m_editorTabs{nullptr};
    QMap<CodeEditor*, QString> m_tabPath;               // project-relative path per editor (empty: stand-alone file)
    QMap<CodeEditor*, QSyntaxHighlighter*> m_highlighters;
    QVector<QMetaObject::Connection> m_editorConnections;
    ProjectController* m_project{nullptr};
    ProjectExplorerDock* m_explorerDock{nullptr};
    QDockWidget* m_analysisDock{nullptr};
    AnalysisPanel* m_analysisPanel{nullptr};
    QMenu* m_applyProfileMenu{nullptr};
    QVector<QAction*> m_projectOnlyActions;
    QAction* m_compileNowAction{nullptr};
    QWidget* m_sourceEditorContainer{nullptr};
    QSplitter* m_splitter{nullptr};
    QSplitter* m_outputSplitter{nullptr};
    QPushButton* m_addPaneButton{nullptr};
    QPushButton* m_compareDiffButton{nullptr};
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
    QSyntaxHighlighter* m_sourceHighlighter{nullptr};
    RecentFilesManager m_recentFilesManager;
    SnippetManager m_snippetManager;
    ThemeType m_currentTheme{ThemeType::Dark};

    QMenu* m_recentMenu{nullptr};
    QString m_currentFile;
};

#endif // MAINWINDOW_H

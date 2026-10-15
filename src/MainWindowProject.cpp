// Project mode, editor tabs, profiles, docks and layout persistence of the main window.
// The actual project logic lives in ProjectController; this file only wires it to widgets.
#include "MainWindow.h"

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStatusBar>

#include "core/AppSettings.h"
#include "core/ProfileStore.h"
#include "ui/AnalysisPanel.h"
#include "ui/ProfileDialog.h"
#include "ui/ProjectExplorerDock.h"
#include "ui/SourceEditorTab.h"

// ---------------------------------------------------------------------------------------------
// Editor tabs
// ---------------------------------------------------------------------------------------------

CodeEditor* MainWindow::createEditorTab(const QString& title, const QString& projectPath, Language lang) {
    auto* tab = new SourceEditorTab();
    CodeEditor* editor = tab->editor();
    m_tabPath[editor] = projectPath;
    applyHighlighter(editor, lang);
    m_editorTabs->addTab(tab, title);

    // Changes of ANY tab flow into the project (other files are compiled along with the active one).
    connect(editor, &QPlainTextEdit::textChanged, this, [this, editor]() {
        const QString path = m_tabPath.value(editor);
        if (m_project->isOpen() && !path.isEmpty()) m_project->setFileContent(path, editor->toPlainText());
    });
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor]() { updateTabTitle(editor); });
    return editor;
}

void MainWindow::setActiveEditor(CodeEditor* editor) {
    for (const auto& c : std::as_const(m_editorConnections)) disconnect(c);
    m_editorConnections.clear();
    m_inputEditor = editor;
    if (!editor) return;
    m_sourceHighlighter = m_highlighters.value(editor, nullptr);
    m_editorConnections << connect(editor, &QPlainTextEdit::textChanged, this, &MainWindow::onInputTextChanged);
    m_editorConnections << connect(editor->document(), &QTextDocument::modificationChanged, this,
                                   &MainWindow::onDocumentModificationChanged);
    m_editorConnections << connect(editor, &QPlainTextEdit::cursorPositionChanged, this,
                                   &MainWindow::onSourceCursorPositionChanged);
    updateEditorMarkers();
}

void MainWindow::updateTabTitle(CodeEditor* editor) {
    const int idx = m_editorTabs->indexOf(editor->parentWidget());
    if (idx < 0) return;
    const QString path = m_tabPath.value(editor);
    QString title;
    bool dirty = false;
    if (m_project->isOpen() && !path.isEmpty()) {
        title = path.mid(path.lastIndexOf('/') + 1);
        dirty = m_project->isFileDirty(path);
    } else {
        title = m_currentFile.isEmpty() ? "Untitled" : QFileInfo(m_currentFile).fileName();
        dirty = editor->document()->isModified();
    }
    m_editorTabs->setTabText(idx, dirty ? title + " ●" : title);
    m_editorTabs->setTabToolTip(idx, path.isEmpty() ? m_currentFile : path);
}

void MainWindow::onEditorTabChanged(int index) {
    // During construction the first tab is added before the window is fully wired up.
    if (index < 0 || !m_debounceTimer) return;
    auto* tab = qobject_cast<SourceEditorTab*>(m_editorTabs->widget(index));
    if (!tab || tab->editor() == m_inputEditor) return;
    setActiveEditor(tab->editor());

    // In project mode the file decides the language of the compile panes.
    const QString path = m_tabPath.value(tab->editor());
    if (m_project->isOpen() && !path.isEmpty()) {
        const Language lang = ProjectController::languageForPath(path);
        for (CompilerOutputPane* pane : std::as_const(m_outputPanes)) {
            if (pane->getLanguage() != lang) pane->setLanguage(lang);
        }
        // Switching tabs only changes what is compiled and the pane language. The project itself
        // (targets, main file) changes only through explicit actions such as "Use as main file".
    }
    updateWindowTitle();
    onInputTextChanged();
}

void MainWindow::onEditorTabCloseRequested(int index) {
    auto* tab = qobject_cast<SourceEditorTab*>(m_editorTabs->widget(index));
    if (!tab) return;
    CodeEditor* editor = tab->editor();
    const QString path = m_tabPath.value(editor);

    if (m_project->isOpen() && !path.isEmpty()) {
        if (m_project->isFileDirty(path)) {
            const auto r = QMessageBox::question(this, "Unsaved changes",
                QString("'%1' has unsaved changes. Save it into the project?").arg(path),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            if (r == QMessageBox::Cancel) return;
            if (r == QMessageBox::Save) {
                if (m_project->projectFilePath().isEmpty()) { onSaveProjectAs(); if (m_project->isFileDirty(path)) return; }
                else {
                    QString err;
                    if (!m_project->saveFile(path, &err)) { QMessageBox::warning(this, "Save failed", err); return; }
                }
            }
        }
        // The last tab of a project is replaced by an empty one, so there is always an editor.
        if (m_editorTabs->count() == 1) return;
    } else if (m_editorTabs->count() == 1) {
        return;   // stand-alone mode keeps its single tab
    }

    const bool wasActive = (editor == m_inputEditor);
    m_editorTabs->removeTab(index);
    m_tabPath.remove(editor);
    delete m_highlighters.take(editor);
    tab->deleteLater();
    if (wasActive) {
        m_inputEditor = nullptr;
        auto* cur = qobject_cast<SourceEditorTab*>(m_editorTabs->currentWidget());
        if (cur) setActiveEditor(cur->editor());
    }
    updateWindowTitle();
}

// ---------------------------------------------------------------------------------------------
// Project lifecycle
// ---------------------------------------------------------------------------------------------

bool MainWindow::leaveProjectForStandalone() {
    if (!m_project->isOpen()) return true;
    // New / Open of a stand-alone file would otherwise write into the file of the open project tab.
    const auto r = QMessageBox::question(this, "Project open",
        QString("The project '%1' is open.\nClose it to work with a single file?").arg(m_project->workspace().name()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (r != QMessageBox::Yes) return false;
    onCloseProject();                 // asks to save unsaved changes; the user may cancel there
    return !m_project->isOpen();
}

bool MainWindow::maybeSaveProject() {
    if (!m_project->isOpen() || !m_project->isDirty()) return true;
    const auto r = QMessageBox::warning(this, "Save Project",
        QString("The project '%1' has unsaved changes.\nSave all changes?").arg(m_project->workspace().name()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Cancel) return false;
    if (r == QMessageBox::Discard) return true;
    onSaveProject();
    return !m_project->isDirty();
}

void MainWindow::rebuildProjectTabs() {
    // Remove every tab, then create one per open project file (called after open/new/close).
    while (m_editorTabs->count() > 0) {
        auto* tab = qobject_cast<SourceEditorTab*>(m_editorTabs->widget(0));
        m_editorTabs->removeTab(0);
        if (tab) {
            m_tabPath.remove(tab->editor());
            delete m_highlighters.take(tab->editor());
            tab->deleteLater();
        }
    }
    m_inputEditor = nullptr;
}

void MainWindow::onNewProject() {
    if (m_project->isOpen() && !maybeSaveProject()) return;
    if (!m_project->isOpen() && !maybeSave()) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New Project", "Project name:", QLineEdit::Normal, "My Project", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    const QString mainName = QInputDialog::getText(this, "New Project", "Main source file (relative path):", QLineEdit::Normal, "main.cpp", &ok);
    if (!ok) return;

    rebuildProjectTabs();
    m_project->newProject(name.trimmed());
    QString err;
    if (!m_project->addFile(mainName.trimmed(), "", &err)) {
        QMessageBox::warning(this, "New Project", err);
        m_project->closeProject();
        m_inputEditor = createEditorTab("Untitled", QString(), Language::Cpp);
        setActiveEditor(m_inputEditor);
        return;
    }
    m_project->ensureTargetForMain("default", m_project->filePaths().first());
    m_project->setActiveTarget("default");
    m_currentFile.clear();
    openProjectFile(m_project->filePaths().first());
    m_explorerDock->show();
}

void MainWindow::onOpenProject() {
    if (m_project->isOpen() && !maybeSaveProject()) return;
    if (!m_project->isOpen() && !maybeSave()) return;
    const QString file = QFileDialog::getOpenFileName(this, "Open Project", AppSettings::instance().lastDirectory(),
                                                      "OCTOS Projects (*.octos);;All Files (*)");
    if (file.isEmpty()) return;
    QString err;
    ProjectController probe;
    if (!probe.openProject(file, &err)) {   // validate before touching the current session
        QMessageBox::warning(this, "Open Project", err);
        return;
    }
    rebuildProjectTabs();
    m_project->openProject(file, &err);
    AppSettings::instance().setLastDirectory(QFileInfo(file).absolutePath());
    m_currentFile.clear();
    const QStringList paths = m_project->filePaths();
    const BuildTarget* t = m_project->workspace().findTarget(m_project->workspace().configuration().activeTargetName);
    const QString first = (t && m_project->workspace().hasSourceFile(t->mainSourceFile)) ? t->mainSourceFile
                          : (paths.isEmpty() ? QString() : paths.first());
    if (first.isEmpty()) {
        m_inputEditor = createEditorTab("Untitled", QString(), Language::Cpp);
        setActiveEditor(m_inputEditor);
    } else {
        openProjectFile(first);
    }
    m_explorerDock->show();
    for (const QString& w : m_project->workspace().warnings()) qWarning().noquote() << "project:" << w;
    updateWindowTitle();
    rebuildApplyProfileMenu();
}

void MainWindow::openProjectFile(const QString& path) {
    if (!m_project->isOpen() || !m_project->workspace().hasSourceFile(path)) return;
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto* tab = qobject_cast<SourceEditorTab*>(m_editorTabs->widget(i));
        if (tab && m_tabPath.value(tab->editor()) == path) { m_editorTabs->setCurrentIndex(i); return; }
    }
    const SourceFile sf = m_project->workspace().findSourceFile(path);
    CodeEditor* editor = createEditorTab(path.mid(path.lastIndexOf('/') + 1), path, sf.language);
    {
        QSignalBlocker b(editor);   // loading is not an edit
        editor->setPlainText(sf.content);
    }
    editor->document()->setModified(false);
    m_editorTabs->setCurrentWidget(editor->parentWidget());
    if (m_inputEditor != editor) setActiveEditor(editor);
    onEditorTabChanged(m_editorTabs->indexOf(editor->parentWidget()));
    updateTabTitle(editor);
}

void MainWindow::onSaveProject() {
    if (!m_project->isOpen()) return;
    if (m_project->projectFilePath().isEmpty()) { onSaveProjectAs(); return; }
    QString err;
    if (!m_project->saveProject(&err)) { QMessageBox::warning(this, "Save Project", err); return; }
    for (auto it = m_tabPath.constBegin(); it != m_tabPath.constEnd(); ++it) it.key()->document()->setModified(false);
    updateWindowTitle();
}

void MainWindow::onSaveProjectAs() {
    if (!m_project->isOpen()) return;
    QString file = QFileDialog::getSaveFileName(this, "Save Project As", AppSettings::instance().lastDirectory(),
                                                "OCTOS Projects (*.octos)");
    if (file.isEmpty()) return;
    if (!file.endsWith(".octos")) file += ".octos";
    QString err;
    if (!m_project->saveProjectAs(file, &err)) { QMessageBox::warning(this, "Save Project", err); return; }
    AppSettings::instance().setLastDirectory(QFileInfo(file).absolutePath());
    for (auto it = m_tabPath.constBegin(); it != m_tabPath.constEnd(); ++it) it.key()->document()->setModified(false);
    updateWindowTitle();
}

void MainWindow::onCloseProject() {
    if (!m_project->isOpen()) return;
    if (!maybeSaveProject()) return;
    rebuildProjectTabs();
    m_project->closeProject();
    m_inputEditor = createEditorTab("Untitled", QString(), Language::Cpp);
    m_inputEditor->setPlaceholderText("\n Enter your code here...");
    setActiveEditor(m_inputEditor);
    m_currentFile.clear();
    m_explorerDock->hide();
    updateWindowTitle();
}

void MainWindow::onAddProjectFile() {
    if (!m_project->isOpen()) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Add File", "New file (relative path, e.g. src/util.h):", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    QString err;
    if (!m_project->addFile(name.trimmed(), "", &err)) { QMessageBox::warning(this, "Add File", err); return; }
    bool norm = false;
    openProjectFile(Workspace::normalizeRelativePath(name.trimmed(), &norm));
}

void MainWindow::onRemoveProjectFile(const QString& path) {
    if (!m_project->isOpen()) return;
    if (m_project->filePaths().size() <= 1) { QMessageBox::information(this, "Remove File", "A project needs at least one file."); return; }
    if (QMessageBox::question(this, "Remove File", QString("Remove '%1' from the project?\nThe project data of this file is lost.").arg(path),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto* tab = qobject_cast<SourceEditorTab*>(m_editorTabs->widget(i));
        if (tab && m_tabPath.value(tab->editor()) == path) {
            CodeEditor* editor = tab->editor();
            const bool wasActive = editor == m_inputEditor;
            m_editorTabs->removeTab(i);
            m_tabPath.remove(editor);
            delete m_highlighters.take(editor);
            tab->deleteLater();
            if (wasActive) {
                m_inputEditor = nullptr;
                if (auto* cur = qobject_cast<SourceEditorTab*>(m_editorTabs->currentWidget())) setActiveEditor(cur->editor());
            }
            break;
        }
    }
    QString err;
    if (!m_project->removeFile(path, &err)) QMessageBox::warning(this, "Remove File", err);
    if (!m_inputEditor && !m_project->filePaths().isEmpty()) openProjectFile(m_project->filePaths().first());
    updateWindowTitle();
}

void MainWindow::onSetMainFile(const QString& path) {
    if (!m_project->isOpen()) return;
    const QString target = m_project->workspace().configuration().activeTargetName.isEmpty()
                               ? QString("default") : m_project->workspace().configuration().activeTargetName;
    if (m_project->ensureTargetForMain(target, path)) m_project->setActiveTarget(target);
    openProjectFile(path);
}

// ---------------------------------------------------------------------------------------------
// Menus, docks, layout
// ---------------------------------------------------------------------------------------------

void MainWindow::setupProjectAndProfileMenus() {
    QMenu* projectMenu = menuBar()->addMenu("&Project");
    projectMenu->addAction("&New Project…", this, &MainWindow::onNewProject);
    projectMenu->addAction("&Open Project…", this, &MainWindow::onOpenProject);
    projectMenu->addSeparator();
    QAction* saveProj = projectMenu->addAction("&Save Project", this, &MainWindow::onSaveProject);
    // Ctrl+Shift+S is the platform "Save As" chord: a second action on it would make both ambiguous.
    saveProj->setShortcut(QKeySequence("Ctrl+Alt+S"));
    QAction* saveProjAs = projectMenu->addAction("Save Project &As…", this, &MainWindow::onSaveProjectAs);
    QAction* addFile = projectMenu->addAction("Add &File…", this, &MainWindow::onAddProjectFile);
    projectMenu->addSeparator();
    QAction* closeProj = projectMenu->addAction("&Close Project", this, &MainWindow::onCloseProject);
    m_projectOnlyActions << saveProj << saveProjAs << addFile << closeProj;
    for (QAction* a : std::as_const(m_projectOnlyActions)) a->setEnabled(false);
    connect(m_project, &ProjectController::projectChanged, this, [this]() {
        for (QAction* a : std::as_const(m_projectOnlyActions)) a->setEnabled(m_project->isOpen());
        updateWindowTitle();
    });
    connect(m_project, &ProjectController::dirtyChanged, this, [this]() { updateWindowTitle(); });
    connect(m_project, &ProjectController::fileDirtyChanged, this, [this](const QString& path) {
        for (auto it = m_tabPath.constBegin(); it != m_tabPath.constEnd(); ++it)
            if (it.value() == path) updateTabTitle(it.key());
    });

    QMenu* profiles = menuBar()->addMenu("Pro&files");
    profiles->addAction("&Manage Profiles…", this, &MainWindow::onManageProfiles);
    profiles->addAction("&Save Pane as Profile…", this, &MainWindow::onSavePaneAsProfile);
    m_applyProfileMenu = profiles->addMenu("&Apply Profile to Pane");
    profiles->addAction("&Compare Two Profiles…", this, &MainWindow::onCompareProfiles);
    rebuildApplyProfileMenu();
}

void MainWindow::setupDocks(QMenu* viewMenu) {
    setDockNestingEnabled(true);

    m_explorerDock = new ProjectExplorerDock(m_project, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_explorerDock);
    m_explorerDock->hide();   // shown when a project is open
    connect(m_explorerDock, &ProjectExplorerDock::openFileRequested, this, &MainWindow::openProjectFile);
    connect(m_explorerDock, &ProjectExplorerDock::addFileRequested, this, &MainWindow::onAddProjectFile);
    connect(m_explorerDock, &ProjectExplorerDock::removeFileRequested, this, &MainWindow::onRemoveProjectFile);
    connect(m_explorerDock, &ProjectExplorerDock::setMainRequested, this, &MainWindow::onSetMainFile);

    m_analysisPanel = new AnalysisPanel();
    m_analysisDock = new QDockWidget("Assembly Analysis", this);
    m_analysisDock->setObjectName("AssemblyAnalysisDock");
    m_analysisDock->setWidget(m_analysisPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_analysisDock);
    m_analysisDock->hide();
    connect(m_analysisPanel, &AnalysisPanel::assemblyLineActivated, this, [this](quint64 paneId, int rawLine) {
        CompilerOutputPane* pane = m_paneLookup.value(paneId, nullptr);
        if (!pane) return;
        const int display = pane->displayLineForRaw(rawLine);
        if (display < 0) {
            statusBar()->showMessage("That line is hidden by the current assembly filters.", 4000);
            return;
        }
        pane->showArtifactText(pane->getEditor()->toPlainText(), ArtifactType::Assembly, false);   // make sure the editor is visible
        pane->getEditor()->goToLine(display + 1);
    });
    connect(m_analysisPanel, &AnalysisPanel::sourceLineActivated, this, [this](int line) {
        if (m_inputEditor) m_inputEditor->goToLine(line);
    });

    viewMenu->addAction(m_explorerDock->toggleViewAction());
    viewMenu->addAction(m_analysisDock->toggleViewAction());
    viewMenu->addSeparator();
    viewMenu->addAction("&Reset Layout", this, &MainWindow::onResetLayout);
    statusBar()->setSizeGripEnabled(true);

    // Remember the factory layout so "Reset Layout" can return to it.
    AppSettings::instance().setLayoutBlob("defaultState", saveState(1));
}

void MainWindow::saveLayout() {
    auto& s = AppSettings::instance();
    s.setLayoutBlob("geometry", saveGeometry());
    s.setLayoutBlob("state", saveState(1));
    if (m_splitter) s.setLayoutBlob("splitter", m_splitter->saveState());
    if (m_outputSplitter) s.setLayoutBlob("outputSplitter", m_outputSplitter->saveState());
}

void MainWindow::restoreLayout() {
    auto& s = AppSettings::instance();
    const QByteArray geo = s.layoutBlob("geometry");
    if (!geo.isEmpty()) restoreGeometry(geo);
    const QByteArray state = s.layoutBlob("state");
    if (!state.isEmpty()) restoreState(state, 1);
    if (m_splitter && !s.layoutBlob("splitter").isEmpty()) m_splitter->restoreState(s.layoutBlob("splitter"));
    if (m_outputSplitter && !s.layoutBlob("outputSplitter").isEmpty()) m_outputSplitter->restoreState(s.layoutBlob("outputSplitter"));
    // A restored state must never hide the explorer of an open project.
    if (m_project->isOpen()) m_explorerDock->show();
}

void MainWindow::onResetLayout() {
    auto& s = AppSettings::instance();
    const QByteArray def = s.layoutBlob("defaultState");
    s.clearLayout();
    s.setLayoutBlob("defaultState", def);
    if (!def.isEmpty()) restoreState(def, 1);
    m_analysisDock->hide();
    m_explorerDock->setVisible(m_project->isOpen());
    if (m_splitter) m_splitter->setSizes({600, 600});
    resize(1600, 900);
}

// ---------------------------------------------------------------------------------------------
// Profiles
// ---------------------------------------------------------------------------------------------

QStringList MainWindow::installedCompilerImages() const {
    QStringList out;
    for (const DockerImage& img : m_dockerManager->listInstalledImages()) out << img.fullName;
    return out;
}

QVector<CompilerProfile> MainWindow::allProfiles() const {
    QVector<CompilerProfile> all = ProfileStore::load().profiles();
    if (m_project->isOpen()) {
        for (CompilerProfile p : m_project->workspace().profiles()) {
            p.name += " (project)";
            all.append(p);
        }
    }
    return all;
}

QString MainWindow::paneTitle(CompilerOutputPane* pane) const {
    const int idx = m_outputPanes.indexOf(pane) + 1;
    QString flags = pane->getCompilerFlags();
    QString compiler = pane->getSelectedCompiler();
    if (compiler.isEmpty()) compiler = "no compiler";
    return QString("Pane %1: %2%3").arg(idx).arg(compiler, flags.isEmpty() ? QString() : " " + flags);
}

void MainWindow::applyProfileToPane(const CompilerProfile& profile, CompilerOutputPane* pane) {
    pane->setLanguage(profile.language);                // refreshes the compiler list for that language
    if (!profile.compilerImage.isEmpty()) {
        if (pane->hasCompiler(profile.compilerImage)) pane->setSelectedCompiler(profile.compilerImage);
        else QMessageBox::warning(this, "Profile", QString("The image '%1' of profile '%2' is not installed. Install it in the Compiler Manager.")
                                                       .arg(profile.compilerImage, profile.name));
    }
    pane->setCompilerFlags(profile.effectiveFlags());
    pane->setSyntaxFlavor(profile.syntaxFlavor);
}

CompilerOutputPane* MainWindow::ensurePaneCount(int n) {
    while (m_outputPanes.size() < n) onAddComparisonPane();
    return m_outputPanes.last();
}

void MainWindow::rebuildApplyProfileMenu() {
    if (!m_applyProfileMenu) return;
    m_applyProfileMenu->clear();
    const auto profiles = allProfiles();
    if (profiles.isEmpty()) {
        m_applyProfileMenu->addAction("(no profiles yet)")->setEnabled(false);
        return;
    }
    for (const CompilerProfile& p : profiles) {
        QString id = p.id;
        QAction* a = m_applyProfileMenu->addAction(p.name);
        connect(a, &QAction::triggered, this, [this, id]() { onApplyProfile(id); });
    }
}

void MainWindow::onApplyProfile(const QString& id) {
    for (const CompilerProfile& p : allProfiles()) {
        if (p.id != id) continue;
        CompilerOutputPane* target = m_outputPanes.first();
        if (m_outputPanes.size() > 1) {
            QStringList names;
            for (CompilerOutputPane* pane : std::as_const(m_outputPanes)) names << paneTitle(pane);
            bool ok = false;
            const QString choice = QInputDialog::getItem(this, "Apply Profile", "Apply to:", names, 0, false, &ok);
            if (!ok) return;
            target = m_outputPanes.at(names.indexOf(choice));
        }
        applyProfileToPane(p, target);
        return;
    }
}

void MainWindow::onManageProfiles() {
    ProfileDialog dlg(ProfileStore::load(), installedCompilerImages(), this);
    if (dlg.exec() == QDialog::Accepted) {
        QString err;
        dlg.result().save(&err);
        rebuildApplyProfileMenu();
    }
}

void MainWindow::onSavePaneAsProfile() {
    CompilerOutputPane* pane = m_outputPanes.first();
    if (m_outputPanes.size() > 1) {
        QStringList names;
        for (CompilerOutputPane* p : std::as_const(m_outputPanes)) names << paneTitle(p);
        bool ok = false;
        const QString choice = QInputDialog::getItem(this, "Save Pane as Profile", "Pane:", names, 0, false, &ok);
        if (!ok) return;
        pane = m_outputPanes.at(names.indexOf(choice));
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Save Profile", "Profile name:", QLineEdit::Normal, paneTitle(pane).section(": ", 1), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    ProfileStore store = ProfileStore::load();
    CompilerProfile p;
    p.name = name.trimmed();
    p.id = store.makeUniqueId(p.name);
    p.compilerImage = pane->getSelectedCompiler();
    p.language = pane->getLanguage();
    p.userFlags = pane->getCompilerFlags();       // flags are stored as typed (optimization included)
    p.syntaxFlavor = pane->getSyntaxFlavor();
    QString err;
    if (!store.add(p, &err) || !store.save(&err)) { QMessageBox::warning(this, "Save Profile", err); return; }
    rebuildApplyProfileMenu();
    statusBar()->showMessage(QString("Profile '%1' saved").arg(p.name), 4000);
}

void MainWindow::onCompareProfiles() {
    const auto profiles = allProfiles();
    if (profiles.size() < 2) {
        QMessageBox::information(this, "Compare Profiles", "Create at least two profiles first (Profiles > Manage Profiles).");
        return;
    }
    CompareProfilesDialog dlg(profiles, this);
    if (dlg.exec() != QDialog::Accepted) return;
    const CompilerProfile* a = nullptr;
    const CompilerProfile* b = nullptr;
    for (const CompilerProfile& p : profiles) {
        if (p.id == dlg.profileA() && !a) a = &p;
        if (p.id == dlg.profileB()) b = &p;
    }
    if (!a || !b) return;
    ensurePaneCount(2);
    applyProfileToPane(*a, m_outputPanes.at(0));
    applyProfileToPane(*b, m_outputPanes.at(1));
    onPaneSettingsChanged();
}

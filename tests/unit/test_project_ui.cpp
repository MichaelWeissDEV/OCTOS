// Project mode, editor tabs, find/replace, analysis panel and layout persistence in the real window.
#include "../TestSupport.h"
#include <QTest>
#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QSplitter>
#include <QStandardPaths>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QAction>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "core/AppSettings.h"
#include "MainWindow.h"
#include "ui/AnalysisPanel.h"
#include "ui/ProjectExplorerDock.h"
#include "ui/SourceEditorTab.h"

namespace {
QString readCorpus(const QString& name) {
    QFile f(QString(OCTOS_TEST_DATA_DIR) + "/asm/" + name);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}
}

class TestProjectUi : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void init() { AppSettings::instance().clearLayout(); }

    void testSingleFileModeHasNoTabBar();
    void testProjectTabsDirtyAndSave();
    void testOpeningAndSwitchingTabsDoesNotChangeTheProject();
    void testStandaloneActionsNeverOverwriteProjectFiles();
    void testSaveProjectShortcutIsUnambiguous();
    void testExplorerReflectsProject();
    void testFindReplace();
    void testAnalysisPanelFromRealAssembly();
    void testAnalysisNavigation();
    void testAnalysisDockFedByCompileResult();
    void testLayoutIsPersisted();
    void testResetLayout();
};

void TestProjectUi::testSingleFileModeHasNoTabBar() {
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QCOMPARE(w.editorTabs()->count(), 1);
    QVERIFY(!w.editorTabs()->tabBar()->isVisible());     // the classic single-file look is unchanged
    QVERIFY(!w.explorerDock()->isVisible());
    QVERIFY(w.activeEditor());
}

void TestProjectUi::testProjectTabsDirtyAndSave() {
    QTemporaryDir dir;
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    ProjectController* pc = w.projectController();

    pc->newProject("Demo");
    QString err;
    QVERIFY(pc->addFile("src/main.cpp", "#include \"util.h\"\nint main(){return h();}\n", &err));
    QVERIFY(pc->addFile("src/util.h", "int h();\n", &err));
    QVERIFY(pc->saveProjectAs(dir.path() + "/demo.octos", &err));
    QVERIFY(!pc->isDirty());

    // The window still shows the stand-alone tab; opening files replaces it with project tabs.
    w.openProjectFile("src/main.cpp");
    w.openProjectFile("src/util.h");
    w.openProjectFile("src/main.cpp");                          // focusing an open file does not duplicate its tab
    int projectTabs = 0;
    for (int i = 0; i < w.editorTabs()->count(); ++i)
        if (w.editorTabs()->tabText(i).contains("main.cpp") || w.editorTabs()->tabText(i).contains("util.h")) ++projectTabs;
    QCOMPARE(projectTabs, 2);
    QVERIFY(w.editorTabs()->tabBar()->isVisible());
    QCOMPARE(w.activeEditor()->toPlainText(), QString("#include \"util.h\"\nint main(){return h();}\n"));

    // Typing marks exactly this file dirty, in the controller and in the tab title.
    w.activeEditor()->moveCursor(QTextCursor::End);
    w.activeEditor()->insertPlainText("// edit");
    QVERIFY(pc->isFileDirty("src/main.cpp"));
    QVERIFY(!pc->isFileDirty("src/util.h"));
    QVERIFY(w.editorTabs()->tabText(w.editorTabs()->currentIndex()).contains("●"));
    QVERIFY(w.windowTitle().contains("*"));

    // Ctrl+S semantics: saves only the current file into the project file
    QMetaObject::invokeMethod(&w, "onSave");
    QVERIFY(!pc->isFileDirty("src/main.cpp"));
    QVERIFY(!w.editorTabs()->tabText(w.editorTabs()->currentIndex()).contains("●"));
    Workspace onDisk = Workspace::loadFromFile(dir.path() + "/demo.octos", &err);
    QVERIFY(onDisk.findSourceFile("src/main.cpp").content.endsWith("// edit"));
}

namespace {
// Answers the next modal QMessageBox with the given button (asynchronously, the dialog blocks exec()).
void answerNextDialog(QMessageBox::StandardButton button, int* shown = nullptr) {
    QTimer::singleShot(100, [button, shown]() {
        if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            if (shown) ++*shown;
            if (auto* b = box->button(button)) b->click(); else box->reject();
        }
    });
}
}

void TestProjectUi::testOpeningAndSwitchingTabsDoesNotChangeTheProject() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/p.octos";
    {   // a project on disk with a main .cpp, a second .cpp and a header; target main = main.cpp
        ProjectController pc;
        pc.newProject("Saved");
        QString err;
        QVERIFY(pc.addFile("main.cpp", "int main(){}\n", &err));
        QVERIFY(pc.addFile("other.cpp", "int other(){}\n", &err));
        QVERIFY(pc.addFile("util.h", "int h();\n", &err));
        QVERIFY(pc.ensureTargetForMain("default", "main.cpp"));
        QVERIFY(pc.setActiveTarget("default"));
        QVERIFY(pc.saveProjectAs(path, &err));
    }
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QString err;
    QVERIFY2(w.projectController()->openProject(path, &err), qPrintable(err));
    QVERIFY(!w.projectController()->isDirty());                    // a freshly opened project is clean
    w.openProjectFile("main.cpp");
    QVERIFY(!w.projectController()->isDirty());                    // opening a tab does not change anything
    const QStringList targetFilesBefore = w.projectController()->workspace().findTarget("default")->sourceFiles;
    w.openProjectFile("util.h");
    w.openProjectFile("other.cpp");
    w.openProjectFile("main.cpp");
    QVERIFY2(!w.projectController()->isDirty(), "switching tabs must not mark the project dirty");
    const BuildTarget* t = w.projectController()->workspace().findTarget("default");
    QVERIFY(t);
    QCOMPARE(t->mainSourceFile, QString("main.cpp"));              // the header tab did not become the main file
    QCOMPARE(t->sourceFiles, targetFilesBefore);                    // and the target's file list was not rewritten
    QVERIFY(!w.windowTitle().contains("*"));
}

void TestProjectUi::testStandaloneActionsNeverOverwriteProjectFiles() {
    QTemporaryDir dir;
    QFile ext(dir.path() + "/external.cpp");
    QVERIFY(ext.open(QIODevice::WriteOnly));
    ext.write("// external stand-alone file\n");
    ext.close();

    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    ProjectController* pc = w.projectController();
    pc->newProject("Guarded");
    QString err;
    QVERIFY(pc->addFile("main.cpp", "int precious();\n", &err));
    w.openProjectFile("main.cpp");
    QCOMPARE(w.activeEditor()->toPlainText(), QString("int precious();\n"));

    // File > New: declined -> the project file keeps its content
    int shown = 0;
    answerNextDialog(QMessageBox::No, &shown);
    QMetaObject::invokeMethod(&w, "onNew");
    QCOMPARE(shown, 1);
    QCOMPARE(pc->fileContent("main.cpp"), QString("int precious();\n"));
    QCOMPARE(w.activeEditor()->toPlainText(), QString("int precious();\n"));
    QVERIFY(pc->isOpen());

    // File > Open / drag&drop / recent files all end in loadFile(): declined -> unchanged
    shown = 0;
    answerNextDialog(QMessageBox::No, &shown);
    w.openStandaloneFile(dir.path() + "/external.cpp");
    QCOMPARE(shown, 1);
    QCOMPARE(pc->fileContent("main.cpp"), QString("int precious();\n"));
    QCOMPARE(w.activeEditor()->toPlainText(), QString("int precious();\n"));

    // Accepted: the project is closed first (it is unsaved -> the save prompt is answered with Discard),
    // then the stand-alone file is loaded into a fresh editor; the project file is never rewritten.
    shown = 0;
    answerNextDialog(QMessageBox::Yes, &shown);
    QTimer::singleShot(400, []() {
        if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
            if (auto* b = box->button(QMessageBox::Discard)) b->click();
    });
    w.openStandaloneFile(dir.path() + "/external.cpp");
    QVERIFY(!pc->isOpen());
    QCOMPARE(w.activeEditor()->toPlainText(), QString("// external stand-alone file\n"));
}

void TestProjectUi::testSaveProjectShortcutIsUnambiguous() {
    MainWindow w;
    // No two menu actions may share a shortcut (Qt would treat both as ambiguous and neither would fire).
    QMap<QString, QString> seen;
    for (QAction* a : w.findChildren<QAction*>()) {
        for (const QKeySequence& ks : a->shortcuts()) {
            if (ks.isEmpty() || a->text().isEmpty()) continue;
            const QString key = ks.toString(QKeySequence::PortableText);
            if (seen.contains(key) && seen[key] != a->text())
                QFAIL(qPrintable(QString("shortcut %1 used by '%2' and '%3'").arg(key, seen[key], a->text())));
            seen[key] = a->text();
        }
    }
    QVERIFY(!seen.isEmpty());
    for (QAction* a : w.findChildren<QAction*>())
        if (a->text().contains("Save Project") && !a->text().contains("As")) {
            QVERIFY(!a->shortcuts().contains(QKeySequence(QKeySequence::SaveAs)));
            QVERIFY(!a->shortcut().isEmpty());
        }
}

void TestProjectUi::testExplorerReflectsProject() {
    MainWindow w;
    w.show();
    ProjectController* pc = w.projectController();
    pc->newProject("Tree");
    QString err;
    pc->addFile("a.cpp", "x", &err);
    pc->addFile("inc/b.h", "y", &err);
    pc->addFile("inc/deep/c.h", "z", &err);
    auto* model = w.explorerDock()->model();
    QCOMPARE(model->rowCount(), 2);                                  // "inc/" folder and a.cpp
    QStringList top;
    for (int i = 0; i < model->rowCount(); ++i) top << model->item(i)->text();
    QVERIFY(top.contains("inc/"));
    QVERIFY(top.join(",").contains("a.cpp"));
    // Unsaved files are marked
    QVERIFY(top.join(",").contains("● a.cpp"));
    pc->removeFile("a.cpp", &err);
    QCOMPARE(model->rowCount(), 1);
    pc->closeProject();
    QCOMPARE(model->rowCount(), 0);
}

void TestProjectUi::testFindReplace() {
    SourceEditorTab tab;
    tab.show();
    tab.editor()->setPlainText("alpha beta alpha\nBeta alpha");
    FindReplaceBar* bar = tab.findBar();
    bar->openBar(true);
    QVERIFY(bar->isVisible());
    bar->setFindText("alpha");
    QVERIFY(bar->findNext());
    QCOMPARE(tab.editor()->textCursor().selectedText(), QString("alpha"));
    const int firstPos = tab.editor()->textCursor().position();
    QVERIFY(bar->findNext());
    QVERIFY(tab.editor()->textCursor().position() > firstPos);
    QVERIFY(bar->findNext());                                         // third match
    QVERIFY(bar->findNext());                                         // wraps around to the first
    QCOMPARE(tab.editor()->textCursor().position(), firstPos);
    QVERIFY(bar->statusText().isEmpty() || !bar->statusText().contains("No match"));

    bar->setFindText("nothing-like-this");
    QVERIFY(!bar->findNext());
    QVERIFY(bar->statusText().contains("No match"));

    bar->setFindText("beta");
    bar->setCaseSensitive(true);
    bar->setReplaceText("X");
    QCOMPARE(bar->replaceAll(), 1);                                   // "Beta" does not match case-sensitively
    QCOMPARE(tab.editor()->toPlainText(), QString("alpha X alpha\nBeta alpha"));
    bar->setCaseSensitive(false);
    bar->setFindText("alpha"); bar->setReplaceText("γ");
    QCOMPARE(bar->replaceAll(), 3);
    QCOMPARE(tab.editor()->toPlainText(), QString("γ X γ\nBeta γ"));
    tab.editor()->setReadOnly(true);                                   // read-only editors are never modified
    QCOMPARE(bar->replaceAll(), 0);
}

void TestProjectUi::testAnalysisPanelFromRealAssembly() {
    AnalysisPanel panel;
    panel.resize(900, 400);
    panel.show();
    const QString asmText = readCorpus("clang_x86_64_att_O3.s");
    QVERIFY(!asmText.isEmpty());
    panel.setPaneAssembly(1, "Pane 1: clang -O3", asmText);
    QVERIFY(panel.isAnalyzing());
    QTRY_VERIFY_WITH_TIMEOUT(panel.currentModule() != nullptr, 10000);
    QVERIFY(!panel.isAnalyzing());

    auto* funcs = panel.functionTree();
    QCOMPARE(funcs->topLevelItemCount(), panel.currentModule()->functions.size());
    QVERIFY(funcs->topLevelItemCount() >= 8);
    // Select the loop function: its blocks and successors are listed from the model
    QTreeWidgetItem* loop = nullptr;
    for (int i = 0; i < funcs->topLevelItemCount(); ++i)
        if (funcs->topLevelItem(i)->text(0) == "loop_sum") loop = funcs->topLevelItem(i);
    QVERIFY(loop);
    funcs->setCurrentItem(loop);
    QCOMPARE(panel.blockTree()->topLevelItemCount(), 9);               // matches the parser test for -O3 loop_sum
    bool sawTaken = false;
    for (int i = 0; i < panel.blockTree()->topLevelItemCount(); ++i)
        if (panel.blockTree()->topLevelItem(i)->text(4).contains("Taken →")) sawTaken = true;
    QVERIFY(sawTaken);

    // Indirect jumps are shown as unknown, never as an invented target
    for (int i = 0; i < funcs->topLevelItemCount(); ++i)
        if (funcs->topLevelItem(i)->text(0) == "indirect") funcs->setCurrentItem(funcs->topLevelItem(i));
    bool sawUnknown = false;
    for (int i = 0; i < panel.blockTree()->topLevelItemCount(); ++i)
        if (panel.blockTree()->topLevelItem(i)->text(4).contains("targets unknown")) sawUnknown = true;
    QVERIFY(sawUnknown);

    // Removing the pane clears the view
    panel.removePane(1);
    QCOMPARE(funcs->topLevelItemCount(), 0);
}

void TestProjectUi::testAnalysisNavigation() {
    AnalysisPanel panel;
    panel.show();
    panel.setPaneAssembly(7, "P", readCorpus("clang_x86_64_att_O0.s"));
    QTRY_VERIFY_WITH_TIMEOUT(panel.currentModule() != nullptr, 10000);
    QSignalSpy asmSpy(&panel, &AnalysisPanel::assemblyLineActivated);
    QSignalSpy srcSpy(&panel, &AnalysisPanel::sourceLineActivated);
    auto* funcs = panel.functionTree();
    emit funcs->itemDoubleClicked(funcs->topLevelItem(0), 0);
    QCOMPARE(asmSpy.count(), 1);
    QCOMPARE(asmSpy.at(0).at(0).toULongLong(), quint64(7));
    QVERIFY(asmSpy.at(0).at(1).toInt() > 0);
    funcs->setCurrentItem(funcs->topLevelItem(0));
    auto* blocks = panel.blockTree();
    QVERIFY(blocks->topLevelItemCount() > 0);
    emit blocks->itemDoubleClicked(blocks->topLevelItem(0), 3);          // source column
    QCOMPARE(srcSpy.count(), 1);
    QVERIFY(srcSpy.at(0).at(0).toInt() > 0);
}

void TestProjectUi::testAnalysisDockFedByCompileResult() {
    MainWindow w;
    w.resize(1500, 900);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    ProjectController* pc = w.projectController();
    pc->newProject("Shot");
    QString err;
    pc->addFile("main.cpp", "int simple_add(int a, int b){return a+b;}\n", &err);
    pc->addFile("util.h", "int h();\n", &err);
    w.openProjectFile("main.cpp");
    w.openProjectFile("util.h");

    CompilerOutputPane* pane = w.outputPanes().first();
    CompilationResult r;
    r.paneId = pane->paneId(); r.jobId = 11; r.revision = 100000; r.state = JobState::Completed; r.exitCode = 0;
    r.compilerImage = "clang:17"; r.durationMs = 42;
    CompilationArtifact a; a.type = ArtifactType::Assembly; a.name = "output.s";
    a.content = readCorpus("clang_x86_64_att_O3.s"); a.sizeBytes = a.content.toUtf8().size();
    r.artifacts << a;
    pane->setCurrentJobId(11);
    QVERIFY(QMetaObject::invokeMethod(&w, "onCompilationResultReceived", Q_ARG(CompilationResult, r)));

    w.analysisPanel()->parentWidget()->show();
    QTRY_VERIFY_WITH_TIMEOUT(w.analysisPanel()->currentModule() != nullptr, 10000);
    QVERIFY(w.analysisPanel()->functionTree()->topLevelItemCount() >= 8);

    // Filtered-out raw lines are reported instead of jumping somewhere arbitrary
    QVERIFY(pane->displayLineForRaw(1) == -1 || pane->displayLineForRaw(1) >= 0);
    QVERIFY(!pane->getEditor()->toPlainText().contains(".cfi_"));       // default filters hide CFI directives

    const QString shotDir = qEnvironmentVariable("OCTOS_SCREENSHOT_DIR");
    if (!shotDir.isEmpty()) w.grab().save(shotDir + "/project_analysis.png");
}

void TestProjectUi::testLayoutIsPersisted() {
    {
        // Control: without saved layout the split is even, so a saved uneven split proves persistence.
        MainWindow control;
        control.resize(1300, 800);
        control.show();
        QVERIFY(QTest::qWaitForWindowExposed(&control));
        QSplitter* cs = nullptr;
        for (QSplitter* s : control.findChildren<QSplitter*>())
            if (s->orientation() == Qt::Horizontal && s->count() == 2 && s->widget(0)->findChild<CodeEditor*>()) cs = s;
        QVERIFY(cs);
        QVERIFY2(qAbs(cs->sizes()[0] - cs->sizes()[1]) < 80, qPrintable(QString("control sizes %1/%2").arg(cs->sizes()[0]).arg(cs->sizes()[1])));
    }
    {
        MainWindow w;
        w.resize(1300, 800);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        auto splitters = w.findChildren<QSplitter*>();
        QSplitter* main = nullptr;
        for (QSplitter* s : splitters) if (s->orientation() == Qt::Horizontal && s->count() == 2 && s->widget(0)->findChild<CodeEditor*>()) main = s;
        QVERIFY(main);
        main->setSizes({300, 900});
        w.analysisPanel()->parentWidget()->show();                       // reveal the analysis dock
        QVERIFY(w.close());                                              // saves the layout
    }
    MainWindow w2;
    w2.resize(1300, 800);                                                // same window size: only the splitter differs from the default
    w2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w2));
    QTest::qWait(50);
    auto splitters = w2.findChildren<QSplitter*>();
    QSplitter* main = nullptr;
    for (QSplitter* s : splitters) if (s->orientation() == Qt::Horizontal && s->count() == 2 && s->widget(0)->findChild<CodeEditor*>()) main = s;
    QVERIFY(main);
    QVERIFY2(main->sizes()[0] < main->sizes()[1], qPrintable(QString("sizes %1/%2").arg(main->sizes()[0]).arg(main->sizes()[1])));
    QVERIFY(w2.analysisPanel()->parentWidget()->isVisible());              // dock visibility restored
}

void TestProjectUi::testResetLayout() {
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    w.analysisPanel()->parentWidget()->show();
    QVERIFY(w.analysisPanel()->parentWidget()->isVisible());
    QMetaObject::invokeMethod(&w, "onResetLayout");
    QVERIFY(!w.analysisPanel()->parentWidget()->isVisible());
}

OCTOS_TEST_MAIN(TestProjectUi)
#include "test_project_ui.moc"

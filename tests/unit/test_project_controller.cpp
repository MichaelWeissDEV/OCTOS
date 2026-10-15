#include "../TestSupport.h"
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFile>
#include "managers/ProjectController.h"

class TestProjectController : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void testNewProjectAndFiles();
    void testDirtyStatePerFile();
    void testSaveSingleFileWritesOnlyThatFile();
    void testSaveAndReopen();
    void testSaveProjectRequiresPath();
    void testAddRemoveValidation();
    void testRemoveFileCleansTargets();
    void testTargetsProfilesAndRequest();
    void testFailedSaveKeepsDirtyState();
    void testCloseAndHelpers();
};

static ProjectController* makeProject(QObject* parent, const QString& path = QString()) {
    auto* pc = new ProjectController(parent);
    pc->newProject("P");
    QString err;
    // Real calls, never inside Q_ASSERT (compiled out in Release builds).
    const bool a = pc->addFile("main.cpp", "int main(){return 0;}", &err);
    const bool b = pc->addFile("util.h", "int h();", &err);
    const bool c = path.isEmpty() || pc->saveProjectAs(path, &err);
    if (!a || !b || !c) qFatal("test fixture could not be created: %s", qPrintable(err));
    return pc;
}

void TestProjectController::testNewProjectAndFiles() {
    ProjectController pc;
    QVERIFY(!pc.isOpen());
    QSignalSpy changed(&pc, &ProjectController::projectChanged);
    QSignalSpy added(&pc, &ProjectController::fileAdded);
    pc.newProject("Demo");
    QVERIFY(pc.isOpen());
    QVERIFY(!pc.isDirty());                       // an empty new project is clean
    QString err;
    QVERIFY(pc.addFile("src/a.cpp", "x", &err));
    QVERIFY(pc.addFile("src/a.h", "y", &err));
    QCOMPARE(added.count(), 2);
    QVERIFY(changed.count() >= 3);
    QCOMPARE(pc.filePaths(), QStringList({"src/a.cpp", "src/a.h"}));
    QVERIFY(pc.workspace().findSourceFile("src/a.h").isHeader);          // recognised by extension
    QVERIFY(!pc.workspace().findSourceFile("src/a.cpp").isHeader);
    QCOMPARE(pc.workspace().findSourceFile("src/a.cpp").language, Language::Cpp);
    QCOMPARE(ProjectController::languageForPath("lib.rs"), Language::Rust);
    QCOMPARE(ProjectController::languageForPath("m.py"), Language::Python);
}

void TestProjectController::testDirtyStatePerFile() {
    QTemporaryDir dir;
    ProjectController* pc = makeProject(this, dir.path() + "/p.octos");
    QVERIFY(!pc->isDirty());
    QSignalSpy fileDirty(pc, &ProjectController::fileDirtyChanged);
    QSignalSpy anyDirty(pc, &ProjectController::dirtyChanged);

    pc->setFileContent("main.cpp", "int main(){return 1;}");
    QVERIFY(pc->isFileDirty("main.cpp"));
    QVERIFY(!pc->isFileDirty("util.h"));
    QVERIFY(pc->isDirty());
    QCOMPARE(fileDirty.count(), 1);
    QCOMPARE(fileDirty.at(0).at(0).toString(), QString("main.cpp"));
    QCOMPARE(fileDirty.at(0).at(1).toBool(), true);
    QCOMPARE(anyDirty.count(), 1);

    pc->setFileContent("main.cpp", "int main(){return 1;}");               // same content: no new signal
    QCOMPARE(fileDirty.count(), 1);
    pc->setFileContent("main.cpp", "int main(){return 0;}");               // back to the saved text: clean again
    QVERIFY(!pc->isFileDirty("main.cpp"));
    QVERIFY(!pc->isDirty());
    QCOMPARE(anyDirty.last().at(0).toBool(), false);
    pc->setFileContent("ghost.cpp", "x");                                    // unknown file: ignored
    QVERIFY(!pc->workspace().hasSourceFile("ghost.cpp"));
    delete pc;
}

void TestProjectController::testSaveSingleFileWritesOnlyThatFile() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/p.octos";
    ProjectController* pc = makeProject(this, path);
    pc->setFileContent("main.cpp", "// edited main");
    pc->setFileContent("util.h", "// edited header");
    QCOMPARE(pc->dirtyFiles().size(), 2);

    QString err;
    QVERIFY2(pc->saveFile("main.cpp", &err), qPrintable(err));
    QVERIFY(!pc->isFileDirty("main.cpp"));
    QVERIFY(pc->isFileDirty("util.h"));                                      // untouched by the single-file save
    QVERIFY(pc->isDirty());

    // On disk: new main.cpp, OLD util.h
    Workspace onDisk = Workspace::loadFromFile(path, &err);
    QVERIFY2(err.isEmpty(), qPrintable(err));
    QCOMPARE(onDisk.findSourceFile("main.cpp").content, QString("// edited main"));
    QCOMPARE(onDisk.findSourceFile("util.h").content, QString("int h();"));

    QVERIFY(pc->saveProject(&err));
    QVERIFY(!pc->isDirty());
    onDisk = Workspace::loadFromFile(path, &err);
    QCOMPARE(onDisk.findSourceFile("util.h").content, QString("// edited header"));
    delete pc;
}

void TestProjectController::testSaveAndReopen() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/p.octos";
    ProjectController* pc = makeProject(this, path);
    CompilerProfile prof; prof.id = "o3"; prof.name = "O3"; prof.compilerImage = "gcc:13"; prof.optimization = "-O3";
    QVERIFY(pc->addOrUpdateProfile(prof));
    QVERIFY(pc->ensureTargetForMain("app", "main.cpp"));
    QVERIFY(pc->setActiveProfile("o3"));
    QVERIFY(pc->setActiveTarget("app"));
    QVERIFY(pc->isDirty());                                                  // structure changes count
    QString err;
    QVERIFY(pc->saveProject(&err));
    delete pc;

    ProjectController reopened;
    QVERIFY2(reopened.openProject(path, &err), qPrintable(err));
    QCOMPARE(reopened.workspace().name(), QString("P"));
    QCOMPARE(reopened.workspace().configuration().activeProfileId, QString("o3"));
    QCOMPARE(reopened.workspace().findTarget("app")->sourceFiles, QStringList({"main.cpp", "util.h"}));
    QVERIFY(!reopened.isDirty());
    QVERIFY(!reopened.openProject(dir.path() + "/missing.octos", &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(reopened.isOpen());                                              // a failed open keeps the current project
}

void TestProjectController::testSaveProjectRequiresPath() {
    ProjectController* pc = makeProject(this);
    QString err;
    QVERIFY(!pc->saveProject(&err));
    QVERIFY(err.contains("Save As"));
    QVERIFY(!pc->saveFile("main.cpp", &err));
    QVERIFY(pc->isDirty());
    delete pc;
}

void TestProjectController::testAddRemoveValidation() {
    ProjectController* pc = makeProject(this);
    QString err;
    QVERIFY(!pc->addFile("../escape.cpp", "x", &err));
    QVERIFY(err.contains("valid project-relative path"));
    QVERIFY(!pc->addFile("/abs.cpp", "x", &err));
    QVERIFY(!pc->addFile("main.cpp", "dup", &err));
    QVERIFY(err.contains("already exists"));
    QVERIFY(pc->addFile("sub//dir/./x.cpp", "x", &err));                     // normalised
    QVERIFY(pc->workspace().hasSourceFile("sub/dir/x.cpp"));
    QVERIFY(!pc->removeFile("nope.cpp", &err));
    QVERIFY(pc->removeFile("sub/dir/x.cpp", &err));
    delete pc;
}

void TestProjectController::testRemoveFileCleansTargets() {
    ProjectController* pc = makeProject(this);
    QVERIFY(pc->ensureTargetForMain("app", "main.cpp"));
    QString err;
    QVERIFY(pc->removeFile("util.h", &err));
    QCOMPARE(pc->workspace().findTarget("app")->sourceFiles, QStringList({"main.cpp"}));
    QVERIFY(pc->removeFile("main.cpp", &err));
    QVERIFY(pc->workspace().findTarget("app")->mainSourceFile.isEmpty());
    QVERIFY(pc->workspace().validate().isEmpty());                           // no dangling references
    delete pc;
}

void TestProjectController::testTargetsProfilesAndRequest() {
    ProjectController* pc = makeProject(this);
    CompilerProfile p0; p0.id = "o0"; p0.name = "O0"; p0.compilerImage = "gcc:13"; p0.optimization = "-O0";
    CompilerProfile p3 = p0; p3.id = "o3"; p3.name = "O3"; p3.optimization = "-O3";
    QVERIFY(pc->addOrUpdateProfile(p0));
    QVERIFY(pc->addOrUpdateProfile(p3));
    QVERIFY(!pc->setActiveProfile("nope"));
    QVERIFY(pc->ensureTargetForMain("app", "main.cpp"));
    QVERIFY(!pc->ensureTargetForMain("x", "missing.cpp"));
    CompilationRequest a, b;
    QString err;
    QVERIFY2(pc->workspace().buildRequest("o0", "app", &a, &err), qPrintable(err));
    QVERIFY2(pc->workspace().buildRequest("o3", "app", &b, &err), qPrintable(err));
    QCOMPARE(a.sourceCode, b.sourceCode);
    QCOMPARE(a.userArgs, QString("-O0"));
    QCOMPARE(b.userArgs, QString("-O3"));
    QCOMPARE(a.extraFiles.size(), 1);                                        // the header travels along
    QVERIFY(pc->removeProfile("o3"));
    QVERIFY(!pc->removeProfile("o3"));
    delete pc;
}

void TestProjectController::testFailedSaveKeepsDirtyState() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/p.octos";
    ProjectController* pc = makeProject(this, path);
    pc->setFileContent("main.cpp", "// changed");
    QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner);
    QString err;
    const bool ok = pc->saveProject(&err);
    QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    if (ok) QSKIP("running with privileges that ignore directory permissions", SkipAll);
    QVERIFY(!err.isEmpty());
    QVERIFY(pc->isFileDirty("main.cpp"));                                    // the user's work is still marked unsaved
    QVERIFY(pc->isDirty());
    QVERIFY(pc->saveProject(&err));                                          // and can be saved once the problem is gone
    QVERIFY(!pc->isDirty());
    delete pc;
}

void TestProjectController::testCloseAndHelpers() {
    ProjectController* pc = makeProject(this);
    QSignalSpy changed(pc, &ProjectController::projectChanged);
    pc->closeProject();
    QVERIFY(!pc->isOpen());
    QVERIFY(!pc->isDirty());
    QCOMPARE(changed.count(), 1);
    QVERIFY(pc->filePaths().isEmpty());
    QVERIFY(ProjectController::looksLikeHeader("a/b.HPP"));
    QVERIFY(!ProjectController::looksLikeHeader("a/b.cpp"));
    delete pc;
}

OCTOS_TEST_MAIN(TestProjectController)
#include "test_project_controller.moc"

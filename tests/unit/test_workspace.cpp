#include "../TestSupport.h"
#include <QTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include "core/WorkspaceModel.h"
#include "core/ProfileStore.h"
#include "backend/DockerExecutionBackend.h"

namespace {
QPair<QString, QString> ef(const char* a, const char* b) { return {QString(a), QString(b)}; }
Workspace sample() {
    Workspace ws("Demo");
    SourceFile main; main.path = "src/main.cpp"; main.content = "#include \"util.h\"\nint main(){return helper();}\n"; main.language = Language::Cpp;
    SourceFile hdr; hdr.path = "src/util.h"; hdr.content = "int helper();\n"; hdr.isHeader = true;
    ws.addSourceFile(main); ws.addSourceFile(hdr);
    CompilerProfile o0; o0.id = "gcc-o0"; o0.name = "GCC -O0"; o0.compilerImage = "gcc:13"; o0.optimization = "-O0";
    CompilerProfile o3 = o0; o3.id = "gcc-o3"; o3.name = "GCC -O3"; o3.optimization = "-O3"; o3.userFlags = "-Wall";
    ws.addProfile(o0); ws.addProfile(o3);
    BuildTarget t; t.name = "app"; t.mainSourceFile = "src/main.cpp"; t.sourceFiles = {"src/main.cpp", "src/util.h"}; t.includePaths = {"src"};
    ws.addTarget(t);
    CompilationConfiguration c; c.activeProfileId = "gcc-o3"; c.activeTargetName = "app"; c.environmentVars["CXXFLAGS"] = "x";
    ws.setConfiguration(c);
    return ws;
}
QByteArray mutateJson(const QByteArray& json, const std::function<void(QJsonObject&)>& f) {
    QJsonObject o = QJsonDocument::fromJson(json).object();
    f(o);
    return QJsonDocument(o).toJson();
}
}

class TestWorkspace : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void testRoundTripKeepsEverything();
    void testVersionGate();
    void testMalformedAndHostileJson();
    void testMissingOptionalFieldsAreTolerated();
    void testDuplicateIdsAndReferencesRejected();
    void testPathValidation();
    void testMaterializeStaysInsideRoot();
    void testMaterializeRefusesSymlinkEscape();
    void testAtomicSaveKeepsOldFileOnFailure();
    void testInvalidWorkspaceIsNeverSaved();
    void testRelativePathsResolveAgainstProjectFile();
    void testBuildRequestUsesProfileAndHeaders();
    void testProfileStorePersistsBetweenSessions();
    void testProfileComparisonRequestsDifferOnlyInProfile();
    void testBackendRejectsTraversalInExtraFiles();
    void testBackendDeliversExtraFiles();
    void testJsonFuzzDoesNotCrash();
};

void TestWorkspace::testRoundTripKeepsEverything() {
    Workspace ws = sample();
    QString err;
    Workspace back = Workspace::fromJson(ws.toJson(), &err);
    QVERIFY2(err.isEmpty(), qPrintable(err));
    QCOMPARE(back.name(), QString("Demo"));
    QCOMPARE(back.sourceFiles().size(), 2);
    QCOMPARE(back.profiles().size(), 2);
    QCOMPARE(back.findProfile("gcc-o3")->optimization, QString("-O3"));
    QCOMPARE(back.findProfile("gcc-o3")->effectiveFlags(), QString("-O3 -Wall"));
    QCOMPARE(back.targets().first().includePaths, QStringList({"src"}));
    QCOMPARE(back.configuration().activeProfileId, QString("gcc-o3"));
    QVERIFY(back.validate().isEmpty());
}

void TestWorkspace::testVersionGate() {
    const QByteArray good = sample().toJson();
    QString err;
    // A future format must be rejected with a clear message, never half-read
    auto future = mutateJson(good, [](QJsonObject& o) { o["format_version"] = "octos-project-v2"; });
    Workspace::fromJson(future, &err);
    QVERIFY(err.contains("newer OCTOS"));
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) { o["format_version"] = "something-else"; }), &err);
    QVERIFY(err.contains("Unrecognised"));
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) { o["format_version"] = 1; }), &err);
    QVERIFY(err.contains("must be a string"));
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) { o["format_version"] = "octos-project-v0"; }), &err);
    QVERIFY(!err.isEmpty());
    // missing version: accepted with a warning
    err.clear();
    Workspace noVer = Workspace::fromJson(mutateJson(good, [](QJsonObject& o) { o.remove("format_version"); }), &err);
    QVERIFY2(err.isEmpty(), qPrintable(err));
    QVERIFY(!noVer.warnings().isEmpty());
    QCOMPARE(Workspace::parseFormatVersion("octos-project-v7"), 7);
    QCOMPARE(Workspace::parseFormatVersion("octos-project-vX"), -1);
}

void TestWorkspace::testMalformedAndHostileJson() {
    QString err;
    for (const QByteArray& bad : {QByteArray("{not json"), QByteArray(""), QByteArray("[]"), QByteArray("42"), QByteArray("null")}) {
        err.clear();
        Workspace::fromJson(bad, &err);
        QVERIFY2(!err.isEmpty(), bad.constData());
    }
    const QByteArray good = sample().toJson();
    // wrong types in structural positions
    for (const char* key : {"source_files", "profiles", "targets"}) {
        err.clear();
        Workspace::fromJson(mutateJson(good, [key](QJsonObject& o) { o[key] = "oops"; }), &err);
        QVERIFY2(!err.isEmpty(), key);
    }
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) { o["source_files"] = QJsonArray{1, 2}; }), &err);
    QVERIFY(!err.isEmpty());
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) { o["name"] = 5; }), &err);
    QVERIFY(!err.isEmpty());
    // oversized input is refused before parsing
    err.clear();
    QByteArray huge(static_cast<int>(Workspace::kMaxJsonBytes / 1024 / 1024 + 1) * 1024 * 1024, ' ');
    Workspace::fromJson(huge, &err);
    QVERIFY(err.contains("too large"));
}

void TestWorkspace::testMissingOptionalFieldsAreTolerated() {
    const QByteArray minimal = R"({"format_version":"octos-project-v1","name":"m",
        "source_files":[{"path":"a.c"}],"profiles":[{"id":"p","name":"P"}],"targets":[{"name":"t","main_source_file":"a.c"}]})";
    QString err;
    Workspace ws = Workspace::fromJson(minimal, &err);
    QVERIFY2(err.isEmpty(), qPrintable(err));
    QCOMPARE(ws.sourceFiles().first().content, QString());
    QCOMPARE(ws.findProfile("p")->outputFormat, QString("assembly"));
    QCOMPARE(ws.findProfile("p")->language, Language::Cpp);
}

void TestWorkspace::testDuplicateIdsAndReferencesRejected() {
    const QByteArray good = sample().toJson();
    QString err;
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) {
        QJsonArray p = o["profiles"].toArray(); p.append(p.first()); o["profiles"] = p; }), &err);
    QVERIFY2(err.contains("Duplicate profile id"), qPrintable(err));
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) {
        QJsonArray f = o["source_files"].toArray(); f.append(f.first()); o["source_files"] = f; }), &err);
    QVERIFY2(err.contains("Duplicate source path"), qPrintable(err));   // no longer silently merged
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) {
        QJsonArray t = o["targets"].toArray(); t.append(t.first()); o["targets"] = t; }), &err);
    QVERIFY2(err.contains("Duplicate target name"), qPrintable(err));
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) {
        QJsonArray t = o["targets"].toArray(); QJsonObject t0 = t[0].toObject(); t0["main_source_file"] = "missing.cpp"; t[0] = t0; o["targets"] = t; }), &err);
    QVERIFY2(err.contains("not part of the project"), qPrintable(err));
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) {
        QJsonObject c = o["configuration"].toObject(); c["active_profile_id"] = "ghost"; o["configuration"] = c; }), &err);
    QVERIFY2(err.contains("Active profile"), qPrintable(err));
    err.clear();
    Workspace::fromJson(mutateJson(good, [](QJsonObject& o) {
        QJsonObject c = o["configuration"].toObject(); QJsonObject e; e["BAD NAME"] = "1"; c["environment_vars"] = e; o["configuration"] = c; }), &err);
    QVERIFY2(err.contains("environment variable"), qPrintable(err));
}

void TestWorkspace::testPathValidation() {
    bool ok = false;
    QCOMPARE(Workspace::normalizeRelativePath("a//b/./c.cpp", &ok), QString("a/b/c.cpp")); QVERIFY(ok);
    QCOMPARE(Workspace::normalizeRelativePath("a/b/../c.cpp", &ok), QString("a/c.cpp")); QVERIFY(ok);
    for (const char* bad : {"../x", "a/../../x", "/etc/passwd", "~/x", "C:/x", "C:\\x", "a\\b", "", ".", "a/..", "a/../..", "x\1y"}) {
        Workspace::normalizeRelativePath(bad, &ok);
        QVERIFY2(!ok, bad);
    }
    QString withNul = QString("a") + QChar('\0') + "b";
    Workspace::normalizeRelativePath(withNul, &ok);
    QVERIFY(!ok);
    QVERIFY(Workspace::isSafeRelativePath("src/a.cpp"));
    QVERIFY(!Workspace::isSafeRelativePath("src//a.cpp"));        // must already be normalised
    // include paths are validated too
    Workspace ws = sample();
    BuildTarget t = *ws.findTarget("app");
    t.includePaths = {"/usr/include"};
    ws.addTarget(t);
    QVERIFY(!ws.validate().isEmpty());
}

void TestWorkspace::testMaterializeStaysInsideRoot() {
    QTemporaryDir dir;
    Workspace ws = sample();
    QString err;
    QVERIFY2(ws.materializeTo(dir.path() + "/proj", &err), qPrintable(err));
    QVERIFY(QFile::exists(dir.path() + "/proj/src/main.cpp"));
    QVERIFY(QFile::exists(dir.path() + "/proj/src/util.h"));
    // a traversal path cannot even get into a workspace that materialises
    Workspace evil("evil");
    SourceFile sf; sf.path = "../../escape.txt"; sf.content = "x";
    evil.addSourceFile(sf);
    QVERIFY(!evil.materializeTo(dir.path() + "/proj2", &err));
    QVERIFY(!QFile::exists(dir.path() + "/escape.txt"));
    QVERIFY(Workspace::resolveInside(dir.path(), "../x", &err).isEmpty());
}

void TestWorkspace::testMaterializeRefusesSymlinkEscape() {
    QTemporaryDir dir, outside;
    QVERIFY(QDir().mkpath(dir.path() + "/proj"));
    // A pre-existing symlink inside the project directory points outside of it.
    QVERIFY(QFile::link(outside.path(), dir.path() + "/proj/src"));
    Workspace ws = sample();
    QString err;
    QVERIFY(!ws.materializeTo(dir.path() + "/proj", &err));
    QVERIFY2(err.contains("escapes"), qPrintable(err));
    QVERIFY(QDir(outside.path()).entryList(QDir::Files).isEmpty());       // nothing was written outside
}

void TestWorkspace::testAtomicSaveKeepsOldFileOnFailure() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/p.octos";
    Workspace ws = sample();
    QString err;
    QVERIFY2(ws.saveToFile(path, &err), qPrintable(err));
    QFile f(path); QVERIFY(f.open(QIODevice::ReadOnly)); const QByteArray before = f.readAll(); f.close();

    // Make the directory read-only: the temporary file cannot be created, the save must fail cleanly.
    QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner);
    Workspace changed = sample();
    changed.setName("Changed");
    const bool saved = changed.saveToFile(path, &err);
    QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    if (saved) QSKIP("running with privileges that ignore directory permissions", SkipAll);
    QVERIFY(!err.isEmpty());
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), before);                     // previous valid project is intact
    // and no stray temp files were left behind
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 1);
}

void TestWorkspace::testInvalidWorkspaceIsNeverSaved() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/p.octos";
    QString err;
    QVERIFY(sample().saveToFile(path, &err));
    QFile f(path); QVERIFY(f.open(QIODevice::ReadOnly)); const QByteArray before = f.readAll(); f.close();
    Workspace bad = sample();
    SourceFile sf; sf.path = "../evil.cpp"; bad.addSourceFile(sf);
    QVERIFY(!bad.saveToFile(path, &err));
    QVERIFY(err.contains("invalid"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), before);
    // And a file written by someone else with a bad path is refused on load
    QFile w(dir.path() + "/hostile.octos"); QVERIFY(w.open(QIODevice::WriteOnly));
    w.write(mutateJson(sample().toJson(), [](QJsonObject& o) {
        QJsonArray a = o["source_files"].toArray(); QJsonObject x; x["path"] = "../../../tmp/x"; a.append(x); o["source_files"] = a; }));
    w.close();
    QString lerr;
    Workspace::loadFromFile(dir.path() + "/hostile.octos", &lerr);
    QVERIFY(lerr.contains("Unsafe"));
}

void TestWorkspace::testRelativePathsResolveAgainstProjectFile() {
    QTemporaryDir dir;
    QVERIFY(QDir().mkpath(dir.path() + "/work"));
    const QString path = dir.path() + "/work/p.octos";
    QString err;
    QVERIFY(sample().saveToFile(path, &err));
    Workspace loaded = Workspace::loadFromFile(path, &err);
    QVERIFY2(err.isEmpty(), qPrintable(err));
    QCOMPARE(QDir(loaded.projectDirectory()).canonicalPath(), QDir(dir.path() + "/work").canonicalPath());
    const QString abs = Workspace::resolveInside(loaded.projectDirectory(), "src/main.cpp", &err);
    QVERIFY(abs.endsWith("/work/src/main.cpp"));
}

void TestWorkspace::testBuildRequestUsesProfileAndHeaders() {
    Workspace ws = sample();
    CompilationRequest req;
    QString err;
    QVERIFY2(ws.buildRequest("gcc-o3", "", &req, &err), qPrintable(err));
    QCOMPARE(req.compilerImage, QString("gcc:13"));
    QCOMPARE(req.userArgs, QString("-O3 -Wall"));
    QVERIFY(req.sourceCode.contains("helper"));
    QCOMPARE(req.extraFiles.size(), 1);
    QCOMPARE(req.extraFiles[0].first, QString("src/util.h"));
    QVERIFY(!ws.buildRequest("nope", "", &req, &err));
    QVERIFY(!ws.buildRequest("gcc-o3", "nope", &req, &err));
}

void TestWorkspace::testProfileStorePersistsBetweenSessions() {
    {
        ProfileStore s = ProfileStore::load();
        const auto existing = s.profiles();
        for (const auto& p : existing) s.remove(p.id);
        CompilerProfile a; a.id = s.makeUniqueId("GCC -O0"); a.name = "GCC -O0"; a.compilerImage = "gcc:13"; a.optimization = "-O0";
        QCOMPARE(a.id, QString("gcc-o0"));
        QVERIFY(s.add(a));
        CompilerProfile b = a; b.id = s.makeUniqueId("GCC -O3"); b.name = "GCC -O3"; b.optimization = "-O3";
        QVERIFY(s.add(b));
        CompilerProfile c = a; c.id = s.makeUniqueId("GCC -O0"); c.name = "GCC -O0 again";    // id collision gets a suffix
        QCOMPARE(c.id, QString("gcc-o0-2"));
        QVERIFY(s.save());
    }
    // "next session": a fresh store instance reads the same data back
    ProfileStore again = ProfileStore::load();
    QCOMPARE(again.profiles().size(), 2);
    QCOMPARE(again.find("gcc-o3")->optimization, QString("-O3"));
    QCOMPARE(again.find("gcc-o0")->effectiveFlags(), QString("-O0"));
    ProfileStore cleanup = again;
    cleanup.remove("gcc-o0"); cleanup.remove("gcc-o3"); cleanup.save();
    QString e;
    QVERIFY(!cleanup.add(CompilerProfile(), &e));                 // empty id/name rejected
}

void TestWorkspace::testProfileComparisonRequestsDifferOnlyInProfile() {
    CompilerProfile a; a.id = "a"; a.name = "A"; a.compilerImage = "gcc:13"; a.optimization = "-O0";
    CompilerProfile b = a; b.id = "b"; b.optimization = "-O3";
    CompilationRequest ra, rb;
    QVERIFY(ProfileStore::buildComparisonRequests(a, b, "int f(){return 1;}", &ra, &rb));
    QCOMPARE(ra.sourceCode, rb.sourceCode);
    QCOMPARE(ra.compilerImage, rb.compilerImage);
    QCOMPARE(ra.userArgs, QString("-O0"));
    QCOMPARE(rb.userArgs, QString("-O3"));
}

static QString writeFakeDocker(const QString& dir) {
    QFile f(dir + "/docker");
    if (!f.open(QIODevice::WriteOnly)) return QString();
    f.write(R"SH(#!/bin/sh
DIR=$(dirname "$0")
case "$1" in
  rm) exit 0;;
  run)
    in=""; out=""
    for a in "$@"; do
      case "$a" in *:/input:ro) in="${a%:/input:ro}";; *:/output:rw) out="${a%:/output:rw}";; esac
    done
    (cd "$in" && find . -type f | sort) > "$DIR/input_files.txt"
    echo ".text" > "$out/output.s"
    exit 0;;
esac
)SH");
    f.close();
    f.setPermissions(f.permissions() | QFile::ExeOwner | QFile::ExeUser);
    return dir + "/docker";
}

void TestWorkspace::testBackendRejectsTraversalInExtraFiles() {
    QTemporaryDir bin, victim;
    DockerExecutionConfig cfg; cfg.dockerProgram = writeFakeDocker(bin.path());
    DockerExecutionBackend be(cfg);
    CompilationRequest req; req.jobId = 1; req.language = Language::Cpp; req.compilerImage = "gcc:13";
    req.sourceCode = "int f();";
    req.extraFiles.append(ef("../../../../../../../../tmp/octos_traversal_probe", "pwned"));
    auto r = be.execute(req);
    QCOMPARE(r.state, JobState::Failed);
    QVERIFY(r.errorOutput.contains("Rejected"));
    QVERIFY(!QFile::exists("/tmp/octos_traversal_probe"));
    QVERIFY(!QFile::exists(bin.path() + "/input_files.txt"));      // docker never ran
    req.extraFiles.clear();
    req.extraFiles.append(ef("/etc/octos_abs", "x"));
    QCOMPARE(be.execute(req).state, JobState::Failed);
    req.extraFiles.clear();
    req.extraFiles.append(ef("source.cpp", "shadow the main file"));
    r = be.execute(req);
    QCOMPARE(r.state, JobState::Failed);
    QVERIFY(r.errorOutput.contains("collides"));
}

void TestWorkspace::testBackendDeliversExtraFiles() {
    QTemporaryDir bin;
    DockerExecutionConfig cfg; cfg.dockerProgram = writeFakeDocker(bin.path());
    DockerExecutionBackend be(cfg);
    CompilationRequest req; req.jobId = 2; req.language = Language::Cpp; req.compilerImage = "gcc:13";
    req.sourceCode = "#include \"inc/util.h\"\nint main(){return h();}";
    req.extraFiles.append(ef("inc/util.h", "int h();"));
    req.extraFiles.append(ef("inc/deep/more.h", "// more"));
    auto r = be.execute(req);
    QCOMPARE(r.state, JobState::Completed);
    QFile f(bin.path() + "/input_files.txt");
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QStringList files = QString::fromUtf8(f.readAll()).split('\n', Qt::SkipEmptyParts);
    QCOMPARE(files, QStringList({"./inc/deep/more.h", "./inc/util.h", "./source.cpp"}));
}

void TestWorkspace::testJsonFuzzDoesNotCrash() {
    QRandomGenerator rng(99);
    const QByteArray base = sample().toJson();
    int accepted = 0;
    for (int round = 0; round < 1500; ++round) {
        QByteArray data = base;
        const int edits = 1 + rng.bounded(6);
        for (int e = 0; e < edits; ++e) {
            const int pos = rng.bounded(data.size());
            switch (rng.bounded(4)) {
                case 0: data[pos] = char(rng.bounded(256)); break;
                case 1: data.remove(pos, 1 + rng.bounded(8)); break;
                case 2: data.insert(pos, QByteArray(1 + rng.bounded(8), char(rng.bounded(256)))); break;
                default: data.truncate(pos); break;
            }
            if (data.isEmpty()) data = "{}";
        }
        QString err;
        Workspace ws = Workspace::fromJson(data, &err);
        if (err.isEmpty()) {
            ++accepted;
            QVERIFY2(ws.validate().isEmpty(), "an accepted project must be valid");
        }
    }
    QVERIFY(accepted > 0);   // some mutations are harmless (whitespace, content text)
}

OCTOS_TEST_MAIN(TestWorkspace)
#include "test_workspace.moc"

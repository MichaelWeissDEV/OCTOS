// Backend behaviour tests driven by a scripted fake `docker` client, so they are
// deterministic and need neither Docker nor images.
#include "../TestSupport.h"
#include <QTest>
#include <QFile>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <atomic>
#include <thread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include "backend/DockerExecutionBackend.h"
#include "backend/DockerExecutionConfig.h"

class TestDockerBackend : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;
    QString m_script;

    void setMode(const QString& mode) {
        QFile f(m_dir.path() + "/mode");
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(mode.toUtf8());
    }
    QString callLog() const {
        QFile f(m_dir.path() + "/calls.log");
        return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
    }
    void clearLog() { QFile::remove(m_dir.path() + "/calls.log"); }
    DockerExecutionConfig cfg() const {
        DockerExecutionConfig c;
        c.dockerProgram = m_script;
        c.maxFileSizeBytes = 1024;
        c.maxOutputBytes = 1024;
        c.cleanupTimeoutMs = 2000;
        return c;
    }
    static CompilationRequest req(quint64 id) {
        CompilationRequest r;
        r.jobId = id; r.paneId = 1; r.revision = 1;
        r.language = Language::Cpp;
        r.compilerImage = "gcc:13";
        r.sourceCode = "int main(){return 0;}";
        r.timeoutMs = 10000;
        return r;
    }

private slots:
    void initTestCase();
    void init() { clearLog(); setMode("ok"); }

    void testSuccessRemovesContainer();
    void testCancelBeforeDockerStart();
    void testCancelDuringRun();
    void testCancelDuringSlowStart();
    void testClientExitsContainerStillThere();
    void testDaemonUnreachable();
    void testCleanupFailureIsTrackedAndRetried();
    void testRepeatedCleanupIsIdempotent();
    void testCancelNeverBlocksTheCaller();
    void testCancelAndCompletionRace();
    void testTruncatedArtifactKeepsFlag();
    void testOutputFloodIsStopped();
    void testOversizedSourceRejected();
    void testRunArgsAreHardened();
    void testArtifactsMetadataAndDiagnostics();
    void testArtifactPersistence();
    void testLlvmIrOutputKind();
    void testBoundedAppendAndUtf8();
    void testConfigSanitizingIsPerField();
};

void TestDockerBackend::initTestCase() {
    QVERIFY(m_dir.isValid());
    m_script = m_dir.path() + "/docker";
    QFile f(m_script);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"SH(#!/bin/sh
DIR=$(dirname "$0")
MODE=$(cat "$DIR/mode")
echo "$@" >> "$DIR/calls.log"
case "$1" in
  rm)
    case "$MODE" in
      cleanupfail) echo "Error: daemon busy" >&2; exit 1;;
      slowrm) sleep 2; exit 0;;
      nosuch) echo "Error response from daemon: No such container: x" >&2; exit 1;;
      *) exit 0;;
    esac;;
  run)
    out=""
    for a in "$@"; do case "$a" in *:/output:rw) out="${a%:/output:rw}";; esac; done
    case "$MODE" in
      ok|clientexit) echo ".text" > "$out/output.s"; echo "define i32 @f() { ret i32 0 }" > "$out/output.ll"; echo "compiler says hi"; exit 0;;
      slowstart) sleep 1; echo ".text" > "$out/output.s"; exit 0;;
      compilererror) echo "source.cpp:1:5: error: 'x' was not declared in this scope" >&2; exit 1;;
      daemondown) echo "Cannot connect to the Docker daemon at unix:///x. Is the docker daemon running?" >&2; exit 125;;
      bigartifact) head -c 3000 /dev/zero | tr '\0' 'a' > "$out/output.s"; exit 0;;
      flood) exec yes aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;;
      hang|slowrm) exec sleep 30;;
    esac;;
esac
exit 0
)SH");
    f.close();
    f.setPermissions(f.permissions() | QFile::ExeOwner | QFile::ExeUser);
    setMode("ok");
}

void TestDockerBackend::testSuccessRemovesContainer() {
    DockerExecutionBackend be(cfg());
    auto r = be.execute(req(1));
    QCOMPARE(r.state, JobState::Completed);
    QVERIFY(r.assemblyOutput.contains(".text"));
    QVERIFY(callLog().contains("rm -f octos_"));          // cleanup always happens
    QVERIFY(be.activeContainerNames().isEmpty());          // registry released after cleanup
    QVERIFY(be.leakedContainers().isEmpty());
    // Artifact lifecycle: in-memory, no stale host path, checksum + provenance filled.
    auto art = r.artifact(ArtifactType::Assembly);
    QVERIFY(!art.id.isEmpty());
    QVERIFY(art.hostTempPath.isEmpty());
    QVERIFY(!art.hasPersistentFile());
    QCOMPARE(art.containerPath, QString("/output/output.s"));
    QCOMPARE(art.compilationId, quint64(1));
    QVERIFY(!art.checksumSha256.isEmpty());
}

void TestDockerBackend::testCancelBeforeDockerStart() {
    DockerExecutionBackend be(cfg());
    auto r = be.execute(req(2), [] { return true; });
    QCOMPARE(r.state, JobState::Cancelled);
    QVERIFY(!callLog().contains("run "));                  // docker was never launched
    QVERIFY(be.activeContainerNames().isEmpty());
}

void TestDockerBackend::testCancelDuringRun() {
    setMode("hang");
    DockerExecutionBackend be(cfg());
    std::atomic<bool> stop{false};
    CompilationResult r;
    std::thread t([&] { r = be.execute(req(3), [&] { return stop.load(); }); });
    QTRY_VERIFY_WITH_TIMEOUT(!be.activeContainerNames().isEmpty(), 3000);
    QTest::qWait(150);
    QElapsedTimer timer; timer.start();
    stop = true;
    t.join();
    QVERIFY(timer.elapsed() < 3000);
    QCOMPARE(r.state, JobState::Cancelled);
    QVERIFY(callLog().contains("rm -f octos_"));
    QVERIFY(be.activeContainerNames().isEmpty());
}

void TestDockerBackend::testCancelDuringSlowStart() {
    setMode("slowstart");
    DockerExecutionBackend be(cfg());
    CompilationResult r;
    std::thread t([&] { r = be.execute(req(4)); });
    QTRY_VERIFY_WITH_TIMEOUT(!be.activeContainerNames().isEmpty(), 3000);
    be.cancel(4);                                          // cancel while the client is still starting
    t.join();
    QCOMPARE(r.state, JobState::Cancelled);
    QVERIFY(be.activeContainerNames().isEmpty());
    QVERIFY(be.leakedContainers().isEmpty());
    be.cancel(4);                                          // repeated cancel of a finished job: no-op
    QVERIFY(be.activeContainerNames().isEmpty());
}

void TestDockerBackend::testClientExitsContainerStillThere() {
    // The client returned 0, yet a container could still exist: removal must be attempted.
    setMode("clientexit");
    DockerExecutionBackend be(cfg());
    auto r = be.execute(req(5));
    QCOMPARE(r.state, JobState::Completed);
    QCOMPARE(callLog().count("rm -f octos_"), 1);
}

void TestDockerBackend::testDaemonUnreachable() {
    setMode("daemondown");
    DockerExecutionBackend be(cfg());
    auto r = be.execute(req(6));
    QCOMPARE(r.state, JobState::Failed);
    QCOMPARE(r.failureOrigin, FailureOrigin::Docker);
    QVERIFY(r.errorOutput.contains("Cannot connect to the Docker daemon"));
    QVERIFY(be.leakedContainers().isEmpty());              // nothing can exist => nothing "leaked"
    QVERIFY(!callLog().contains("rm -f"));
}

void TestDockerBackend::testCleanupFailureIsTrackedAndRetried() {
    setMode("cleanupfail");
    DockerExecutionBackend be(cfg());
    auto r = be.execute(req(7));
    QCOMPARE(r.state, JobState::Completed);
    QVERIFY(!r.warnings.filter("could not be removed").isEmpty());
    QCOMPARE(be.leakedContainers().size(), 1);             // never silently forgotten
    QVERIFY(be.activeContainerNames().isEmpty());
    QCOMPARE(be.retryLeakedCleanup(), 1);                  // still failing: still leaked
    setMode("ok");
    QCOMPARE(be.retryLeakedCleanup(), 0);
    QVERIFY(be.leakedContainers().isEmpty());
}

void TestDockerBackend::testRepeatedCleanupIsIdempotent() {
    const auto c = cfg();
    QVERIFY(DockerExecutionBackend::cleanupContainer("octos_1_1", c.dockerProgram, 2000));
    QVERIFY(DockerExecutionBackend::cleanupContainer("octos_1_1", c.dockerProgram, 2000));
    setMode("nosuch");                                     // "No such container" == already gone == success
    QVERIFY(DockerExecutionBackend::cleanupContainer("octos_1_1", c.dockerProgram, 2000));
    setMode("cleanupfail");                                // genuine failure must NOT be reported as success
    QVERIFY(!DockerExecutionBackend::cleanupContainer("octos_1_1", c.dockerProgram, 2000));
    // Containers not created by OCTOS are never touched.
    clearLog();
    QVERIFY(!DockerExecutionBackend::cleanupContainer("postgres", c.dockerProgram, 2000));
    QVERIFY(!DockerExecutionBackend::cleanupContainer("", c.dockerProgram, 2000));
    QVERIFY(callLog().isEmpty());
}

void TestDockerBackend::testCancelNeverBlocksTheCaller() {
    // `docker rm` takes 2 s here; cancel() is called from the GUI thread and must return at once.
    setMode("slowrm");
    DockerExecutionBackend be(cfg());
    CompilationResult r;
    std::thread t([&] { r = be.execute(req(20)); });
    QTRY_VERIFY_WITH_TIMEOUT(!be.activeContainerNames().isEmpty(), 3000);
    QTest::qWait(100);
    QElapsedTimer timer; timer.start();
    be.cancel(20);
    be.cancel(20);
    QVERIFY2(timer.elapsed() < 100, qPrintable(QString("cancel() blocked for %1 ms").arg(timer.elapsed())));
    t.join();                                   // the worker still performs the (slow) cleanup
    QCOMPARE(r.state, JobState::Cancelled);
    QVERIFY(callLog().contains("rm -f octos_"));
    QVERIFY(be.activeContainerNames().isEmpty());
}

void TestDockerBackend::testCancelAndCompletionRace() {
    for (int round = 0; round < 8; ++round) {
        setMode("ok");
        DockerExecutionBackend be(cfg());
        std::atomic<bool> flag{false};
        CompilationResult r;
        const quint64 id = 100 + round;
        std::thread worker([&] { r = be.execute(req(id), [&] { return flag.load(); }); });
        std::thread canceller([&] {
            QThread::msleep(round * 3);
            flag = true;
            be.cancel(id);
            be.cancel(id);
        });
        worker.join();
        canceller.join();
        QVERIFY2(r.state == JobState::Completed || r.state == JobState::Cancelled,
                 qPrintable(jobStateToString(r.state)));
        QVERIFY(be.activeContainerNames().isEmpty());
        QVERIFY(be.leakedContainers().isEmpty());
    }
}

void TestDockerBackend::testTruncatedArtifactKeepsFlag() {
    setMode("bigartifact");
    DockerExecutionBackend be(cfg());
    auto r = be.execute(req(8));
    QCOMPARE(r.state, JobState::Completed);
    QVERIFY(r.artifactTruncated);                          // regression: used to be reset by processOutput()
    QCOMPARE(r.assemblyOutput.toUtf8().size(), 1024);      // exactly the configured limit
    QVERIFY(r.artifact(ArtifactType::Assembly).truncated);
    QVERIFY(!r.warnings.filter("truncated").isEmpty());    // UI gets a warning
    QVERIFY(!r.isComplete());                              // never presented as a complete result
}

void TestDockerBackend::testOutputFloodIsStopped() {
    setMode("flood");
    DockerExecutionBackend be(cfg());
    QElapsedTimer t; t.start();
    auto r = be.execute(req(9));
    QVERIFY(t.elapsed() < 8000);
    QCOMPARE(r.state, JobState::Failed);
    QCOMPARE(r.failureOrigin, FailureOrigin::Limits);
    QVERIFY(be.activeContainerNames().isEmpty());
}

void TestDockerBackend::testOversizedSourceRejected() {
    DockerExecutionBackend be(cfg());
    auto q = req(10);
    q.sourceCode = QString(2 * 1024 * 1024, 'x');
    auto r = be.execute(q);
    QCOMPARE(r.state, JobState::Failed);
    QCOMPARE(r.failureOrigin, FailureOrigin::Limits);
    QVERIFY(!callLog().contains("run "));
}

void TestDockerBackend::testRunArgsAreHardened() {
    DockerExecutionConfig c;
    auto a = DockerExecutionBackend::buildDockerRunArgs(req(1), c, "octos_x_1", "/in", "/out", {"gcc"});
    QVERIFY(a.contains("--pull=never"));          // compiling must never trigger an image download
    QVERIFY(a.contains("--network") && a.contains("none"));
    QVERIFY(a.contains("--cap-drop=ALL"));
    QVERIFY(a.contains("no-new-privileges"));
    QVERIFY(a.contains("--memory-swap"));
    QVERIFY(a.contains("--ipc"));
    const int tmp = a.indexOf("--tmpfs");
    QVERIFY(tmp >= 0);
    QVERIFY(a[tmp + 1].contains("size=64m") && a[tmp + 1].contains("noexec"));
    QVERIFY(a.indexOf("--ulimit") >= 0);
    QVERIFY(a.contains("/in:/input:ro"));
}

void TestDockerBackend::testArtifactsMetadataAndDiagnostics() {
    DockerExecutionBackend be(cfg());
    auto r = be.execute(req(30));
    QCOMPARE(r.state, JobState::Completed);
    QVERIFY(r.hasArtifact(ArtifactType::Assembly));
    QVERIFY(r.hasArtifact(ArtifactType::RawOutput));
    QVERIFY(r.hasArtifact(ArtifactType::CompilerMetadata));
    QVERIFY(!r.compilerCommand.isEmpty());
    QVERIFY(r.compilerCommand.startsWith("g++"));                    // reproducible: the exact invocation is recorded
    auto meta = r.artifact(ArtifactType::CompilerMetadata);
    const auto doc = QJsonDocument::fromJson(meta.content.toUtf8()).object();
    QCOMPARE(doc["image"].toString(), QString("gcc:13"));
    QCOMPARE(doc["command"].toString(), r.compilerCommand);
    QCOMPARE(doc["network"].toString(), QString("none"));
    QCOMPARE(doc["exit_code"].toInt(), 0);
    // unique stable ids, provenance on every artifact, no stale host paths
    QSet<QString> ids;
    for (const auto& a : r.artifacts) {
        QVERIFY(!a.id.isEmpty());
        ids.insert(a.id);
        QCOMPARE(a.compilationId, quint64(30));
        QVERIFY(a.hostTempPath.isEmpty());
        QVERIFY(a.createdAt.isValid());
        QVERIFY(!a.checksumSha256.isEmpty());
        QVERIFY(!a.source.isEmpty());
    }
    QCOMPARE(ids.size(), r.artifacts.size());

    setMode("compilererror");
    auto e = be.execute(req(31));
    QCOMPARE(e.state, JobState::Failed);
    QCOMPARE(e.failureOrigin, FailureOrigin::Compiler);
    QCOMPARE(e.diagnostics.size(), 1);
    QCOMPARE(e.diagnostics[0].line, 1);
    QVERIFY(e.hasArtifact(ArtifactType::Diagnostics));
    QVERIFY(e.artifact(ArtifactType::Diagnostics).content.contains("not declared"));
    QVERIFY(!e.hasArtifact(ArtifactType::Assembly));                  // no output file: no assembly artifact
}

void TestDockerBackend::testArtifactPersistence() {
    DockerExecutionBackend be(cfg());
    auto r = be.execute(req(32));
    auto art = r.artifact(ArtifactType::Assembly);
    QVERIFY(!art.hasPersistentFile());
    QVERIFY(art.persistentPath.isEmpty());
    QTemporaryDir out;
    QString err;
    QVERIFY2(art.saveTo(out.path() + "/saved.s", &err), qPrintable(err));
    QVERIFY(art.hasPersistentFile());
    QFile f(out.path() + "/saved.s");
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(f.readAll()), art.content);
    CompilationArtifact other = art;
    QVERIFY(!other.saveTo(out.path() + "/missing-dir/x.s", &err));       // a failed save claims nothing
    other.persistentPath.clear();
    QVERIFY(!other.saveTo(QString(), &err));
}

void TestDockerBackend::testLlvmIrOutputKind() {
    DockerExecutionBackend be(cfg());
    auto q = req(40);
    q.compilerImage = "silkeh/clang:17";
    q.outputKind = OutputKind::LlvmIr;
    auto r = be.execute(q);
    QCOMPARE(r.state, JobState::Completed);
    QVERIFY(r.hasArtifact(ArtifactType::LLVM_IR));
    QVERIFY(!r.hasArtifact(ArtifactType::Assembly));
    QCOMPARE(r.artifact(ArtifactType::LLVM_IR).name, QString("output.ll"));
    QVERIFY(r.assemblyOutput.contains("define i32 @f"));
    QVERIFY(r.compilerCommand.contains("-emit-llvm"));
    QVERIFY(r.compilerCommand.contains("output.ll"));
    QVERIFY(!r.compilerCommand.contains("-masm"));                    // no assembly syntax option for IR
    QCOMPARE(r.artifact(ArtifactType::LLVM_IR).containerPath, QString("/output/output.ll"));

    // GCC cannot produce IR: the request quietly falls back to assembly instead of failing
    auto g = req(41);
    g.compilerImage = "gcc:13";
    g.outputKind = OutputKind::LlvmIr;
    auto gr = be.execute(g);
    QCOMPARE(gr.state, JobState::Completed);
    QVERIFY(gr.hasArtifact(ArtifactType::Assembly));
    QVERIFY(!gr.compilerCommand.contains("-emit-llvm"));
}

void TestDockerBackend::testBoundedAppendAndUtf8() {
    QByteArray buf; bool trunc = false;
    QCOMPARE(DockerExecutionBackend::appendBounded(buf, QByteArray(10, 'a'), 16, trunc), qint64(0));
    QVERIFY(!trunc);
    QCOMPARE(DockerExecutionBackend::appendBounded(buf, QByteArray(10, 'b'), 16, trunc), qint64(4));
    QVERIFY(trunc);
    QCOMPARE(buf.size(), 16);
    QCOMPARE(DockerExecutionBackend::appendBounded(buf, QByteArray(5, 'c'), 16, trunc), qint64(5));
    QCOMPARE(buf.size(), 16);
    // Huge single chunk is cut, with no 32-bit overflow in the arithmetic.
    QByteArray big; bool t2 = false;
    DockerExecutionBackend::appendBounded(big, QByteArray(200000, 'z'), 1024, t2);
    QCOMPARE(big.size(), 1024);

    // Cut through the middle of "€" (E2 82 AC) and of a 4-byte emoji.
    QByteArray euro = QByteArray("ab") + QString(QChar(0x20AC)).toUtf8();
    QByteArray cut = euro.left(euro.size() - 1);
    DockerExecutionBackend::trimToUtf8Boundary(cut);
    QCOMPARE(cut, QByteArray("ab"));
    QByteArray full = euro;
    DockerExecutionBackend::trimToUtf8Boundary(full);
    QCOMPARE(full, euro);
    QByteArray emoji = QByteArray("x") + QString::fromUcs4(U"\U0001F600").toUtf8();
    QByteArray e2 = emoji.left(emoji.size() - 2);
    DockerExecutionBackend::trimToUtf8Boundary(e2);
    QCOMPARE(e2, QByteArray("x"));
    QByteArray junk("\x80\x80\x80");
    DockerExecutionBackend::trimToUtf8Boundary(junk);   // invalid input must not crash
    QByteArray empty; DockerExecutionBackend::trimToUtf8Boundary(empty);
}

void TestDockerBackend::testConfigSanitizingIsPerField() {
    DockerExecutionConfig c;
    c.memoryLimit = "garbage";
    c.pidsLimit = 256;
    c.cpuLimit = "2";
    auto s = c.sanitized();
    QCOMPARE(s.memoryLimit, DockerExecutionConfig().memoryLimit);  // only the bad field reset
    QCOMPARE(s.pidsLimit, 256);
    QCOMPARE(s.cpuLimit, QString("2"));
    QVERIFY(s.validate());
}

OCTOS_TEST_MAIN(TestDockerBackend)
#include "test_docker_backend.moc"

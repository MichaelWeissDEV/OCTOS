// Verifies against a REAL Docker daemon that the hardening flags produced by
// DockerExecutionBackend::buildDockerRunArgs() actually take effect inside a container.
// Uses only locally present images (alpine:latest); never pulls. SKIPs (visibly) otherwise.
#include "../TestSupport.h"
#include <QTest>
#include <QElapsedTimer>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include "backend/DockerExecutionBackend.h"
#include "backend/DockerExecutionConfig.h"

namespace {
bool dockerUp() {
    QProcess p; p.start("docker", {"info"});
    return p.waitForStarted(2000) && p.waitForFinished(8000) && p.exitCode() == 0;
}
bool haveImage(const QString& img) {
    QProcess p; p.start("docker", {"images", "-q", img});
    return p.waitForStarted(2000) && p.waitForFinished(8000) && !p.readAllStandardOutput().trimmed().isEmpty();
}
struct Run { int code{-1}; QString out, err; };
}

class TestDockerFlags : public QObject {
    Q_OBJECT
    QTemporaryDir m_in, m_out;
    QString m_image{"alpine:latest"};

    Run runScript(const QString& script, DockerExecutionConfig cfg = DockerExecutionConfig(), const QString& name = "octos_flagtest") {
        CompilationRequest req;
        req.compilerImage = m_image;
        cfg.runAsCurrentUser = false;       // alpine runs fine as root; user mapping is Linux-only anyway
        const QStringList args = DockerExecutionBackend::buildDockerRunArgs(
            req, cfg, name, m_in.path(), m_out.path(), {"sh", "-c", script});
        QProcess p;
        p.start("docker", args);
        Run r;
        if (!p.waitForStarted(5000)) return r;
        p.waitForFinished(60000);
        r.code = p.exitCode();
        r.out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        r.err = QString::fromUtf8(p.readAllStandardError()).trimmed();
        return r;
    }

private slots:
    void initTestCase() {
        if (!dockerUp()) QSKIP("Docker daemon not reachable", SkipAll);
        if (!haveImage(m_image)) QSKIP("alpine:latest not present locally (not pulling)", SkipAll);
        QVERIFY(m_in.isValid() && m_out.isValid());
    }
    void testOutputWritableInputReadOnly() {
        QFile f(m_in.path() + "/a.txt"); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("x"); f.close();
        auto r = runScript("echo hi > /output/o.txt && echo out_ok; (echo x > /input/new.txt) 2>/dev/null && echo input_writable || echo input_ro");
        QVERIFY2(r.code == 0, qPrintable(r.err));
        QVERIFY(r.out.contains("out_ok"));
        QVERIFY(r.out.contains("input_ro"));
        QVERIFY(QFile::exists(m_out.path() + "/o.txt"));
    }
    void testNoNetwork() {
        // No ethernet/bridge interface exists and an outbound connection cannot succeed.
        auto r = runScript("ls /sys/class/net | grep -c '^eth'; wget -T 3 -q -O /dev/null http://1.1.1.1 2>&1; echo wget_rc=$?");
        QVERIFY2(r.code == 0, qPrintable(r.err));
        QVERIFY2(r.out.startsWith("0"), qPrintable(r.out));
        QVERIFY2(!r.out.contains("wget_rc=0"), qPrintable(r.out));
    }
    void testPrivilegesAndCapabilities() {
        auto r = runScript("grep -E '^(CapEff|NoNewPrivs)' /proc/self/status");
        QVERIFY2(r.code == 0, qPrintable(r.err));
        QVERIFY2(r.out.contains("CapEff:\t0000000000000000"), qPrintable(r.out));
        QVERIFY2(r.out.contains("NoNewPrivs:\t1"), qPrintable(r.out));
    }
    void testCgroupLimits() {
        DockerExecutionConfig c; c.memoryLimit = "256m"; c.pidsLimit = 64;
        auto r = runScript("cat /sys/fs/cgroup/memory.max /sys/fs/cgroup/memory.swap.max /sys/fs/cgroup/pids.max | tr '\\n' ' '", c);
        QVERIFY2(r.code == 0, qPrintable(r.err));
        const QStringList v = r.out.split(' ', Qt::SkipEmptyParts);
        QCOMPARE(v.value(0), QString("268435456"));  // memory limit
        QCOMPARE(v.value(1), QString("0"));          // swap disabled (memory-swap == memory)
        QCOMPARE(v.value(2), QString("64"));         // pids
    }
    void testTmpfsIsBoundedAndNoExec() {
        DockerExecutionConfig c; c.tmpfsSizeMb = 8;
        auto r = runScript("dd if=/dev/zero of=/tmp/big bs=1M count=32 2>&1; echo rc=$?; "
                           "printf '#!/bin/sh\\necho ran' > /tmp/x.sh; chmod +x /tmp/x.sh; /tmp/x.sh 2>&1 | head -1", c);
        QVERIFY(r.out.contains("No space left") || !r.out.contains("rc=0"));   // bounded
        QVERIFY2(r.out.contains("Permission denied") || r.out.contains("not permitted"), qPrintable(r.out)); // noexec
    }
    void testFileSizeUlimit() {
        DockerExecutionConfig c; c.maxFileSizeBytes = 1024; c.maxTotalArtifactBytes = 4096;
        auto r = runScript("dd if=/dev/zero of=/output/big bs=1024 count=64 2>/dev/null; echo rc=$?", c);
        // The writer is killed by SIGXFSZ (153): this is the code the backend maps to "Limit exceeded"
        QVERIFY2(r.out.contains("rc=153"), qPrintable(r.out));
        QFileInfo fi(m_out.path() + "/big");
        QVERIFY(!fi.exists() || fi.size() <= 4096);
    }
    void testCleanupOfRealContainer() {
        const QString name = "octos_flagtest_cleanup";
        QProcess::execute("docker", {"rm", "-f", name});
        QProcess p; p.start("docker", {"run", "-d", "--name", name, m_image, "sleep", "300"});
        QVERIFY(p.waitForFinished(30000) && p.exitCode() == 0);
        auto exists = [&] { QProcess q; q.start("docker", {"ps", "-aq", "--filter", "name=^" + name + "$"}); q.waitForFinished(8000); return !q.readAllStandardOutput().trimmed().isEmpty(); };
        QVERIFY(exists());
        QVERIFY(DockerExecutionBackend::cleanupContainer(name));
        QVERIFY(!exists());
        QVERIFY(DockerExecutionBackend::cleanupContainer(name));      // repeated: still success (idempotent)
        QVERIFY(!DockerExecutionBackend::cleanupContainer("postgres")); // foreign names are refused
    }
    void testBackendWithRealDaemonAndMissingCompilerImage() {
        // Image absent locally: with --pull=never Docker fails immediately instead of downloading.
        DockerExecutionConfig c; c.defaultTimeoutMs = 20000;
        DockerExecutionBackend be(c);
        CompilationRequest req;
        req.jobId = 777; req.language = Language::Cpp;
        req.compilerImage = "octos-test-nonexistent-image:0";
        req.sourceCode = "int main(){}";
        QElapsedTimer t; t.start();
        auto r = be.execute(req);
        QVERIFY2(t.elapsed() < 15000, "must fail fast, not attempt a download");
        QCOMPARE(r.state, JobState::Failed);
        QCOMPARE(r.failureOrigin, FailureOrigin::Docker);
        QVERIFY2(r.errorOutput.contains("No such image") || r.errorOutput.contains("Unable to find image"), qPrintable(r.errorOutput));
        QVERIFY(be.activeContainerNames().isEmpty());
    }
};

OCTOS_TEST_MAIN(TestDockerFlags)
#include "test_docker_flags.moc"

#include <QTest>
#include "backend/DockerExecutionBackend.h"
#include "backend/DockerExecutionConfig.h"
#include "core/CompilationRequest.h"

class TestDockerSecurity : public QObject {
    Q_OBJECT

private slots:
    void testSecurityArguments();
    void testCustomResourceLimits();
};

void TestDockerSecurity::testSecurityArguments() {
    DockerExecutionConfig config;
    config.networkNone = true;
    config.memoryLimit = "512m";
    config.cpuLimit = "1.5";
    config.pidsLimit = 128;
    config.noNewPrivileges = true;
    config.dropCapabilities = true;

    CompilationRequest req;
    req.jobId = 123;
    req.compilerImage = "gcc:13";

    QString containerName = "octos_test_container_123";
    QString inputHost = "/tmp/host_in";
    QString outputHost = "/tmp/host_out";
    QStringList compilerArgs = {"g++", "-S", "/input/source.cpp", "-o", "/output/output.s"};

    QStringList dockerArgs = DockerExecutionBackend::buildDockerRunArgs(
        req, config, containerName, inputHost, outputHost, compilerArgs);

    // Verify critical security arguments
    QVERIFY(dockerArgs.contains("run"));
    QVERIFY(dockerArgs.contains("--rm"));
    QVERIFY(dockerArgs.contains("--name"));
    QVERIFY(dockerArgs.contains(containerName));

    // Network isolation
    int netIdx = dockerArgs.indexOf("--network");
    QVERIFY(netIdx >= 0 && netIdx + 1 < dockerArgs.size());
    QCOMPARE(dockerArgs[netIdx + 1], QString("none"));

    // Memory limit
    int memIdx = dockerArgs.indexOf("--memory");
    QVERIFY(memIdx >= 0 && memIdx + 1 < dockerArgs.size());
    QCOMPARE(dockerArgs[memIdx + 1], QString("512m"));

    // CPU limit
    int cpuIdx = dockerArgs.indexOf("--cpus");
    QVERIFY(cpuIdx >= 0 && cpuIdx + 1 < dockerArgs.size());
    QCOMPARE(dockerArgs[cpuIdx + 1], QString("1.5"));

    // PID limit
    int pidIdx = dockerArgs.indexOf("--pids-limit");
    QVERIFY(pidIdx >= 0 && pidIdx + 1 < dockerArgs.size());
    QCOMPARE(dockerArgs[pidIdx + 1], QString("128"));

    // Capabilities and privileges
    QVERIFY(dockerArgs.contains("--cap-drop=ALL"));
    QVERIFY(dockerArgs.contains("--security-opt"));
    QVERIFY(dockerArgs.contains("no-new-privileges"));

    // Volume isolation: input must be read-only (:ro), output must be read-write (:rw)
    QVERIFY(dockerArgs.contains(inputHost + ":/input:ro"));
    QVERIFY(dockerArgs.contains(outputHost + ":/output:rw"));

    // Working directory
    int workIdx = dockerArgs.indexOf("-w");
    QVERIFY(workIdx >= 0 && workIdx + 1 < dockerArgs.size());
    QCOMPARE(dockerArgs[workIdx + 1], QString("/output"));
}

void TestDockerSecurity::testCustomResourceLimits() {
    DockerExecutionConfig config;
    config.memoryLimit = "1024m";
    config.cpuLimit = "2.0";
    config.pidsLimit = 64;

    CompilationRequest req;
    req.compilerImage = "clang:16";

    QStringList dockerArgs = DockerExecutionBackend::buildDockerRunArgs(
        req, config, "octos_custom", "/in", "/out", {"clang++"});

    QVERIFY(dockerArgs.contains("1024m"));
    QVERIFY(dockerArgs.contains("2.0"));
    QVERIFY(dockerArgs.contains("64"));
}

QTEST_MAIN(TestDockerSecurity)
#include "test_docker_security.moc"

#include <QTest>
#include <QProcess>
#include "backend/DockerExecutionBackend.h"
#include "backend/DockerExecutionConfig.h"
#include "core/CompilationRequest.h"
#include "core/CompilationResult.h"

class TestDockerIntegration : public QObject {
    Q_OBJECT

private:
    bool isDockerAvailable() {
        QProcess proc;
        proc.start("docker", QStringList() << "info");
        if (!proc.waitForStarted(2000)) {
            return false;
        }
        if (!proc.waitForFinished(3000)) {
            proc.kill();
            return false;
        }
        return proc.exitCode() == 0;
    }

    bool isImageAvailable(const QString& imageName) {
        QProcess proc;
        proc.start("docker", QStringList() << "images" << "-q" << imageName);
        if (!proc.waitForStarted(2000) || !proc.waitForFinished(3000)) {
            return false;
        }
        return !proc.readAllStandardOutput().trimmed().isEmpty();
    }

private slots:
    void testDockerCompilationIfDaemonRunning() {
        if (!isDockerAvailable()) {
            QSKIP("Docker daemon is not running or not reachable. Skipping Docker integration test.", 0);
        }

        QString gccImage = "gcc:latest";
        if (!isImageAvailable(gccImage)) {
            QSKIP("Docker image gcc:latest is not present locally. Skipping to avoid unprompted network pull.", 0);
        }

        DockerExecutionBackend backend;
        CompilationRequest req;
        req.jobId = 999;
        req.compilerImage = gccImage;
        req.language = Language::Cpp;
        req.syntaxFlavor = SyntaxFlavor::Intel;
        req.sourceCode = "int compute() { return 42; }";

        CompilationResult result = backend.execute(req);
        QVERIFY2(result.success(), qPrintable(result.errorOutput));
        QVERIFY(result.assemblyOutput.contains("compute"));
    }

    void testDockerCompilationErrorIfDaemonRunning() {
        if (!isDockerAvailable()) {
            QSKIP("Docker daemon is not running or not reachable. Skipping Docker integration test.", 0);
        }

        QString gccImage = "gcc:latest";
        if (!isImageAvailable(gccImage)) {
            QSKIP("Docker image gcc:latest is not present locally. Skipping to avoid unprompted network pull.", 0);
        }

        DockerExecutionBackend backend;
        CompilationRequest req;
        req.jobId = 1000;
        req.compilerImage = gccImage;
        req.language = Language::Cpp;
        req.sourceCode = "int invalid_syntax = ;";

        CompilationResult result = backend.execute(req);
        QVERIFY(!result.success());
        QVERIFY(!result.errorOutput.isEmpty());
        QVERIFY(!result.diagnostics.isEmpty());
    }
};

QTEST_MAIN(TestDockerIntegration)
#include "test_docker_integration.moc"

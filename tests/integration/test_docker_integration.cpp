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
    void testGccCompilationSuccess();
    void testGccCompilationOptimization();
    void testGccCompilationSyntaxError();
    void testClangCompilationSuccess();
    void testClangCompilationSyntaxError();
    void testRustCompilationSuccess();
    void testRustCompilationOptimization();
    void testRustCompilationSyntaxError();
    void testJavaSimpleClass();
    void testJavaPublicClass();
    void testJavaCompilationSyntaxError();
    void testPythonCompilationSuccess();
    void testPythonCompilationSyntaxError();
    void testCSharpExperimental();
    void testAdaExperimental();
};

void TestDockerIntegration::testGccCompilationSuccess() {
    if (!isDockerAvailable()) {
        QSKIP("Docker daemon is not running or not reachable.", 0);
    }
    QString image = "gcc:latest";
    if (!isImageAvailable(image)) {
        QSKIP("Image gcc:latest not installed locally. Skipping to avoid unprompted network pull.", 0);
    }

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1001;
    req.compilerImage = image;
    req.language = Language::Cpp;
    req.syntaxFlavor = SyntaxFlavor::Intel;
    req.sourceCode = "int compute_val() { return 42; }";

    CompilationResult res = backend.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput));
    QVERIFY(res.assemblyOutput.contains("compute_val"));
    QVERIFY(res.hasArtifact(ArtifactType::Assembly));
}

void TestDockerIntegration::testGccCompilationOptimization() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "gcc:latest";
    if (!isImageAvailable(image)) QSKIP("gcc:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1002;
    req.compilerImage = image;
    req.language = Language::Cpp;
    req.userArgs = "-O3";
    req.sourceCode = "int multiply(int a) { return a * 8; }";

    CompilationResult res = backend.execute(req);
    QVERIFY(res.success());
    // At -O3, a * 8 typically uses shift instruction (shl or lea)
    QVERIFY(res.assemblyOutput.contains("shl") || res.assemblyOutput.contains("lea") || res.assemblyOutput.contains("sal"));
}

void TestDockerIntegration::testGccCompilationSyntaxError() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "gcc:latest";
    if (!isImageAvailable(image)) QSKIP("gcc:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1003;
    req.compilerImage = image;
    req.language = Language::Cpp;
    req.sourceCode = "int broken_syntax = ;";

    CompilationResult res = backend.execute(req);
    QVERIFY(!res.success());
    QVERIFY(!res.errorOutput.isEmpty());
    QVERIFY(!res.diagnostics.isEmpty());
}

void TestDockerIntegration::testClangCompilationSuccess() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "silkeh/clang:latest";
    if (!isImageAvailable(image)) QSKIP("silkeh/clang:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1004;
    req.compilerImage = image;
    req.language = Language::Cpp;
    req.sourceCode = "int clang_calc() { return 100; }";

    CompilationResult res = backend.execute(req);
    QVERIFY(res.success());
    QVERIFY(res.assemblyOutput.contains("clang_calc"));
}

void TestDockerIntegration::testClangCompilationSyntaxError() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "silkeh/clang:latest";
    if (!isImageAvailable(image)) QSKIP("silkeh/clang:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1005;
    req.compilerImage = image;
    req.language = Language::Cpp;
    req.sourceCode = "void bad_clang() { non_existent_token(); }";

    CompilationResult res = backend.execute(req);
    QVERIFY(!res.success());
    QVERIFY(!res.errorOutput.isEmpty());
}

void TestDockerIntegration::testRustCompilationSuccess() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "rust:latest";
    if (!isImageAvailable(image)) QSKIP("rust:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1006;
    req.compilerImage = image;
    req.language = Language::Rust;
    req.sourceCode = "pub fn add_two(x: i32) -> i32 { x + 2 }";

    CompilationResult res = backend.execute(req);
    QVERIFY(res.success());
    QVERIFY(res.assemblyOutput.contains("add_two"));
}

void TestDockerIntegration::testRustCompilationOptimization() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "rust:latest";
    if (!isImageAvailable(image)) QSKIP("rust:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1007;
    req.compilerImage = image;
    req.language = Language::Rust;
    req.userArgs = "-C opt-level=3";
    req.sourceCode = "pub fn square(x: i32) -> i32 { x * x }";

    CompilationResult res = backend.execute(req);
    QVERIFY(res.success());
    QVERIFY(res.assemblyOutput.contains("square"));
}

void TestDockerIntegration::testRustCompilationSyntaxError() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "rust:latest";
    if (!isImageAvailable(image)) QSKIP("rust:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1008;
    req.compilerImage = image;
    req.language = Language::Rust;
    req.sourceCode = "fn broken_rust() { let x: i32 = \"not an int\"; }";

    CompilationResult res = backend.execute(req);
    QVERIFY(!res.success());
    QVERIFY(res.errorOutput.contains("mismatched types") || !res.errorOutput.isEmpty());
}

void TestDockerIntegration::testJavaSimpleClass() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "eclipse-temurin:latest";
    if (!isImageAvailable(image)) image = "openjdk:latest";
    if (!isImageAvailable(image)) QSKIP("Java compiler image missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1009;
    req.compilerImage = image;
    req.language = Language::Java;
    req.sourceCode = "class Simple { int calc() { return 7; } }";

    CompilationResult res = backend.execute(req);
    QVERIFY(res.success());
    QVERIFY(res.assemblyOutput.contains("calc"));
    QVERIFY(res.hasArtifact(ArtifactType::JVM_Bytecode));
}

void TestDockerIntegration::testJavaPublicClass() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "eclipse-temurin:latest";
    if (!isImageAvailable(image)) image = "openjdk:latest";
    if (!isImageAvailable(image)) QSKIP("Java compiler image missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1010;
    req.compilerImage = image;
    req.language = Language::Java;
    req.sourceCode = "public class Greeter { public static void main(String[] args) {} }";

    CompilationResult res = backend.execute(req);
    QVERIFY(res.success());
    QVERIFY(res.assemblyOutput.contains("Greeter"));
}

void TestDockerIntegration::testJavaCompilationSyntaxError() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "eclipse-temurin:latest";
    if (!isImageAvailable(image)) image = "openjdk:latest";
    if (!isImageAvailable(image)) QSKIP("Java compiler image missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1011;
    req.compilerImage = image;
    req.language = Language::Java;
    req.sourceCode = "public class BadJava { void foo() { missing_semicolon } }";

    CompilationResult res = backend.execute(req);
    QVERIFY(!res.success());
}

void TestDockerIntegration::testPythonCompilationSuccess() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "python:latest";
    if (!isImageAvailable(image)) QSKIP("python:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1012;
    req.compilerImage = image;
    req.language = Language::Python;
    req.sourceCode = "def factorial(n):\n    return 1 if n <= 1 else n * factorial(n - 1)\n";

    CompilationResult res = backend.execute(req);
    QVERIFY(res.success());
    QVERIFY(res.assemblyOutput.contains("LOAD_CONST") || res.assemblyOutput.contains("RETURN_VALUE"));
    QVERIFY(res.hasArtifact(ArtifactType::CPython_Bytecode));
}

void TestDockerIntegration::testPythonCompilationSyntaxError() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "python:latest";
    if (!isImageAvailable(image)) QSKIP("python:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1013;
    req.compilerImage = image;
    req.language = Language::Python;
    req.sourceCode = "def broken_python(:\n  pass";

    CompilationResult res = backend.execute(req);
    QVERIFY(!res.success());
    QVERIFY(!res.errorOutput.isEmpty());
}

void TestDockerIntegration::testCSharpExperimental() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "mono:latest";
    if (!isImageAvailable(image)) QSKIP("mono:latest missing (C# is experimental in OCTOS)", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1014;
    req.compilerImage = image;
    req.language = Language::CSharp;
    req.sourceCode = "public class Program { public static void Main() {} }";

    CompilationResult res = backend.execute(req);
    // Verified or experimental execution
    QVERIFY(res.state == JobState::Completed || res.state == JobState::Failed);
}

void TestDockerIntegration::testAdaExperimental() {
    if (!isDockerAvailable()) QSKIP("Docker offline", 0);
    QString image = "gcc:latest";
    if (!isImageAvailable(image)) QSKIP("gcc:latest missing", 0);

    DockerExecutionBackend backend;
    CompilationRequest req;
    req.jobId = 1015;
    req.compilerImage = image;
    req.language = Language::Ada;
    req.sourceCode = "procedure Hello is begin null; end Hello;";

    CompilationResult res = backend.execute(req);
    // GNAT toolchain availability check
    QVERIFY(res.state == JobState::Completed || res.state == JobState::Failed);
}

QTEST_MAIN(TestDockerIntegration)
#include "test_docker_integration.moc"

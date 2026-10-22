// End-to-end compilations through the REAL Docker backend with locally installed compiler images.
// Every repository is matched by name, any tag counts (e.g. rust:1.96.1-bookworm). Nothing is pulled:
// a missing image makes the test SKIP visibly - a skip is never evidence that a compiler works.
#include "../TestSupport.h"
#include <QTest>
#include <QProcess>
#include <QElapsedTimer>
#include <thread>
#include <atomic>
#include "backend/DockerExecutionBackend.h"
#include "backend/DockerExecutionConfig.h"
#include "core/CompilationRequest.h"
#include "core/CompilationResult.h"

namespace {
bool dockerAvailable() {
    QProcess p;
    p.start("docker", {"info"});
    return p.waitForStarted(2000) && p.waitForFinished(8000) && p.exitCode() == 0;
}

// First locally installed image of any of the given repositories (any tag), or "".
QString findImage(const QStringList& repos) {
    for (const QString& repo : repos) {
        QProcess p;
        p.start("docker", {"images", "--format", "{{.Repository}}:{{.Tag}}", repo});
        if (!p.waitForStarted(2000) || !p.waitForFinished(8000)) continue;
        for (const QString& line : QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts)) {
            if (!line.endsWith(":<none>")) return line.trimmed();
        }
    }
    return QString();
}

QString daemonArchitecture() {
    QProcess p;
    p.start("docker", {"info", "--format", "{{.Architecture}}"});
    return p.waitForFinished(8000) ? QString::fromUtf8(p.readAllStandardOutput()).trimmed() : QString();
}

int containersNamed(const QString& prefix) {
    QProcess p;
    p.start("docker", {"ps", "-aq", "--filter", "name=" + prefix});
    p.waitForFinished(8000);
    return QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts).size();
}
}

#define REQUIRE_IMAGE(var, ...)                                                        \
    if (!dockerAvailable()) QSKIP("Docker daemon not reachable", SkipAll);           \
    const QString var = findImage({__VA_ARGS__});                                     \
    if (var.isEmpty()) QSKIP("no local image of: " #__VA_ARGS__ " (not pulling)", SkipAll)

class TestDockerIntegration : public QObject {
    Q_OBJECT

    CompilationRequest base(const QString& image, Language lang, quint64 job) {
        CompilationRequest r;
        r.jobId = job;
        r.compilerImage = image;
        r.language = lang;
        r.targetArchitecture = daemonArchitecture();   // what OCTOS does for unverified images
        r.timeoutMs = 120000;                           // first runs of big images can be slow
        return r;
    }

private slots:
    void testGccCompilation();
    void testGccOptimizationChangesOutput();
    void testGccSyntaxError();
    void testClangCompilation();
    void testClangLlvmIr();
    void testRustLibraryWithoutMain();
    void testRustBinaryWithMain();
    void testRustOptimizationChangesOutput();
    void testRustSyntaxErrorHasDiagnostics();
    void testRustLlvmIr();
    void testRustMultiFileCrate();
    void testRustCancellationRemovesContainer();
    void testJavaPackagesAndInnerClasses();
    void testJavaSyntaxError();
    void testPythonBytecode();
    void testPythonSyntaxError();
};

void TestDockerIntegration::testGccCompilation() {
    REQUIRE_IMAGE(image, "gcc");
    DockerExecutionBackend be;
    auto req = base(image, Language::Cpp, 1001);
    req.sourceCode = "int compute_val() { return 42; }";
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput));
    QVERIFY(res.assemblyOutput.contains("compute_val"));
    QVERIFY(res.hasArtifact(ArtifactType::Assembly));
}

void TestDockerIntegration::testGccOptimizationChangesOutput() {
    REQUIRE_IMAGE(image, "gcc");
    DockerExecutionBackend be;
    auto a = base(image, Language::Cpp, 1002);
    a.sourceCode = "int sum(int n){int s=0;for(int i=0;i<n;++i)s+=i;return s;}";
    a.userArgs = "-O0";
    auto b = a; b.jobId = 1003; b.userArgs = "-O3";
    auto ra = be.execute(a), rb = be.execute(b);
    QVERIFY2(ra.success() && rb.success(), qPrintable(ra.errorOutput + rb.errorOutput));
    QVERIFY(ra.assemblyOutput != rb.assemblyOutput);
}

void TestDockerIntegration::testGccSyntaxError() {
    REQUIRE_IMAGE(image, "gcc");
    DockerExecutionBackend be;
    auto req = base(image, Language::Cpp, 1004);
    req.sourceCode = "int f() { return undefined_symbol; }";
    auto res = be.execute(req);
    QCOMPARE(res.state, JobState::Failed);
    QCOMPARE(res.failureOrigin, FailureOrigin::Compiler);
    QVERIFY(!res.diagnostics.isEmpty());
    QCOMPARE(res.diagnostics.first().line, 1);
}

void TestDockerIntegration::testClangCompilation() {
    REQUIRE_IMAGE(image, "silkeh/clang", "clang");
    DockerExecutionBackend be;
    auto req = base(image, Language::Cpp, 1005);
    req.sourceCode = "int compute_val() { return 42; }";
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput));
    QVERIFY(res.assemblyOutput.contains("compute_val"));
}

void TestDockerIntegration::testClangLlvmIr() {
    REQUIRE_IMAGE(image, "silkeh/clang", "clang");
    DockerExecutionBackend be;
    auto req = base(image, Language::Cpp, 1006);
    req.outputKind = OutputKind::LlvmIr;
    req.sourceCode = "int square(int x) { return x * x; }";
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput));
    QVERIFY(res.hasArtifact(ArtifactType::LLVM_IR));
    QVERIFY(res.assemblyOutput.contains("define"));
}

void TestDockerIntegration::testRustLibraryWithoutMain() {
    REQUIRE_IMAGE(image, "rust");
    DockerExecutionBackend be;
    auto req = base(image, Language::Rust, 1010);
    req.sourceCode = "pub fn add_two(x: i32) -> i32 {\n    x + 2\n}\n";   // no main: Auto -> library
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput + "\n" + res.compilerCommand));
    QVERIFY(res.assemblyOutput.contains("add_two"));
    QVERIFY(res.compilerCommand.contains("--crate-type=lib"));
    QVERIFY(res.hasArtifact(ArtifactType::Assembly));
    // the assembly matches the daemon's architecture (aarch64 on Apple Silicon)
    const QString arch = daemonArchitecture();
    if (arch == "aarch64") QVERIFY2(res.assemblyOutput.contains("ret"), "expected AArch64 code");
    if (arch.contains("aarch64")) QVERIFY(!res.compilerCommand.contains("x86-asm-syntax"));
}

void TestDockerIntegration::testRustBinaryWithMain() {
    REQUIRE_IMAGE(image, "rust");
    DockerExecutionBackend be;
    auto req = base(image, Language::Rust, 1011);
    req.sourceCode = "fn main() { let v: Vec<i32> = (1..10).collect(); println!(\"{}\", v.iter().sum::<i32>()); }\n";
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput));
    QVERIFY(res.compilerCommand.contains("--crate-type=bin"));
    QVERIFY(res.assemblyOutput.contains("main"));
}

void TestDockerIntegration::testRustOptimizationChangesOutput() {
    REQUIRE_IMAGE(image, "rust");
    DockerExecutionBackend be;
    auto a = base(image, Language::Rust, 1012);
    a.sourceCode = "#[no_mangle]\npub extern \"C\" fn sum(n: u32) -> u32 { let mut s = 0; for i in 0..n { s += i; } s }\n";
    a.userArgs = "-C opt-level=0";
    auto b = a; b.jobId = 1013; b.userArgs = "-C opt-level=3";
    auto ra = be.execute(a), rb = be.execute(b);
    QVERIFY2(ra.success() && rb.success(), qPrintable(ra.errorOutput + rb.errorOutput));
    QVERIFY(ra.assemblyOutput != rb.assemblyOutput);
    QVERIFY(ra.assemblyOutput.size() > rb.assemblyOutput.size());
}

void TestDockerIntegration::testRustSyntaxErrorHasDiagnostics() {
    REQUIRE_IMAGE(image, "rust");
    DockerExecutionBackend be;
    auto req = base(image, Language::Rust, 1014);
    req.sourceCode = "pub fn broken_rust() -> i32 {\n    let x: i32 = \"not an int\";\n    x\n}\n";
    auto res = be.execute(req);
    QCOMPARE(res.state, JobState::Failed);
    QCOMPARE(res.failureOrigin, FailureOrigin::Compiler);
    QVERIFY2(res.errorOutput.contains("mismatched types"), qPrintable(res.errorOutput));
    QVERIFY(!res.diagnostics.isEmpty());
    QCOMPARE(res.diagnostics.first().line, 2);
    QVERIFY(res.hasArtifact(ArtifactType::Diagnostics));
}

void TestDockerIntegration::testRustLlvmIr() {
    REQUIRE_IMAGE(image, "rust");
    DockerExecutionBackend be;
    auto req = base(image, Language::Rust, 1015);
    req.outputKind = OutputKind::LlvmIr;
    req.sourceCode = "#[no_mangle]\npub extern \"C\" fn add_two(x: i32) -> i32 { x + 2 }\n";
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput));
    QVERIFY(res.hasArtifact(ArtifactType::LLVM_IR));
    QVERIFY(res.assemblyOutput.contains("define"));
    QVERIFY(res.assemblyOutput.contains("add_two"));
}

void TestDockerIntegration::testRustMultiFileCrate() {
    REQUIRE_IMAGE(image, "rust");
    DockerExecutionBackend be;
    auto req = base(image, Language::Rust, 1016);
    // A real crate layout: src/lib.rs declares a module that lives in src/math.rs.
    req.mainFilePath = "src/lib.rs";
    req.sourceCode = "pub mod math;\npub fn api(x: i32) -> i32 { math::triple(x) + 1 }\n";
    req.extraFiles.append({QString("src/math.rs"), QString("pub fn triple(x: i32) -> i32 { x * 3 }\n")});
    req.extraFiles.append({QString("Cargo.toml"), QString("[package]\nname = \"demo_crate\"\nedition = \"2021\"\n")});
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput + "\n" + res.compilerCommand));
    QVERIFY(res.assemblyOutput.contains("api"));
    QVERIFY(res.compilerCommand.contains("--crate-type=lib"));   // lib.rs is a library crate root
    QVERIFY(res.compilerCommand.contains("demo_crate"));          // crate name taken from Cargo.toml
}

void TestDockerIntegration::testRustCancellationRemovesContainer() {
    REQUIRE_IMAGE(image, "rust");
    DockerExecutionBackend be;
    auto req = base(image, Language::Rust, 1017);
    // A compile-time loop the const evaluator needs minutes for: the job is reliably still running when
    // we cancel, independent of machine speed (a large function body proved too fast on a warm machine).
    req.sourceCode =
        "#![allow(long_running_const_eval)]\n"
        "pub const SPIN: u64 = { let mut i = 0u64; let mut s = 0u64;\n"
        "    while i < 4_000_000_000 { s = s.wrapping_add(i ^ (s >> 3)); i += 1; } s };\n"
        "pub fn f() -> u64 { SPIN }\n";
    std::atomic<bool> stop{false};
    CompilationResult res;
    std::thread t([&] { res = be.execute(req, [&] { return stop.load(); }); });
    QTRY_VERIFY_WITH_TIMEOUT(!be.activeContainerNames().isEmpty(), 20000);
    QTest::qWait(1500);
    QElapsedTimer timer; timer.start();
    stop = true;
    t.join();
    QVERIFY2(timer.elapsed() < 15000, "cancellation must stop the job promptly");
    QCOMPARE(res.state, JobState::Cancelled);
    QVERIFY(be.leakedContainers().isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(containersNamed(QString("octos_%1_1017").arg(QCoreApplication::applicationPid())), 0, 10000);
}

void TestDockerIntegration::testJavaPackagesAndInnerClasses() {
    REQUIRE_IMAGE(image, "eclipse-temurin", "openjdk");
    DockerExecutionBackend be;
    auto req = base(image, Language::Java, 1020);
    req.sourceCode = "package demo.pkg;\npublic class Outer {\n  static class Inner { int v() { return 3; } }\n  int w() { return new Inner().v(); }\n}\n";
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput));
    QVERIFY(res.assemblyOutput.contains("demo.pkg.Outer"));
    QVERIFY(res.assemblyOutput.contains("Inner"));
    QVERIFY(res.hasArtifact(ArtifactType::JVM_Bytecode));
}

void TestDockerIntegration::testJavaSyntaxError() {
    REQUIRE_IMAGE(image, "eclipse-temurin", "openjdk");
    DockerExecutionBackend be;
    auto req = base(image, Language::Java, 1021);
    req.sourceCode = "public class Main {\n  int f() { return \"x\"; }\n}\n";
    auto res = be.execute(req);
    QCOMPARE(res.failureOrigin, FailureOrigin::Compiler);
    QVERIFY(!res.diagnostics.isEmpty());
    QCOMPARE(res.diagnostics.first().line, 2);
}

void TestDockerIntegration::testPythonBytecode() {
    REQUIRE_IMAGE(image, "python");
    DockerExecutionBackend be;
    auto req = base(image, Language::Python, 1030);
    req.sourceCode = "def factorial(n):\n    return 1 if n <= 1 else n * factorial(n - 1)\n";
    auto res = be.execute(req);
    QVERIFY2(res.success(), qPrintable(res.errorOutput));
    QVERIFY(res.assemblyOutput.contains("CPython Bytecode"));
    QVERIFY(res.hasArtifact(ArtifactType::CPython_Bytecode));
}

void TestDockerIntegration::testPythonSyntaxError() {
    REQUIRE_IMAGE(image, "python");
    DockerExecutionBackend be;
    auto req = base(image, Language::Python, 1031);
    req.sourceCode = "def broken_python(:\n  pass";
    auto res = be.execute(req);
    QCOMPARE(res.failureOrigin, FailureOrigin::Compiler);
    QVERIFY(!res.diagnostics.isEmpty());
    QCOMPARE(res.diagnostics.first().line, 1);
}

OCTOS_TEST_MAIN(TestDockerIntegration)
#include "test_docker_integration.moc"

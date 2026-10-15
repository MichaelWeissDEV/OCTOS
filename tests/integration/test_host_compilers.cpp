// Runs the argument vectors produced by the language strategies against real compilers that
// are installed on the HOST (rustc, clang, javac/javap, python3). This validates the strategies
// against real compiler output without Docker. It is not a substitute for the Docker
// integration tests (test_docker_integration), which cover the container path.
// Tests SKIP (never silently pass) when a tool is missing.
#include "../TestSupport.h"
#include <QTest>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include "strategies/LanguageStrategyRegistry.h"
#include "strategies/RustStrategy.h"
#include "core/CompilationRequest.h"
#include "core/CompilationResult.h"

namespace {
struct HostRun {
    bool started{false};
    int exitCode{-1};
    QString out, err, asmFile;
    QStringList args;
};

bool toolWorks(const QString& tool, const QStringList& args = {"--version"}) {
    QProcess p;
    p.start(tool, args);
    return p.waitForStarted(2000) && p.waitForFinished(8000) && p.exitCode() == 0;
}

HostRun runStrategy(const CompilationRequest& req, const QStringList& afterBinary = {}) {
    HostRun r;
    const auto* strat = LanguageStrategyRegistry::instance().getStrategy(req.language, req.compilerImage);
    QTemporaryDir in, out;
    const QString src = in.path() + "/" + strat->sourceFileName(req.sourceCode);
    QFile f(src);
    if (!f.open(QIODevice::WriteOnly)) return r;
    f.write(req.sourceCode.toUtf8());
    f.close();

    QStringList args = strat->buildCompilerArgs(req, "/input/" + strat->sourceFileName(req.sourceCode), "/output");
    for (QString& a : args) {
        a.replace("/input/", in.path() + "/");
        if (a == "/output") a = out.path();
        else a.replace("/output/", out.path() + "/");
    }
    r.args = args;
    const QString bin = args.takeFirst();
    for (int i = afterBinary.size() - 1; i >= 0; --i) args.prepend(afterBinary[i]);
    QProcess p;
    p.start(bin, args);
    r.started = p.waitForStarted(5000);
    if (!r.started) return r;
    p.waitForFinished(60000);
    r.exitCode = p.exitCode();
    r.out = QString::fromUtf8(p.readAllStandardOutput());
    r.err = QString::fromUtf8(p.readAllStandardError());
    QFile a(out.path() + "/" + strat->outputFileFor(req));
    if (a.open(QIODevice::ReadOnly)) r.asmFile = QString::fromUtf8(a.readAll());
    return r;
}

CompilationRequest rustReq(const QString& code, const QString& flags = {}, RustCrateType t = RustCrateType::Auto) {
    CompilationRequest r;
    r.language = Language::Rust; r.compilerImage = "rust:latest";
    r.sourceCode = code; r.userArgs = flags; r.rustCrateType = t;
    return r;
}
}

class TestHostCompilers : public QObject {
    Q_OBJECT
private slots:
    void testRustLibraryWithoutMain();
    void testRustBinaryWithMain();
    void testRustBinaryWithoutMainFailsWithDiagnostic();
    void testRustFlagsAreDeduplicated();
    void testRustOptimizationChangesOutput();
    void testRustCompileError();
    void testClangX86ElfOutput();
    void testClangLlvmIr();
    void testRustLlvmIr();
    void testClangCompileError();
    void testPythonDisassembly();
    void testPythonSyntaxError();
    void testJavaInnerClassesAndPackage();
};

void TestHostCompilers::testRustLibraryWithoutMain() {
    if (!toolWorks("rustc")) QSKIP("rustc not installed on host", SkipAll);
    auto r = runStrategy(rustReq("pub fn add_two(x: i32) -> i32 {\n    x + 2\n}\n"));
    QVERIFY2(r.exitCode == 0, qPrintable(r.err + "\nargs: " + r.args.join(' ')));
    QVERIFY(r.asmFile.contains("add_two"));
    QVERIFY(r.args.contains("--crate-type=lib"));
}

void TestHostCompilers::testRustBinaryWithMain() {
    if (!toolWorks("rustc")) QSKIP("rustc not installed on host", SkipAll);
    auto r = runStrategy(rustReq("fn main() { let x = 1 + 2; std::process::exit(x - 3); }\n"));
    QVERIFY2(r.exitCode == 0, qPrintable(r.err));
    QVERIFY(r.args.contains("--crate-type=bin"));
    QVERIFY(r.asmFile.contains("main"));
}

void TestHostCompilers::testRustBinaryWithoutMainFailsWithDiagnostic() {
    if (!toolWorks("rustc")) QSKIP("rustc not installed on host", SkipAll);
    // Explicit Binary on a main-less snippet: the real compiler must reject it, with a parsed diagnostic.
    auto req = rustReq("pub fn add_two(x: i32) -> i32 { x + 2 }\n", {}, RustCrateType::Binary);
    auto r = runStrategy(req);
    QVERIFY(r.exitCode != 0);
    const auto* strat = LanguageStrategyRegistry::instance().getStrategy(Language::Rust, "rust:latest");
    auto res = strat->processOutput(req, r.exitCode, r.out, r.err, r.asmFile);
    QCOMPARE(res.state, JobState::Failed);
    QVERIFY2(!res.diagnostics.isEmpty(), qPrintable(r.err));
}

void TestHostCompilers::testRustFlagsAreDeduplicated() {
    const auto* strat = LanguageStrategyRegistry::instance().getStrategy(Language::Rust, "rust:latest");
    auto count = [](const QStringList& a, const QString& needle) {
        int n = 0; for (const QString& s : a) if (s.startsWith(needle)) ++n; return n;
    };
    // Contradictory / repeated user input must collapse into one value per setting.
    auto req = rustReq("pub fn f() {}", "-O -C opt-level=3 --emit=obj --crate-type=bin --crate-type lib -g --edition 2018 -C debuginfo=1");
    QStringList a = strat->buildCompilerArgs(req, "/input/source.rs", "/output");
    QCOMPARE(count(a, "--emit"), 1);
    QVERIFY(a.contains("--emit=asm"));
    QCOMPARE(count(a, "--crate-type"), 1);
    QVERIFY(a.contains("--crate-type=lib"));          // the last explicit user value wins
    QCOMPARE(count(a, "opt-level="), 1);
    QVERIFY(a.contains("opt-level=3"));
    QCOMPARE(count(a, "--edition"), 1);
    QVERIFY(a.contains("--edition=2018"));
    QVERIFY(!a.contains("-g"));                        // user chose an explicit debuginfo level
    QVERIFY(a.contains("debuginfo=1"));
    // Auto-detection
    QVERIFY(RustStrategy::resolveCrateType(RustCrateType::Auto, "fn main() {}") == RustCrateType::Binary);
    QVERIFY(RustStrategy::resolveCrateType(RustCrateType::Auto, "pub fn domain() {}") == RustCrateType::Library);
    QVERIFY(RustStrategy::resolveCrateType(RustCrateType::Library, "fn main() {}") == RustCrateType::Library);
}

void TestHostCompilers::testRustOptimizationChangesOutput() {
    if (!toolWorks("rustc")) QSKIP("rustc not installed on host", SkipAll);
    const QString code = "#[no_mangle]\npub extern \"C\" fn sum(n: u32) -> u32 { let mut s = 0; for i in 0..n { s += i; } s }\n";
    auto o0 = runStrategy(rustReq(code, "-C opt-level=0"));
    auto o3 = runStrategy(rustReq(code, "-C opt-level=3"));
    QVERIFY2(o0.exitCode == 0 && o3.exitCode == 0, qPrintable(o0.err + o3.err));
    QVERIFY(o0.asmFile != o3.asmFile);
    QVERIFY(o0.asmFile.size() > o3.asmFile.size());
}

void TestHostCompilers::testRustCompileError() {
    if (!toolWorks("rustc")) QSKIP("rustc not installed on host", SkipAll);
    auto req = rustReq("pub fn f() -> i32 { let x: i32 = \"no\"; x }\n");
    auto r = runStrategy(req);
    QVERIFY(r.exitCode != 0);
    const auto* strat = LanguageStrategyRegistry::instance().getStrategy(Language::Rust, "rust:latest");
    auto res = strat->processOutput(req, r.exitCode, r.out, r.err, r.asmFile);
    QVERIFY(!res.diagnostics.isEmpty());
    QCOMPARE(res.diagnostics[0].severity, DiagnosticSeverity::Error);
    QVERIFY(res.diagnostics[0].line >= 1);
}

void TestHostCompilers::testClangX86ElfOutput() {
    if (!toolWorks("clang")) QSKIP("clang not installed on host", SkipAll);
    CompilationRequest req;
    req.language = Language::Cpp; req.compilerImage = "clang:latest";
    req.syntaxFlavor = SyntaxFlavor::ATT;
    req.userArgs = "-O1";
    req.sourceCode = "int square(int x) { return x * x; }\n";
    auto r = runStrategy(req, {"--target=x86_64-unknown-linux-gnu"});
    QVERIFY2(r.exitCode == 0, qPrintable(r.err));
    QVERIFY(r.asmFile.contains(".type\t_Z6squarei,@function"));
    QVERIFY(r.asmFile.contains(".loc"));
    QVERIFY(r.asmFile.contains("imull"));
    QVERIFY(r.asmFile.contains(".size"));
}

void TestHostCompilers::testClangLlvmIr() {
    if (!toolWorks("clang")) QSKIP("clang not installed on host", SkipAll);
    CompilationRequest req;
    req.language = Language::Cpp; req.compilerImage = "clang:latest";
    req.outputKind = OutputKind::LlvmIr;
    req.userArgs = "-O1";
    req.sourceCode = "int square(int x) { return x * x; }\n";
    auto r = runStrategy(req, {"--target=x86_64-unknown-linux-gnu"});
    QVERIFY2(r.exitCode == 0, qPrintable(r.err + "\n" + r.args.join(' ')));
    QVERIFY2(r.asmFile.contains("define"), "output.ll must contain LLVM IR");
    QVERIFY(r.asmFile.contains("_Z6squarei"));
    QVERIFY(!r.asmFile.contains(".type"));                         // not assembly
    QVERIFY(!r.args.join(' ').contains("-masm"));
}

void TestHostCompilers::testRustLlvmIr() {
    if (!toolWorks("rustc")) QSKIP("rustc not installed on host", SkipAll);
    auto req = rustReq("#[no_mangle]\npub extern \"C\" fn add_two(x: i32) -> i32 { x + 2 }\n");
    req.outputKind = OutputKind::LlvmIr;
    auto r = runStrategy(req);
    QVERIFY2(r.exitCode == 0, qPrintable(r.err + "\n" + r.args.join(' ')));
    QVERIFY(r.asmFile.contains("define"));
    QVERIFY(r.asmFile.contains("add_two"));
    QVERIFY(r.args.contains("--emit=llvm-ir"));
    QVERIFY(!r.args.join(' ').contains("x86-asm-syntax"));
}

void TestHostCompilers::testClangCompileError() {
    if (!toolWorks("clang")) QSKIP("clang not installed on host", SkipAll);
    CompilationRequest req;
    req.language = Language::Cpp; req.compilerImage = "clang:latest";
    req.sourceCode = "int f() { return undefined_symbol; }\n";
    auto r = runStrategy(req, {"--target=x86_64-unknown-linux-gnu"});
    QVERIFY(r.exitCode != 0);
    const auto* strat = LanguageStrategyRegistry::instance().getStrategy(Language::Cpp, "clang:latest");
    auto res = strat->processOutput(req, r.exitCode, r.out, r.err, r.asmFile);
    QVERIFY(!res.diagnostics.isEmpty());
    QCOMPARE(res.diagnostics[0].line, 1);
}

void TestHostCompilers::testPythonDisassembly() {
    if (!toolWorks("python3")) QSKIP("python3 not installed on host", SkipAll);
    CompilationRequest req;
    req.language = Language::Python; req.compilerImage = "python:3";
    req.sourceCode = "def f(a, b):\n    return a * b + 1\n";
    auto r = runStrategy(req);
    QVERIFY2(r.exitCode == 0, qPrintable(r.err));
    QVERIFY(r.out.contains("CPython Bytecode"));
    QVERIFY(r.out.contains("not native machine assembly", Qt::CaseInsensitive) || r.out.contains("Not native"));
    QVERIFY(r.out.contains("BINARY_OP") || r.out.contains("BINARY_MULTIPLY"));
}

void TestHostCompilers::testPythonSyntaxError() {
    if (!toolWorks("python3")) QSKIP("python3 not installed on host", SkipAll);
    CompilationRequest req;
    req.language = Language::Python; req.compilerImage = "python:3";
    req.sourceCode = "def f(:\n    pass\n";
    auto r = runStrategy(req);
    QVERIFY(r.exitCode != 0);
    const auto* strat = LanguageStrategyRegistry::instance().getStrategy(Language::Python, "python:3");
    auto res = strat->processOutput(req, r.exitCode, r.out, r.err, r.asmFile);
    QVERIFY2(!res.diagnostics.isEmpty(), qPrintable(r.err));
    QCOMPARE(res.diagnostics[0].line, 1);
}

void TestHostCompilers::testJavaInnerClassesAndPackage() {
    if (!toolWorks("javac", {"-version"}) || !toolWorks("javap", {"-version"})) QSKIP("JDK (javac/javap) not installed on host", SkipAll);
    CompilationRequest req;
    req.language = Language::Java; req.compilerImage = "openjdk:21";
    req.sourceCode = "package demo.pkg;\npublic class Outer {\n  static class Inner { int v() { return 3; } }\n  int w() { return new Inner().v(); }\n}\nclass Second { }\n";
    auto r = runStrategy(req);
    // The strategy emits only the main class' bytecode; inner/secondary classes are reported separately
    QVERIFY2(r.exitCode == 0, qPrintable(r.err));
    QVERIFY(r.out.contains("class demo.pkg.Outer"));
    QVERIFY(r.out.contains("Code:"));
    QVERIFY2(r.out.contains("Inner") , "inner classes must be reachable in the output");
}

OCTOS_TEST_MAIN(TestHostCompilers)
#include "test_host_compilers.moc"

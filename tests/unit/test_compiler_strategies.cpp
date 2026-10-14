#include <QTest>
#include <QStringList>
#include "strategies/LanguageStrategyRegistry.h"
#include "strategies/StrategyUtils.h"
#include "strategies/JavaStrategy.h"
#include "core/CompilationRequest.h"

class TestCompilerStrategies : public QObject {
    Q_OBJECT

private slots:
    void testGccStrategyArgs();
    void testGccSyntaxFlavors();
    void testClangStrategyArgs();
    void testRustStrategyNoGccFlags();
    void testRustSyntaxFlavor();
    void testJavaClassDetection();
    void testJavaStrategyArgs();
    void testPythonDestinationSafe();
    void testFlagSanitization();
    void testDiagnosticParsing();
};

void TestCompilerStrategies::testGccStrategyArgs() {
    auto& registry = LanguageStrategyRegistry::instance();
    const auto* strat = registry.getStrategy(Language::Cpp, "gcc:latest");
    QVERIFY(strat != nullptr);
    QCOMPARE(strat->defaultCompilerBinary("gcc:latest"), QString("g++"));

    CompilationRequest req;
    req.compilerImage = "gcc:latest";
    req.language = Language::Cpp;
    req.syntaxFlavor = SyntaxFlavor::Intel;
    req.userArgs = "-O2";

    QStringList args = strat->buildCompilerArgs(req, "/input/source.cpp", "/output");
    QVERIFY(args.contains("g++"));
    QVERIFY(args.contains("-S"));
    QVERIFY(args.contains("-fno-asynchronous-unwind-tables"));
    QVERIFY(args.contains("-masm=intel"));
    QVERIFY(args.contains("-O2"));
    QVERIFY(args.contains("-g"));
    QVERIFY(args.contains("/input/source.cpp"));
    QVERIFY(args.contains("-o"));
    QVERIFY(args.contains("/output/output.s"));
}

void TestCompilerStrategies::testGccSyntaxFlavors() {
    auto& registry = LanguageStrategyRegistry::instance();
    const auto* strat = registry.getStrategy(Language::C, "gcc:13");
    QVERIFY(strat != nullptr);
    QCOMPARE(strat->defaultCompilerBinary("gcc:13"), QString("gcc"));

    CompilationRequest req;
    req.compilerImage = "gcc:13";
    req.language = Language::C;
    req.syntaxFlavor = SyntaxFlavor::ATT;

    QStringList args = strat->buildCompilerArgs(req, "/input/source.c", "/output");
    QVERIFY(args.contains("gcc"));
    QVERIFY(args.contains("-masm=att"));
    QVERIFY(!args.contains("-masm=intel"));
}

void TestCompilerStrategies::testClangStrategyArgs() {
    auto& registry = LanguageStrategyRegistry::instance();
    const auto* strat = registry.getStrategy(Language::Cpp, "clang:17");
    QVERIFY(strat != nullptr);
    QCOMPARE(strat->defaultCompilerBinary("clang:17"), QString("clang++"));

    CompilationRequest req;
    req.compilerImage = "clang:17";
    req.language = Language::Cpp;
    req.syntaxFlavor = SyntaxFlavor::Intel;
    req.userArgs = "-O3";

    QStringList args = strat->buildCompilerArgs(req, "/input/source.cpp", "/output");
    QVERIFY(args.contains("clang++"));
    QVERIFY(args.contains("-S"));
    QVERIFY(args.contains("-masm=intel"));
    QVERIFY(args.contains("-O3"));
}

void TestCompilerStrategies::testRustStrategyNoGccFlags() {
    auto& registry = LanguageStrategyRegistry::instance();
    const auto* strat = registry.getStrategy(Language::Rust, "rust:latest");
    QVERIFY(strat != nullptr);
    QCOMPARE(strat->defaultCompilerBinary("rust:latest"), QString("rustc"));

    CompilationRequest req;
    req.compilerImage = "rust:latest";
    req.language = Language::Rust;
    req.syntaxFlavor = SyntaxFlavor::Intel;
    // User accidentally supplies GCC-specific flags
    req.userArgs = "-O -masm=intel -fno-asynchronous-unwind-tables";

    QStringList args = strat->buildCompilerArgs(req, "/input/source.rs", "/output");
    QVERIFY(args.contains("rustc"));
    QVERIFY(args.contains("--emit=asm"));
    // Must NOT contain raw GCC flags
    QVERIFY(!args.contains("-masm=intel"));
    QVERIFY(!args.contains("-fno-asynchronous-unwind-tables"));
    QVERIFY(!args.contains("-S"));
}

void TestCompilerStrategies::testRustSyntaxFlavor() {
    auto& registry = LanguageStrategyRegistry::instance();
    const auto* strat = registry.getStrategy(Language::Rust, "rust:1.75");

    CompilationRequest reqIntel;
    reqIntel.compilerImage = "rust:1.75";
    reqIntel.language = Language::Rust;
    reqIntel.syntaxFlavor = SyntaxFlavor::Intel;

    QStringList argsIntel = strat->buildCompilerArgs(reqIntel, "/input/source.rs", "/output");
    QVERIFY(argsIntel.contains("llvm-args=--x86-asm-syntax=intel"));

    CompilationRequest reqAtt;
    reqAtt.compilerImage = "rust:1.75";
    reqAtt.language = Language::Rust;
    reqAtt.syntaxFlavor = SyntaxFlavor::ATT;

    QStringList argsAtt = strat->buildCompilerArgs(reqAtt, "/input/source.rs", "/output");
    QVERIFY(argsAtt.contains("llvm-args=--x86-asm-syntax=att"));
}

void TestCompilerStrategies::testJavaClassDetection() {
    QString src1 = "public class Calculator {\n  public int add(int a, int b) { return a + b; }\n}\n";
    QCOMPARE(JavaStrategy::detectClassName(src1), QString("Calculator"));

    QString src2 = "class Helper {\n}\npublic final class App {\n}\n";
    QCOMPARE(JavaStrategy::detectClassName(src2), QString("App"));

    QString src3 = "record Point(int x, int y) {}\n";
    QCOMPARE(JavaStrategy::detectClassName(src3), QString("Point"));

    QString srcEmpty = "// just comments";
    QCOMPARE(JavaStrategy::detectClassName(srcEmpty), QString("Main"));
}

void TestCompilerStrategies::testJavaStrategyArgs() {
    auto& registry = LanguageStrategyRegistry::instance();
    const auto* strat = registry.getStrategy(Language::Java, "openjdk:17");
    QVERIFY(strat != nullptr);

    CompilationRequest req;
    req.compilerImage = "openjdk:17";
    req.language = Language::Java;
    req.sourceCode = "public class MyClass { public void test() {} }";

    QStringList args = strat->buildCompilerArgs(req, "/input/MyClass.java", "/output");
    QVERIFY(args.contains("sh"));
    QVERIFY(args.contains("-c"));
    QVERIFY(args.contains("/output"));
    QVERIFY(args.contains("/input/MyClass.java"));
    QVERIFY(args.contains("MyClass"));
}

void TestCompilerStrategies::testPythonDestinationSafe() {
    auto& registry = LanguageStrategyRegistry::instance();
    const auto* strat = registry.getStrategy(Language::Python, "python:3.11");
    QVERIFY(strat != nullptr);

    CompilationRequest req;
    req.compilerImage = "python:3.11";
    req.language = Language::Python;
    req.sourceCode = "def foo(): pass";

    QStringList args = strat->buildCompilerArgs(req, "/input/source.py", "/output");
    QVERIFY(args.contains("python3"));
    QVERIFY(args.contains("-c"));
    QVERIFY(args.contains("/input/source.py"));
    QVERIFY(args.contains("/output/source.pyc"));
    // Destination is strictly in /output, preserving read-only /input
}

void TestCompilerStrategies::testFlagSanitization() {
    QString raw = "-O3 ; rm -rf / | cat $PATH `id` -o /tmp/hacked --output=evil.s";
    QStringList split = StrategyUtils::splitCompilerArguments(raw);
    QStringList sanitized = StrategyUtils::sanitizeUserFlags(split);

    QVERIFY(sanitized.contains("-O3"));
    QVERIFY(!sanitized.contains(";"));
    QVERIFY(!sanitized.contains("|"));
    QVERIFY(!sanitized.contains("$PATH"));
    QVERIFY(!sanitized.contains("`id`"));
    QVERIFY(!sanitized.contains("-o"));
    QVERIFY(!sanitized.contains("/tmp/hacked"));
    QVERIFY(!sanitized.contains("--output=evil.s"));
}

void TestCompilerStrategies::testDiagnosticParsing() {
    QString gccErr = "source.cpp:12:5: error: expected ';' before 'return'\n"
                     "source.cpp:15:10: warning: unused variable 'x'\n"
                     "source.cpp:15:10: note: declared here\n";

    QVector<Diagnostic> diags = StrategyUtils::parseClangGccDiagnostics(gccErr);
    QCOMPARE(diags.size(), 3);
    QCOMPARE(diags[0].line, 12);
    QCOMPARE(diags[0].column, 5);
    QCOMPARE(diags[0].severity, DiagnosticSeverity::Error);
    QCOMPARE(diags[1].severity, DiagnosticSeverity::Warning);
    QCOMPARE(diags[2].severity, DiagnosticSeverity::Note);
}

QTEST_MAIN(TestCompilerStrategies)
#include "test_compiler_strategies.moc"

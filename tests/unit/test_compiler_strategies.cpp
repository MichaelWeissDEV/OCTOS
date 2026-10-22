#include "../TestSupport.h"
#include <QTest>
#include <QStringList>
#include "strategies/LanguageStrategyRegistry.h"
#include "strategies/StrategyUtils.h"
#include "strategies/JavaStrategy.h"
#include "strategies/RustStrategy.h"
#include "core/CompilationRequest.h"
#include "core/CompilerRegistry.h"

class TestCompilerStrategies : public QObject {
    Q_OBJECT

private slots:
    void testGccStrategyArgs();
    void testGccSyntaxFlavors();
    void testClangStrategyArgs();
    void testRustStrategyNoGccFlags();
    void testRustSyntaxFlavor();
    void testX86OnlyFlagsAreOmittedForArm();
    void testJavaClassDetection();
    void testJavaStrategyArgs();
    void testPythonDestinationSafe();
    void testFlagSanitization();
    void testDiagnosticParsing();
    void testCompilerRegistryDefaults();
    void testCompilerRegistryInference();
    void testCompilerCompatibilityFilter();
    void testRustCrateTypeFromPath();
    void testRustCrateInfoFromCargoToml();
    void testProjectIncludeFlags();
    void testAcceptsSourceName();
    void testJavaSourceRoot();
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

void TestCompilerStrategies::testX86OnlyFlagsAreOmittedForArm() {
    auto& registry = LanguageStrategyRegistry::instance();
    auto hasMasm = [](const QStringList& a) { for (const QString& s : a) if (s.startsWith("-masm=")) return true; return false; };
    for (Language lang : {Language::Cpp, Language::C}) {
        for (const char* image : {"gcc:13", "silkeh/clang:17"}) {
            const auto* strat = registry.getStrategy(lang, image);
            QVERIFY(strat);
            CompilationRequest req;
            req.language = lang; req.compilerImage = image; req.syntaxFlavor = SyntaxFlavor::Intel;
            QVERIFY2(hasMasm(strat->buildCompilerArgs(req, "/input/source", "/output")), image);       // unknown arch = x86 default
            req.targetArchitecture = "x86_64";
            QVERIFY(hasMasm(strat->buildCompilerArgs(req, "/input/source", "/output")));
            req.targetArchitecture = "aarch64";                                                       // GCC aborts on -masm= there
            QVERIFY2(!hasMasm(strat->buildCompilerArgs(req, "/input/source", "/output")), image);
            req.targetArchitecture = "arm64";
            QVERIFY(!hasMasm(strat->buildCompilerArgs(req, "/input/source", "/output")));
        }
    }
    const auto* rust = registry.getStrategy(Language::Rust, "rust:1.75");
    CompilationRequest rr; rr.language = Language::Rust; rr.compilerImage = "rust:1.75";
    rr.targetArchitecture = "aarch64";
    QVERIFY(!rust->buildCompilerArgs(rr, "/input/source.rs", "/output").join(' ').contains("x86-asm-syntax"));
    rr.targetArchitecture = "x86_64";
    QVERIFY(rust->buildCompilerArgs(rr, "/input/source.rs", "/output").join(' ').contains("x86-asm-syntax"));
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

void TestCompilerStrategies::testCompilerRegistryDefaults() {
    auto& reg = CompilerRegistry::instance();
    QVERIFY(!reg.allCompilers().isEmpty());

    QVERIFY(reg.hasCompilerForImage("gcc:latest"));
    CompilerDefinition gccDef = reg.compilerForImage("gcc:latest");
    QCOMPARE(gccDef.family, CompilerFamily::GCC);
    QVERIFY(gccDef.supportsSyntax(SyntaxFlavor::Intel));
    QVERIFY(gccDef.supportsSyntax(SyntaxFlavor::ATT));
    QVERIFY(gccDef.supportsArtifact(ArtifactType::Assembly));
}

void TestCompilerStrategies::testCompilerRegistryInference() {
    auto& reg = CompilerRegistry::instance();

    CompilerDefinition rustcDef = reg.compilerForImage("rust:1.75");
    QCOMPARE(rustcDef.family, CompilerFamily::Rustc);
    QCOMPARE(rustcDef.language, Language::Rust);

    CompilerDefinition clangDef = reg.compilerForImage("silkeh/clang:16");
    QCOMPARE(clangDef.family, CompilerFamily::Clang);

    CompilerDefinition pyDef = reg.compilerForImage("python:3.11");
    QCOMPARE(pyDef.family, CompilerFamily::Python);
    QCOMPARE(pyDef.language, Language::Python);
}

void TestCompilerStrategies::testCompilerCompatibilityFilter() {
    auto& reg = CompilerRegistry::instance();

    // Verified compilers
    QVERIFY(reg.isImageCompatibleWithLanguage("gcc:13", Language::Cpp));
    QVERIFY(reg.isImageCompatibleWithLanguage("gcc:13", Language::C));
    QVERIFY(reg.isImageCompatibleWithLanguage("silkeh/clang:17", Language::Cpp));
    QVERIFY(reg.isImageCompatibleWithLanguage("rust:latest", Language::Rust));
    QVERIFY(reg.isImageCompatibleWithLanguage("eclipse-temurin:21", Language::Java));
    QVERIFY(reg.isImageCompatibleWithLanguage("python:3.12", Language::Python));

    // INCOMPATIBLE combinations must be strictly REJECTED
    QVERIFY(!reg.isImageCompatibleWithLanguage("rust:latest", Language::Cpp));
    QVERIFY(!reg.isImageCompatibleWithLanguage("gcc:13", Language::Rust));
    QVERIFY(!reg.isImageCompatibleWithLanguage("python:3.11", Language::Cpp));
    QVERIFY(!reg.isImageCompatibleWithLanguage("eclipse-temurin:17", Language::Rust));

    // Arbitrary/unrelated images must NOT be accepted as compilers
    QVERIFY(!reg.isImageCompatibleWithLanguage("ubuntu:latest", Language::Cpp));
    QVERIFY(!reg.isImageCompatibleWithLanguage("nginx:alpine", Language::Rust));
    QVERIFY(!reg.isImageCompatibleWithLanguage("trusty-rust-backend", Language::Rust));
}

void TestCompilerStrategies::testRustCrateTypeFromPath() {
    // The crate root's path decides as Cargo does, even against the content heuristic.
    QCOMPARE(RustStrategy::resolveCrateType(RustCrateType::Auto, "fn main() {}", "src/lib.rs"), RustCrateType::Library);
    QCOMPARE(RustStrategy::resolveCrateType(RustCrateType::Auto, "pub fn f() {}", "src/main.rs"), RustCrateType::Binary);
    QCOMPARE(RustStrategy::resolveCrateType(RustCrateType::Auto, "pub fn f() {}", "src/bin/tool.rs"), RustCrateType::Binary);
    QCOMPARE(RustStrategy::resolveCrateType(RustCrateType::Auto, "pub fn f() {}", "examples/demo.rs"), RustCrateType::Binary);
    QCOMPARE(RustStrategy::resolveCrateType(RustCrateType::Auto, "pub fn f() {}", "src/other.rs"), RustCrateType::Library);
    QCOMPARE(RustStrategy::resolveCrateType(RustCrateType::Auto, "fn main() {}", "src/other.rs"), RustCrateType::Binary);
    // An explicit choice always wins.
    QCOMPARE(RustStrategy::resolveCrateType(RustCrateType::Binary, "", "src/lib.rs"), RustCrateType::Binary);
}

void TestCompilerStrategies::testRustCrateInfoFromCargoToml() {
    const auto* strat = LanguageStrategyRegistry::instance().getStrategy(Language::Rust, "rust:1.80");
    QVERIFY(strat);

    // Single-file mode: fixed name, no manifest, no warnings.
    CompilationRequest single;
    single.language = Language::Rust;
    single.sourceCode = "pub fn f() {}";
    auto info = RustStrategy::crateInfo(single);
    QCOMPARE(info.crateName, QString("octos_snippet"));
    QVERIFY(info.manifestPath.isEmpty());
    QVERIFY(info.warnings.isEmpty());

    // Name with '-' is sanitised; edition in single quotes; comments ignored; dependencies warned about.
    CompilationRequest req = single;
    req.mainFilePath = "src/lib.rs";
    req.extraFiles.append({QString("Cargo.toml"),
                           QString("[package] # the crate\nname = \"my-crate\" # trailing\nedition = '2018'\n\n"
                                   "[dependencies]\nserde = \"1\"\n")});
    info = RustStrategy::crateInfo(req);
    QCOMPARE(info.crateName, QString("my_crate"));
    QCOMPARE(info.edition, QString("2018"));
    QCOMPARE(info.manifestPath, QString("Cargo.toml"));
    QCOMPARE(info.warnings.size(), 1);
    QVERIFY(info.warnings.first().contains("dependencies"));
    QCOMPARE(strat->requestWarnings(req), info.warnings);
    QStringList args = strat->buildCompilerArgs(req, "/input/src/lib.rs", "/output");
    QVERIFY(args.join(' ').contains("--crate-name my_crate"));
    QVERIFY(args.contains("--edition=2018"));
    QVERIFY(args.contains("--crate-type=lib"));
    req.userArgs = "--edition 2024";                        // the user's flag overrides the manifest
    args = strat->buildCompilerArgs(req, "/input/src/lib.rs", "/output");
    QVERIFY(args.contains("--edition=2024"));
    QVERIFY(!args.contains("--edition=2018"));

    // The nearest manifest wins over a workspace root; [lib] name renames the library crate.
    CompilationRequest ws = single;
    ws.mainFilePath = "crates/alpha/src/lib.rs";
    ws.extraFiles.append({QString("Cargo.toml"), QString("[workspace]\nmembers = [\"crates/alpha\"]\n")});
    ws.extraFiles.append({QString("crates/alpha/Cargo.toml"),
                          QString("[package]\nname = \"alpha\"\nedition = \"2021\"\n[lib]\nname = \"alpha_core\"\n"
                                  "[dev-dependencies]\nproptest = \"1\"\n")});
    info = RustStrategy::crateInfo(ws);
    QCOMPARE(info.manifestPath, QString("crates/alpha/Cargo.toml"));
    QCOMPARE(info.crateName, QString("alpha_core"));
    QVERIFY2(info.warnings.isEmpty(), qPrintable(info.warnings.join('\n')));   // dev-dependencies are not needed

    // src/bin/<name>.rs is a binary named after the file; a build script is reported.
    CompilationRequest bin = single;
    bin.mainFilePath = "src/bin/tool-x.rs";
    bin.extraFiles.append({QString("Cargo.toml"), QString("[package]\nname = \"pkg\"\n[dependencies.rand]\nversion = \"0.8\"\n")});
    bin.extraFiles.append({QString("build.rs"), QString("fn main() {}")});
    info = RustStrategy::crateInfo(bin);
    QCOMPARE(info.crateName, QString("tool_x"));
    QCOMPARE(info.warnings.size(), 2);
    QVERIFY(strat->buildCompilerArgs(bin, "/input/src/bin/tool-x.rs", "/output").contains("--crate-type=bin"));

    // Names that are not identifiers fall back to the fixed name with a warning; odd editions are ignored.
    CompilationRequest bad = single;
    bad.mainFilePath = "lib.rs";
    bad.extraFiles.append({QString("Cargo.toml"), QString("[package]\nname = \"1st crate\"\nedition = \"next\"\n")});
    info = RustStrategy::crateInfo(bad);
    QCOMPARE(info.crateName, QString("octos_snippet"));
    QVERIFY(info.edition.isEmpty());
    QCOMPARE(info.warnings.size(), 2);
}

void TestCompilerStrategies::testProjectIncludeFlags() {
    QCOMPARE(StrategyUtils::containerProjectRoot("/input/src/app/main.cpp", "src/app/main.cpp"), QString("/input"));
    QCOMPARE(StrategyUtils::containerProjectRoot("/input/src/source.cpp", "src/main.txt"), QString("/input"));
    QCOMPARE(StrategyUtils::containerProjectRoot("/input/source.cpp", QString()), QString("/input"));

    CompilationRequest req;
    req.language = Language::Cpp;
    req.compilerImage = "gcc:13";
    req.sourceCode = "int main(){}";
    const auto* gcc = LanguageStrategyRegistry::instance().getStrategy(Language::Cpp, "gcc:13");
    // Single-file mode: no include flags at all.
    QVERIFY(StrategyUtils::projectIncludeFlags(req, "/input/source.cpp").isEmpty());
    for (const QString& a : gcc->buildCompilerArgs(req, "/input/source.cpp", "/output")) QVERIFY(!a.startsWith("-I"));

    // Project mode: root, main directory, target include paths (unsafe and duplicate ones dropped).
    req.mainFilePath = "src/app/main.cpp";
    req.includePaths = {"include", "../evil", "/usr/include", "include", "third_party//json/./include"};
    const QStringList flags = StrategyUtils::projectIncludeFlags(req, "/input/src/app/main.cpp");
    QCOMPARE(flags, QStringList({"-I/input", "-I/input/src/app", "-I/input/include", "-I/input/third_party/json/include"}));
    const QStringList args = gcc->buildCompilerArgs(req, "/input/src/app/main.cpp", "/output");
    for (const QString& f : flags) QVERIFY(args.contains(f));
    const auto* clang = LanguageStrategyRegistry::instance().getStrategy(Language::Cpp, "silkeh/clang:17");
    QVERIFY(clang->buildCompilerArgs(req, "/input/src/app/main.cpp", "/output").contains("-I/input/include"));
}

void TestCompilerStrategies::testAcceptsSourceName() {
    auto& reg = LanguageStrategyRegistry::instance();
    const auto* cpp = reg.getStrategy(Language::Cpp, "gcc:13");
    QVERIFY(cpp->acceptsSourceName("main.cpp"));
    QVERIFY(cpp->acceptsSourceName("main.cc"));
    QVERIFY(cpp->acceptsSourceName("main.cxx"));
    QVERIFY(!cpp->acceptsSourceName("main.c"));
    QVERIFY(!cpp->acceptsSourceName("main.h"));
    QVERIFY(!cpp->acceptsSourceName("notes.txt"));
    QVERIFY(!cpp->acceptsSourceName("Makefile"));
    const auto* c = reg.getStrategy(Language::C, "silkeh/clang:17");
    QVERIFY(c->acceptsSourceName("main.c"));
    QVERIFY(!c->acceptsSourceName("main.cpp"));
    QVERIFY(reg.getStrategy(Language::Rust, "rust:1")->acceptsSourceName("lib.rs"));
    QVERIFY(!reg.getStrategy(Language::Rust, "rust:1")->acceptsSourceName("lib.rs.bak"));
    QVERIFY(reg.getStrategy(Language::Java, "openjdk:21")->acceptsSourceName("App.java"));
    QVERIFY(reg.getStrategy(Language::Python, "python:3")->acceptsSourceName("tool.py"));
    QVERIFY(!reg.getStrategy(Language::Python, "python:3")->acceptsSourceName(".py"));
}

void TestCompilerStrategies::testJavaSourceRoot() {
    QCOMPARE(JavaStrategy::sourceRoot("package com.acme;\npublic class App {}", "/input/src/main/java/com/acme/App.java"),
             QString("/input/src/main/java"));
    QCOMPARE(JavaStrategy::sourceRoot("/* header */\npackage com . acme ;\nclass App {}", "/input/com/acme/App.java"),
             QString("/input"));
    // File not inside its package directory (single-file mode): its own directory.
    QCOMPARE(JavaStrategy::sourceRoot("package com.acme;\nclass App {}", "/input/App.java"), QString("/input"));
    QCOMPARE(JavaStrategy::sourceRoot("class App {}", "/input/src/App.java"), QString("/input/src"));

    CompilationRequest req;
    req.language = Language::Java;
    req.compilerImage = "openjdk:21";
    req.sourceCode = "package demo;\npublic class App {}";
    const QStringList args = LanguageStrategyRegistry::instance().getStrategy(Language::Java, "openjdk:21")
                                 ->buildCompilerArgs(req, "/input/src/demo/App.java", "/output");
    QVERIFY(args.join(' ').contains("-sourcepath"));
    QCOMPARE(args.last(), QString("/input/src"));
}

OCTOS_TEST_MAIN(TestCompilerStrategies)
#include "test_compiler_strategies.moc"

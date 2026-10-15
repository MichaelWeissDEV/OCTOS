#include "../TestSupport.h"
#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>

#include "AssemblyParser.h"
#include "AssemblyModel.h"
#include "WorkspaceModel.h"
#include "CompilationArtifact.h"

class TestParserAndWorkspace : public QObject {
    Q_OBJECT

private slots:
    void testAssemblyParserBasicX86();
    void testAssemblyParserControlFlow();
    void testAssemblyParserOperandsAndDirectives();
    void testInstructionFrequencies();
    void testCompilationArtifact();
    void testWorkspaceManagement();
    void testWorkspaceJsonRoundTrip();
    void testWorkspaceFilePersistence();
};

void TestParserAndWorkspace::testAssemblyParserBasicX86()
{
    QString asmText =
        "\t.file\t\"example.cpp\"\n"
        "\t.text\n"
        "\t.globl\tadd_numbers\n"
        "\t.type\tadd_numbers, @function\n"
        "add_numbers:\n"
        "\tpushq\t%rbp\n"
        "\tmovq\t%rsp, %rbp\n"
        "\tmovl\t%edi, -4(%rbp)\n"
        "\tmovl\t%esi, -8(%rbp)\n"
        "\tmovl\t-4(%rbp), %edx\n"
        "\tmovl\t-8(%rbp), %eax\n"
        "\taddl\t%edx, %eax\n"
        "\tpopq\t%rbp\n"
        "\tret\n"
        "\t.size\tadd_numbers, .-add_numbers\n";

    AssemblyModule mod = AssemblyParser::parseX86_64(asmText);
    QCOMPARE(mod.architecture, TargetArchitecture::X86_64);
    QCOMPARE(mod.functions.size(), 1);

    const Function& fn = mod.functions.first();
    QCOMPARE(fn.name, QString("add_numbers"));
    QVERIFY(fn.totalInstructions() >= 9);

    // Verify first instruction
    QVERIFY(!fn.basicBlocks.isEmpty());
    const BasicBlock& bb = fn.basicBlocks.first();
    QVERIFY(!bb.instructions.isEmpty());
    QCOMPARE(bb.instructions.first().mnemonic.toLower(), QString("pushq"));
    QCOMPARE(bb.instructions.last().mnemonic.toLower(), QString("ret"));
}

void TestParserAndWorkspace::testAssemblyParserControlFlow()
{
    QString asmText =
        "test_branch:\n"
        "\tcmpl\t$0, %edi\n"
        "\tjne\t.L2\n"
        "\tmovl\t$10, %eax\n"
        "\tjmp\t.L3\n"
        ".L2:\n"
        "\tmovl\t$20, %eax\n"
        ".L3:\n"
        "\tret\n";

    AssemblyModule mod = AssemblyParser::parseX86_64(asmText);
    QCOMPARE(mod.functions.size(), 1);

    const Function& fn = mod.functions.first();
    // Should have multiple basic blocks due to labels .L2 and .L3
    QVERIFY(fn.basicBlocks.size() >= 3);

    // Find branch instruction in first block
    const BasicBlock& bb0 = fn.basicBlocks.first();
    bool foundBranch = false;
    for (const auto& inst : bb0.instructions) {
        if (inst.isBranch) {
            foundBranch = true;
            QVERIFY(inst.branchTarget == ".L2" || inst.branchTarget == ".L3");
        }
    }
    QVERIFY(foundBranch);
}

void TestParserAndWorkspace::testAssemblyParserOperandsAndDirectives()
{
    QString asmText =
        "\t.loc 1 15 0\n"
        "\tmovl\t$42, (%rax,%rcx,4)\n";

    AssemblyModule mod = AssemblyParser::parseX86_64(asmText);
    // At least one instruction parsed
    int total = mod.totalInstructions();
    QVERIFY(total >= 1);

    for (const auto& fn : mod.functions) {
        for (const auto& bb : fn.basicBlocks) {
            for (const auto& inst : bb.instructions) {
                if (inst.mnemonic == "movl") {
                    QCOMPARE(inst.operands.size(), 2);
                    QVERIFY(inst.operands[0].isImmediate);
                    QVERIFY(inst.operands[1].isMemory);
                    QCOMPARE(inst.sourceLoc.line, 15);
                }
            }
        }
    }
}

void TestParserAndWorkspace::testInstructionFrequencies()
{
    QString asmText =
        "func:\n"
        "\tmovl\t$1, %eax\n"
        "\tmovl\t$2, %ebx\n"
        "\taddl\t%ebx, %eax\n"
        "\tret\n";

    AssemblyModule mod = AssemblyParser::parseX86_64(asmText);
    QMap<QString, int> freqs = mod.instructionFrequencies();
    QCOMPARE(freqs.value("movl"), 2);
    QCOMPARE(freqs.value("addl"), 1);
    QCOMPARE(freqs.value("ret"), 1);
}

void TestParserAndWorkspace::testCompilationArtifact()
{
    CompilationArtifact artifact;
    artifact.type = ArtifactType::Assembly;
    artifact.name = "output.s";
    artifact.containerPath = "/output/output.s";
    artifact.sizeBytes = 1024;
    artifact.content = "pushq %rbp";

    QCOMPARE(artifact.type, ArtifactType::Assembly);
    QCOMPARE(artifact.name, QString("output.s"));
    QCOMPARE(artifact.sizeBytes, 1024);
    QCOMPARE(artifactTypeToString(artifact.type), QString("Assembly"));

    CompilationArtifact llvmIr;
    llvmIr.type = ArtifactType::LLVM_IR;
    llvmIr.name = "test.ll";
    llvmIr.containerPath = "/output/test.ll";
    llvmIr.sizeBytes = 512;
    llvmIr.content = "; ModuleID = 'test.cpp'\n";

    QCOMPARE(artifactTypeToString(llvmIr.type), QString("LLVM IR"));
    QCOMPARE(llvmIr.mimeType, QString("text/plain"));
    QCOMPARE(llvmIr.content, QString("; ModuleID = 'test.cpp'\n"));
}

void TestParserAndWorkspace::testWorkspaceManagement()
{
    Workspace ws("OCTOS Test Project");
    QCOMPARE(ws.name(), QString("OCTOS Test Project"));
    QCOMPARE(ws.projectFormatVersion(), QString("octos-project-v1"));

    // Add source files
    SourceFile sf1;
    sf1.path = "main.cpp";
    sf1.content = "int main() { return 0; }";
    sf1.isHeader = false;
    sf1.language = Language::Cpp;
    ws.addSourceFile(sf1);

    SourceFile sf2;
    sf2.path = "utils.h";
    sf2.content = "#pragma once\nint helper();";
    sf2.isHeader = true;
    sf2.language = Language::Cpp;
    ws.addSourceFile(sf2);

    QCOMPARE(ws.sourceFiles().size(), 2);
    QVERIFY(ws.hasSourceFile("main.cpp"));
    QVERIFY(ws.hasSourceFile("utils.h"));
    QVERIFY(!ws.hasSourceFile("nonexistent.cpp"));

    SourceFile found = ws.findSourceFile("utils.h");
    QCOMPARE(found.path, QString("utils.h"));
    QVERIFY(found.isHeader);

    // Remove source file
    QVERIFY(ws.removeSourceFile("utils.h"));
    QCOMPARE(ws.sourceFiles().size(), 1);
    QVERIFY(!ws.hasSourceFile("utils.h"));

    // Profiles
    CompilerProfile prof;
    prof.id = "prof-gcc-o2";
    prof.name = "GCC 13 -O2";
    prof.compilerImage = "gcc:13";
    prof.userFlags = "-O2 -Wall";
    prof.syntaxFlavor = SyntaxFlavor::Intel;
    ws.addProfile(prof);

    QCOMPARE(ws.profiles().size(), 1);
    QCOMPARE(ws.profiles().first().name, QString("GCC 13 -O2"));

    // Targets
    BuildTarget tgt;
    tgt.name = "App";
    tgt.mainSourceFile = "main.cpp";
    tgt.sourceFiles = QStringList() << "main.cpp";
    tgt.includePaths = QStringList() << "/include";
    ws.addTarget(tgt);

    QCOMPARE(ws.targets().size(), 1);
    QCOMPARE(ws.targets().first().name, QString("App"));

    // Configuration
    CompilationConfiguration config;
    config.activeProfileId = "prof-gcc-o2";
    config.activeTargetName = "App";
    config.environmentVars["DEBUG"] = "1";
    ws.setConfiguration(config);

    QCOMPARE(ws.configuration().activeProfileId, QString("prof-gcc-o2"));
    QCOMPARE(ws.configuration().environmentVars.value("DEBUG"), QString("1"));
}

void TestParserAndWorkspace::testWorkspaceJsonRoundTrip()
{
    Workspace ws("RoundTrip Project");

    SourceFile sf;
    sf.path = "src/calc.rs";
    sf.content = "pub fn add(a: i32, b: i32) -> i32 { a + b }";
    sf.isHeader = false;
    sf.language = Language::Rust;
    ws.addSourceFile(sf);

    CompilerProfile prof;
    prof.id = "rust-release";
    prof.name = "Rustc Release";
    prof.compilerImage = "rust:1.75";
    prof.userFlags = "--crate-type=lib -C opt-level=3";
    prof.syntaxFlavor = SyntaxFlavor::Intel;
    ws.addProfile(prof);

    BuildTarget tgt;
    tgt.name = "libcalc";
    tgt.mainSourceFile = "src/calc.rs";
    tgt.sourceFiles = QStringList() << "src/calc.rs";
    ws.addTarget(tgt);

    CompilationConfiguration config;
    config.activeProfileId = "rust-release";
    config.activeTargetName = "libcalc";
    config.environmentVars["RUST_BACKTRACE"] = "1";
    ws.setConfiguration(config);

    QByteArray json = ws.toJson();
    QVERIFY(!json.isEmpty());

    QString errorMsg;
    Workspace loaded = Workspace::fromJson(json, &errorMsg);
    QVERIFY2(errorMsg.isEmpty(), qPrintable(errorMsg));
    QCOMPARE(loaded.name(), QString("RoundTrip Project"));
    QCOMPARE(loaded.projectFormatVersion(), QString("octos-project-v1"));

    QCOMPARE(loaded.sourceFiles().size(), 1);
    QCOMPARE(loaded.sourceFiles().first().path, QString("src/calc.rs"));
    QCOMPARE(loaded.sourceFiles().first().content, sf.content);
    QCOMPARE(loaded.sourceFiles().first().language, Language::Rust);

    QCOMPARE(loaded.profiles().size(), 1);
    QCOMPARE(loaded.profiles().first().id, QString("rust-release"));
    QCOMPARE(loaded.profiles().first().userFlags, QString("--crate-type=lib -C opt-level=3"));

    QCOMPARE(loaded.targets().size(), 1);
    QCOMPARE(loaded.targets().first().name, QString("libcalc"));

    QCOMPARE(loaded.configuration().activeProfileId, QString("rust-release"));
    QCOMPARE(loaded.configuration().environmentVars.value("RUST_BACKTRACE"), QString("1"));
}

void TestParserAndWorkspace::testWorkspaceFilePersistence()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString filePath = tempDir.path() + "/test_workspace.octos";

    Workspace ws("File Persistence Test");
    SourceFile sf;
    sf.path = "main.py";
    sf.content = "import dis\ndef foo(): pass\ndis.dis(foo)\n";
    sf.language = Language::Python;
    ws.addSourceFile(sf);

    QString errorMsg;
    bool saved = ws.saveToFile(filePath, &errorMsg);
    QVERIFY2(saved, qPrintable(errorMsg));
    QVERIFY(QFile::exists(filePath));

    Workspace loaded = Workspace::loadFromFile(filePath, &errorMsg);
    QVERIFY2(errorMsg.isEmpty(), qPrintable(errorMsg));
    QCOMPARE(loaded.name(), QString("File Persistence Test"));
    QCOMPARE(loaded.sourceFiles().size(), 1);
    QCOMPARE(loaded.sourceFiles().first().path, QString("main.py"));
    QCOMPARE(loaded.sourceFiles().first().content, sf.content);
}

OCTOS_TEST_MAIN(TestParserAndWorkspace)
#include "test_parser_and_workspace.moc"

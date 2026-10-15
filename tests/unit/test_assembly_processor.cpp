#include "../TestSupport.h"
#include <QTest>
#include "utils/AssemblyTextProcessor.h"

class TestAssemblyProcessor : public QObject {
    Q_OBJECT

private slots:
    void testFilterCfiDirectives();
    void testFilterDataAndSegmentDirectives();
    void testFilterComments();
    void testSourceToAssemblyLineMapping();
    void testDemangle();
};

void TestAssemblyProcessor::testFilterCfiDirectives() {
    QString raw = "main:\n"
                  "\t.cfi_startproc\n"
                  "\tpushq %rbp\n"
                  "\t.cfi_def_cfa_offset 16\n"
                  "\tmovq %rsp, %rbp\n"
                  "\t.cfi_endproc\n";

    FilterSettings settings;
    settings.showCfiDirectives = false;
    settings.showComments = true;
    settings.showSegmentDirectives = true;
    settings.showDataDirectives = true;

    ProcessedAssemblyResult result = AssemblyTextProcessor::processAssembly(raw, settings);
    QVERIFY(!result.filteredText.contains(".cfi_startproc"));
    QVERIFY(!result.filteredText.contains(".cfi_def_cfa_offset"));
    QVERIFY(!result.filteredText.contains(".cfi_endproc"));
    QVERIFY(result.filteredText.contains("pushq %rbp"));
    QVERIFY(result.filteredText.contains("movq %rsp, %rbp"));

    // If enabled, they should be present
    settings.showCfiDirectives = true;
    result = AssemblyTextProcessor::processAssembly(raw, settings);
    QVERIFY(result.filteredText.contains(".cfi_startproc"));
}

void TestAssemblyProcessor::testFilterDataAndSegmentDirectives() {
    QString raw = "\t.file \"test.cpp\"\n"
                  "\t.text\n"
                  "\t.section .rodata\n"
                  "\t.align 4\n"
                  "\t.globl foo\n"
                  "foo:\n"
                  "\tret\n";

    FilterSettings settings;
    settings.showSegmentDirectives = false;
    settings.showDataDirectives = false;

    ProcessedAssemblyResult result = AssemblyTextProcessor::processAssembly(raw, settings);
    QVERIFY(!result.filteredText.contains(".section .rodata"));
    QVERIFY(!result.filteredText.contains(".text"));
    QVERIFY(!result.filteredText.contains(".globl foo"));
    QVERIFY(!result.filteredText.contains(".align 4"));
    QVERIFY(result.filteredText.contains("foo:"));
    QVERIFY(result.filteredText.contains("ret"));
}

void TestAssemblyProcessor::testFilterComments() {
    QString raw = "main: # function start\n"
                  "\tmov eax, 0 // return 0\n"
                  "\tret ; return\n";

    FilterSettings settings;
    settings.showComments = false;

    ProcessedAssemblyResult result = AssemblyTextProcessor::processAssembly(raw, settings);
    QVERIFY(!result.filteredText.contains("# function start"));
    QVERIFY(!result.filteredText.contains("// return 0"));
    QVERIFY(!result.filteredText.contains("; return"));
    QVERIFY(result.filteredText.contains("mov eax, 0"));
    QVERIFY(result.filteredText.contains("ret"));
}

void TestAssemblyProcessor::testSourceToAssemblyLineMapping() {
    QString raw = "main:\n"
                  "\t.loc 1 10 0\n"
                  "\tmov eax, 1\n"
                  "\t.loc 1 12 0\n"
                  "\tadd eax, 2\n"
                  "\tret\n";

    FilterSettings settings;
    settings.showDebugInfo = false;

    ProcessedAssemblyResult result = AssemblyTextProcessor::processAssembly(raw, settings);
    QVERIFY(!result.filteredText.contains(".loc"));

    // Source line 10 should map to "mov eax, 1"
    QVERIFY(result.sourceToDisplay.contains(10));
    // Source line 12 should map to "add eax, 2"
    QVERIFY(result.sourceToDisplay.contains(12));
}

void TestAssemblyProcessor::testDemangle() {
    QString raw = "_Z3fooi:\n\tret\n";
    FilterSettings settings;
    settings.demangleIdentifiers = true;

    ProcessedAssemblyResult result = AssemblyTextProcessor::processAssembly(raw, settings);
    QVERIFY(result.filteredText.contains("foo(int)"));
}

OCTOS_TEST_MAIN(TestAssemblyProcessor)
#include "test_assembly_processor.moc"

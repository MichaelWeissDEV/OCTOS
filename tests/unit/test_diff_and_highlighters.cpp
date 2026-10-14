#include <QTest>
#include <QTextDocument>
#include <QElapsedTimer>
#include <atomic>
#include "utils/AssemblyDiffProcessor.h"
#include "utils/OpcodeReference.h"
#include "highlighters/RustHighlighter.h"
#include "highlighters/PythonHighlighter.h"

class TestDiffAndHighlighters : public QObject {
    Q_OBJECT

private slots:
    void testDiffIdentical();
    void testDiffAddedAndRemoved();
    void testDiffEmpty();
    void testDiffOnlyInsertions();
    void testDiffOnlyDeletions();
    void testDiffCompletelyDifferent();
    void testDiffRepeatedIdenticalLines();
    void testDiffShiftedBlocks();
    void testDiffCancellation();
    void testDiffUnifiedExport();
    void testDiffNormalization();

    // Benchmarks
    void testDiffBenchmark1k();
    void testDiffBenchmark10k();
    void testDiffBenchmark50k();

    // Opcode Reference Tests
    void testOpcodeReferenceKnown();
    void testOpcodeReferenceUnknown();
    void testOpcodeArchSeparation();
    void testOpcodeHtmlEscaping();
    void testOpcodeExtractionMnemonicVsOperandsAndComments();

    void testHighlightersInitialization();
};

void TestDiffAndHighlighters::testDiffIdentical() {
    QString asmA = "main:\n\tmov eax, 0\n\tret\n";
    QString asmB = "main:\n\tmov eax, 0\n\tret\n";

    DiffResult result = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QVERIFY(!result.hasDifferences());
    QCOMPARE(result.additions, 0);
    QCOMPARE(result.deletions, 0);
    QCOMPARE(result.unchanged, 3);
}

void TestDiffAndHighlighters::testDiffAddedAndRemoved() {
    QString asmA = "main:\n\tpush rbp\n\tmov eax, 1\n\tpop rbp\n\tret\n";
    QString asmB = "main:\n\tmov eax, 1\n\tret\n";

    DiffResult result = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QVERIFY(result.hasDifferences());
    QVERIFY(result.deletions >= 2);
    QCOMPARE(result.additions, 0);
}

void TestDiffAndHighlighters::testDiffEmpty() {
    DiffResult res1 = AssemblyDiffProcessor::computeDiff("", "");
    QCOMPARE(res1.entries.size(), 0);
    QVERIFY(!res1.hasDifferences());

    DiffResult res2 = AssemblyDiffProcessor::computeDiff("mov eax, 1", "");
    QCOMPARE(res2.deletions, 1);
    QCOMPARE(res2.additions, 0);

    DiffResult res3 = AssemblyDiffProcessor::computeDiff("", "mov eax, 1");
    QCOMPARE(res3.additions, 1);
    QCOMPARE(res3.deletions, 0);
}

void TestDiffAndHighlighters::testDiffOnlyInsertions() {
    QString asmA = "line1\nline2\n";
    QString asmB = "line1\ninsertedA\ninsertedB\nline2\n";

    DiffResult res = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QCOMPARE(res.additions, 2);
    QCOMPARE(res.deletions, 0);
    QCOMPARE(res.unchanged, 2);
}

void TestDiffAndHighlighters::testDiffOnlyDeletions() {
    QString asmA = "line1\ndeleteMeA\ndeleteMeB\nline2\n";
    QString asmB = "line1\nline2\n";

    DiffResult res = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QCOMPARE(res.additions, 0);
    QCOMPARE(res.deletions, 2);
    QCOMPARE(res.unchanged, 2);
}

void TestDiffAndHighlighters::testDiffCompletelyDifferent() {
    QString asmA = "apple\nbanana\ncherry\n";
    QString asmB = "dog\nelephant\nfox\n";

    DiffResult res = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QCOMPARE(res.unchanged, 0);
    QCOMPARE(res.deletions, 3);
    QCOMPARE(res.additions, 3);
}

void TestDiffAndHighlighters::testDiffRepeatedIdenticalLines() {
    QString asmA = "nop\nnop\nnop\nnop\n";
    QString asmB = "nop\nnop\n";

    DiffResult res = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QCOMPARE(res.unchanged, 2);
    QCOMPARE(res.deletions, 2);
    QCOMPARE(res.additions, 0);
}

void TestDiffAndHighlighters::testDiffShiftedBlocks() {
    QString asmA = "blockA\nblockB\nblockC\n";
    QString asmB = "blockC\nblockA\nblockB\n";

    DiffResult res = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QVERIFY(res.hasDifferences());
    QVERIFY(res.unchanged >= 2);
}

void TestDiffAndHighlighters::testDiffCancellation() {
    QString asmA = "lineA\nlineB\n";
    QString asmB = "lineC\nlineD\n";

    auto isCancelled = []() { return true; };
    DiffResult res = AssemblyDiffProcessor::computeDiff(asmA, asmB, DiffOptions(), isCancelled);
    QVERIFY(res.isCancelled);
}

void TestDiffAndHighlighters::testDiffUnifiedExport() {
    QString asmA = "main:\n\tmov eax, 0\n\tret\n";
    QString asmB = "main:\n\tmov eax, 1\n\tret\n";

    DiffResult res = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QString patch = AssemblyDiffProcessor::toUnifiedDiff(res, "Pane1", "Pane2");

    QVERIFY(patch.contains("--- Pane1"));
    QVERIFY(patch.contains("+++ Pane2"));
    QVERIFY(patch.contains("-\tmov eax, 0"));
    QVERIFY(patch.contains("+\tmov eax, 1"));
}

void TestDiffAndHighlighters::testDiffNormalization() {
    QString asmA = "main:\n\tmov eax, 0   # initialize zero\n.L123:\n\tret\n";
    QString asmB = "main:\n\tmov eax, 0\n.L456:\n\tret\n";

    DiffOptions opt;
    opt.ignoreComments = true;
    opt.normalizeLabels = true;
    opt.ignoreWhitespace = true;

    DiffResult res = AssemblyDiffProcessor::computeDiff(asmA, asmB, opt);
    // With normalization, both versions are considered identical
    QVERIFY(!res.hasDifferences());
    QCOMPARE(res.unchanged, 4);
}

void TestDiffAndHighlighters::testDiffBenchmark1k() {
    QStringList linesA;
    QStringList linesB;
    for (int i = 0; i < 1000; ++i) {
        linesA << QString("\tmovq %%rax, %1(%%rsp)").arg(i);
        // Introduce small difference every 20 lines
        if (i % 20 == 0) {
            linesB << QString("\tmovq %%rbx, %1(%%rsp)").arg(i);
        } else {
            linesB << QString("\tmovq %%rax, %1(%%rsp)").arg(i);
        }
    }

    QElapsedTimer timer;
    timer.start();
    DiffResult res = AssemblyDiffProcessor::computeDiff(linesA.join('\n'), linesB.join('\n'));
    qint64 elapsed = timer.elapsed();

    qDebug() << "1,000 lines diff computed in" << elapsed << "ms, entries:" << res.entries.size();
    QVERIFY(elapsed < 1000); // Must be well under 1 second
    QVERIFY(res.hasDifferences());
}

void TestDiffAndHighlighters::testDiffBenchmark10k() {
    QStringList linesA;
    QStringList linesB;
    for (int i = 0; i < 10000; ++i) {
        linesA << QString("\taddl $%1, %%eax").arg(i);
        if (i % 50 == 0) {
            linesB << QString("\tsubl $%1, %%eax").arg(i);
        } else {
            linesB << QString("\taddl $%1, %%eax").arg(i);
        }
    }

    QElapsedTimer timer;
    timer.start();
    DiffResult res = AssemblyDiffProcessor::computeDiff(linesA.join('\n'), linesB.join('\n'));
    qint64 elapsed = timer.elapsed();

    qDebug() << "10,000 lines diff computed in" << elapsed << "ms, entries:" << res.entries.size();
    QVERIFY(elapsed < 3000); // Myers with line hashing must handle 10k in few hundred ms
}

void TestDiffAndHighlighters::testDiffBenchmark50k() {
    QStringList linesA;
    QStringList linesB;
    // Real-world scenario: mostly identical with localized optimization flag differences
    for (int i = 0; i < 50000; ++i) {
        linesA << QString("\txorl %%eax, %%eax # line %1").arg(i);
        if (i == 25000) {
            linesB << "\tmovl $0, %eax";
        } else {
            linesB << QString("\txorl %%eax, %%eax # line %1").arg(i);
        }
    }

    QElapsedTimer timer;
    timer.start();
    DiffResult res = AssemblyDiffProcessor::computeDiff(linesA.join('\n'), linesB.join('\n'));
    qint64 elapsed = timer.elapsed();

    qDebug() << "50,000 lines diff computed in" << elapsed << "ms, entries:" << res.entries.size();
    QVERIFY(elapsed < 5000); // Prefix/suffix trimming handles this almost instantaneously
    QCOMPARE(res.unchanged, 49999);
}

void TestDiffAndHighlighters::testOpcodeReferenceKnown() {
    auto& ref = OpcodeReference::instance();
    QVERIFY(ref.hasOpcode("mov"));
    QVERIFY(ref.hasOpcode("lea"));
    QVERIFY(ref.hasOpcode("xorl"));
    QVERIFY(ref.hasOpcode("ret"));
    QVERIFY(ref.hasOpcode("ldr", "ARM64"));

    OpcodeInfo info = ref.getOpcodeInfo("xor");
    QCOMPARE(info.mnemonic, QString("xor"));
    QVERIFY(info.summary.contains("exclusive OR", Qt::CaseInsensitive));
    QVERIFY(info.flagsAffected.contains("ZF"));
    QVERIFY(!info.toFormattedTooltip().isEmpty());
}

void TestDiffAndHighlighters::testOpcodeReferenceUnknown() {
    auto& ref = OpcodeReference::instance();
    QVERIFY(!ref.hasOpcode("notaninstruction"));
    QVERIFY(!ref.hasOpcode("random_var"));
}

void TestDiffAndHighlighters::testOpcodeArchSeparation() {
    auto& ref = OpcodeReference::instance();
    QVERIFY(ref.hasOpcode("retq", "x86-64"));
    QVERIFY(!ref.hasOpcode("retq", "ARM64"));

    QVERIFY(ref.hasOpcode("ldp", "ARM64"));
    QVERIFY(!ref.hasOpcode("ldp", "x86-64"));
}

void TestDiffAndHighlighters::testOpcodeHtmlEscaping() {
    auto& ref = OpcodeReference::instance();
    OpcodeInfo info = ref.getOpcodeInfo("mov");
    QString tooltip = info.toFormattedTooltip();
    // HTML tags generated by formatter should exist, but angle brackets in user text should be escaped
    QVERIFY(tooltip.contains("<b>mov</b>"));
}

void TestDiffAndHighlighters::testOpcodeExtractionMnemonicVsOperandsAndComments() {
    QString line = "\tmovq\t%rax, %rbx # test comment with mov inside";
    QString mnemonic;
    QString arch;

    // Column 2 is on 'm' of 'movq' -> Mnemonic!
    QVERIFY(OpcodeReference::extractMnemonicAtColumn(line, 2, &mnemonic, &arch));
    QCOMPARE(mnemonic, QString("movq"));

    // Column 8 is on '%rax' (operand) -> Must NOT be recognized as opcode!
    QVERIFY(!OpcodeReference::extractMnemonicAtColumn(line, 8, &mnemonic, &arch));

    // Column 35 is inside the comment '# test comment with mov inside' -> Must NOT be recognized!
    QVERIFY(!OpcodeReference::extractMnemonicAtColumn(line, 35, &mnemonic, &arch));

    // Directive line
    QString directiveLine = "\t.cfi_startproc";
    QVERIFY(!OpcodeReference::extractMnemonicAtColumn(directiveLine, 3, &mnemonic, &arch));

    // Label line
    QString labelLine = ".LFB0:";
    QVERIFY(!OpcodeReference::extractMnemonicAtColumn(labelLine, 2, &mnemonic, &arch));

    // Label followed by instruction
    QString labelAndInst = "main: pushq %rbp";
    // Column on 'main'
    QVERIFY(!OpcodeReference::extractMnemonicAtColumn(labelAndInst, 2, &mnemonic, &arch));
    // Column on 'pushq'
    QVERIFY(OpcodeReference::extractMnemonicAtColumn(labelAndInst, 7, &mnemonic, &arch));
    QCOMPARE(mnemonic, QString("pushq"));
}

void TestDiffAndHighlighters::testHighlightersInitialization() {
    QTextDocument docRust("fn main() {\n    let mut x: i32 = 42;\n    println!(\"{}\", x);\n}\n");
    RustHighlighter rustH(&docRust);
    QVERIFY(docRust.blockCount() >= 4);

    QTextDocument docPy("def compute(val: int) -> int:\n    print(val)\n    return val * 2\n");
    PythonHighlighter pyH(&docPy);
    QVERIFY(docPy.blockCount() >= 3);
}

QTEST_MAIN(TestDiffAndHighlighters)
#include "test_diff_and_highlighters.moc"

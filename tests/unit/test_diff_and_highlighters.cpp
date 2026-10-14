#include <QTest>
#include <QTextDocument>
#include "utils/AssemblyDiffProcessor.h"
#include "utils/OpcodeReference.h"
#include "highlighters/RustHighlighter.h"
#include "highlighters/PythonHighlighter.h"

class TestDiffAndHighlighters : public QObject {
    Q_OBJECT

private slots:
    void testDiffIdentical();
    void testDiffAddedAndRemoved();
    void testOpcodeReferenceKnown();
    void testOpcodeReferenceUnknown();
    void testHighlightersInitialization();
};

void TestDiffAndHighlighters::testDiffIdentical() {
    QString asmA = "main:\n\tmov eax, 0\n\tret\n";
    QString asmB = "main:\n\tmov eax, 0\n\tret\n";

    DiffResult result = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QVERIFY(!result.hasDifferences());
    QCOMPARE(result.additions, 0);
    QCOMPARE(result.deletions, 0);
}

void TestDiffAndHighlighters::testDiffAddedAndRemoved() {
    QString asmA = "main:\n\tpush rbp\n\tmov eax, 1\n\tpop rbp\n\tret\n";
    QString asmB = "main:\n\tmov eax, 1\n\tret\n";

    DiffResult result = AssemblyDiffProcessor::computeDiff(asmA, asmB);
    QVERIFY(result.hasDifferences());
    QVERIFY(result.deletions > 0);
}

void TestDiffAndHighlighters::testOpcodeReferenceKnown() {
    auto& ref = OpcodeReference::instance();
    QVERIFY(ref.hasOpcode("mov"));
    QVERIFY(ref.hasOpcode("lea"));
    QVERIFY(ref.hasOpcode("xorl"));
    QVERIFY(ref.hasOpcode("ret"));

    OpcodeInfo info = ref.getOpcodeInfo("xor");
    QCOMPARE(info.mnemonic, QString("xor"));
    QVERIFY(info.summary.contains("exclusive OR", Qt::CaseInsensitive));
    QVERIFY(info.flagsAffected.contains("ZF"));
    QVERIFY(!info.toFormattedTooltip().isEmpty());
}

void TestDiffAndHighlighters::testOpcodeReferenceUnknown() {
    auto& ref = OpcodeReference::instance();
    QVERIFY(!ref.hasOpcode("notaninstruction"));
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

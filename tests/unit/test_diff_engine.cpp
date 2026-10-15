#include "../TestSupport.h"
#include <QTest>
#include <QRandomGenerator>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include "utils/AssemblyDiffProcessor.h"

namespace {

// Minimal unified-diff applier (strict: context and removed lines must match).
bool applyPatch(const QString& original, const QString& patch, QString* out, QString* err) {
    QStringList src = original.split('\n');
    bool srcNl = original.isEmpty() || original.endsWith('\n');
    if (!src.isEmpty() && src.last().isEmpty()) src.removeLast();

    QStringList dst;
    int pos = 0;  // next unconsumed source line
    bool dstNl = srcNl;
    const QStringList pl = patch.split('\n');
    int i = 0;
    while (i < pl.size() && !pl[i].startsWith("@@")) {
        if (pl[i].startsWith("# OCTOS INCOMPLETE")) { *err = "incomplete banner"; return false; }
        ++i;
    }
    static const QRegularExpression hdr(R"(^@@ -(\d+),(\d+) \+(\d+),(\d+) @@)");
    while (i < pl.size()) {
        auto m = hdr.match(pl[i]);
        if (!m.hasMatch()) { if (pl[i].isEmpty() && i == pl.size() - 1) break; *err = "bad hunk header: " + pl[i]; return false; }
        int aStart = m.captured(1).toInt(), aLen = m.captured(2).toInt();
        int bLen = m.captured(4).toInt();
        int target = aLen == 0 ? aStart : aStart - 1;
        while (pos < target) dst << src[pos++];
        ++i;
        int seenA = 0, seenB = 0;
        QString lastKind;
        while (i < pl.size() && !pl[i].startsWith("@@") && !(pl[i].isEmpty() && i == pl.size() - 1)) {
            const QString& l = pl[i];
            if (l.startsWith("\\ No newline")) {
                if (lastKind == "-" ) { /* source had no newline */ }
                else if (lastKind == "+" || lastKind == " ") dstNl = false;  // final dst line has no newline
                ++i; continue;
            }
            const QChar c = l.isEmpty() ? QChar(' ') : l[0];
            const QString t = l.mid(1);
            if (c == ' ') { if (pos >= src.size() || src[pos] != t) { *err = "context mismatch"; return false; } dst << t; dstNl = true; ++pos; ++seenA; ++seenB; }
            else if (c == '-') { if (pos >= src.size() || src[pos] != t) { *err = "removal mismatch"; return false; } ++pos; ++seenA; }
            else if (c == '+') { dst << t; dstNl = true; ++seenB; }
            else { *err = "bad line"; return false; }
            lastKind = QString(c);
            ++i;
        }
        if (seenA != aLen || seenB != bLen) { *err = QString("hunk length mismatch %1/%2 vs %3/%4").arg(seenA).arg(seenB).arg(aLen).arg(bLen); return false; }
    }
    if (pos < src.size()) dstNl = srcNl;  // untouched tail keeps the source's newline state
    while (pos < src.size()) dst << src[pos++];
    *out = dst.join('\n') + ((dstNl && !dst.isEmpty()) ? "\n" : "");
    return true;
}

int lcsLen(const QStringList& a, const QStringList& b) {
    QVector<QVector<int>> t(a.size() + 1, QVector<int>(b.size() + 1, 0));
    for (int i = 1; i <= a.size(); ++i)
        for (int j = 1; j <= b.size(); ++j)
            t[i][j] = a[i-1] == b[j-1] ? t[i-1][j-1] + 1 : qMax(t[i-1][j], t[i][j-1]);
    return t[a.size()][b.size()];
}

QStringList lines(const QString& s) {
    QStringList l = s.split('\n');
    if (!l.isEmpty() && l.last().isEmpty()) l.removeLast();
    return l;
}

QString randomText(QRandomGenerator& rng, int maxLines, int alphabet, bool trailingNl) {
    const int n = rng.bounded(maxLines + 1);
    QStringList l;
    for (int i = 0; i < n; ++i) l << QString("ins%1").arg(rng.bounded(alphabet));
    return l.join('\n') + ((trailingNl && n > 0) ? "\n" : "");
}
}

class TestDiffEngine : public QObject {
    Q_OBJECT
private slots:
    void testRandomizedAgainstLcsAndReconstruction();
    void testRepeatedInstructionsDeterministic();
    void testEmptyInputs();
    void testMemoryStaysLinear();
    void testStepBudgetYieldsFlaggedApproximation();
    void testLineLimitIsNotSilentlyTruncated();
    void testCancellationInsideAlgorithm();
    void testArm64ImmediateIsNotAComment();
    void testX86CommentsAndStrings();
    void testJvmAndPythonNotStrippedLikeX86();
    void testUnknownDialectIsConservative();
    void testLabelRenamingKeepsJumpTargetsDistinct();
    void testUnifiedDiffRoundtrip();
    void testUnifiedDiffRoundtripWithSystemPatch();
    void testIncompleteDiffExport();
};

void TestDiffEngine::testRandomizedAgainstLcsAndReconstruction() {
    QRandomGenerator rng(12345);
    for (int round = 0; round < 400; ++round) {
        const int alphabet = 1 + rng.bounded(6);  // small alphabets => many repeated lines
        const QString a = randomText(rng, 40, alphabet, rng.bounded(2));
        const QString b = randomText(rng, 40, alphabet, rng.bounded(2));
        DiffResult r = AssemblyDiffProcessor::computeDiff(a, b);
        QVERIFY(r.isComplete());

        // 1. Both sides can be reconstructed exactly from the entries.
        QStringList recA, recB;
        for (const auto& e : r.entries) {
            if (e.type != DiffLineType::Added) recA << e.textA;
            if (e.type != DiffLineType::Removed) recB << e.textB;
        }
        QCOMPARE(recA, lines(a));
        QCOMPARE(recB, lines(b));
        // 2. The script is minimal: matched lines == LCS length.
        QCOMPARE(r.unchanged, lcsLen(lines(a), lines(b)));
        QCOMPARE(r.additions, lines(b).size() - r.unchanged);
        QCOMPARE(r.deletions, lines(a).size() - r.unchanged);
    }
}

void TestDiffEngine::testRepeatedInstructionsDeterministic() {
    const QString a = "mov\nmov\nmov\nadd\nmov\nmov\n";
    const QString b = "mov\nadd\nmov\nmov\nmov\nmov\n";
    DiffResult r1 = AssemblyDiffProcessor::computeDiff(a, b);
    DiffResult r2 = AssemblyDiffProcessor::computeDiff(a, b);
    QCOMPARE(r1.entries.size(), r2.entries.size());
    for (int i = 0; i < r1.entries.size(); ++i) {
        QCOMPARE(int(r1.entries[i].type), int(r2.entries[i].type));
        QCOMPARE(r1.entries[i].lineIndexA, r2.entries[i].lineIndexA);
        QCOMPARE(r1.entries[i].lineIndexB, r2.entries[i].lineIndexB);
    }
    QCOMPARE(r1.unchanged, lcsLen(lines(a), lines(b)));
}

void TestDiffEngine::testEmptyInputs() {
    auto r = AssemblyDiffProcessor::computeDiff("", "");
    QVERIFY(r.isComplete() && !r.hasDifferences() && r.entries.isEmpty());
    r = AssemblyDiffProcessor::computeDiff("", "a\nb\n");
    QCOMPARE(r.additions, 2); QVERIFY(r.isComplete());
    r = AssemblyDiffProcessor::computeDiff("a\n", "");
    QCOMPARE(r.deletions, 1); QVERIFY(r.isComplete());
}

void TestDiffEngine::testMemoryStaysLinear() {
    // 30k completely different lines: the old implementation stored a V-vector snapshot per edit
    // step (O(D^2) ints = many GB). The linear-space version must finish with a bounded budget.
    QStringList a, b;
    for (int i = 0; i < 30000; ++i) { a << QString("a%1").arg(i); b << QString("b%1").arg(i); }
    DiffOptions opt;
    opt.maxDiffSteps = 2000000000;   // let it run to completion
    DiffResult r = AssemblyDiffProcessor::computeDiff(a.join('\n'), b.join('\n'), opt);
    QVERIFY(r.isComplete());
    QCOMPARE(r.deletions, 30000);
    QCOMPARE(r.additions, 30000);
    QVERIFY(r.estimatedPeakBytes < 128LL * 1024 * 1024);
}

void TestDiffEngine::testStepBudgetYieldsFlaggedApproximation() {
    QStringList a, b;
    for (int i = 0; i < 3000; ++i) { a << QString("a%1").arg(i % 7); b << QString("b%1").arg(i % 5); }
    DiffOptions opt;
    opt.maxDiffSteps = 1000;
    DiffResult r = AssemblyDiffProcessor::computeDiff(a.join('\n'), b.join('\n'), opt);
    QVERIFY(r.isTruncated);
    QVERIFY(r.isApproximate);
    QVERIFY(!r.isComplete());
    QVERIFY(r.statusMessage.contains("NOT"));
    // Still self-consistent: both sides reconstructable.
    QStringList recA, recB;
    for (const auto& e : r.entries) {
        if (e.type != DiffLineType::Added) recA << e.textA;
        if (e.type != DiffLineType::Removed) recB << e.textB;
    }
    QCOMPARE(recA, a); QCOMPARE(recB, b);
}

void TestDiffEngine::testLineLimitIsNotSilentlyTruncated() {
    DiffOptions opt;
    opt.maxLines = 10;
    QStringList a; for (int i = 0; i < 50; ++i) a << "x";
    DiffResult r = AssemblyDiffProcessor::computeDiff(a.join('\n'), "x", opt);
    QVERIFY(r.isTruncated);
    QVERIFY(!r.isComplete());
    QVERIFY(r.entries.isEmpty());   // no pretend-result
    QVERIFY(r.statusMessage.contains("limit"));
}

void TestDiffEngine::testCancellationInsideAlgorithm() {
    QStringList a, b;
    for (int i = 0; i < 20000; ++i) { a << QString("a%1").arg(i); b << QString("b%1").arg(i); }
    int polls = 0;
    auto cancel = [&] { return ++polls > 3; };   // false at first, true while the search is running
    DiffOptions opt;
    opt.maxDiffSteps = 2000000000;
    DiffResult r = AssemblyDiffProcessor::computeDiff(a.join('\n'), b.join('\n'), opt, cancel);
    QVERIFY(r.isCancelled);
    QVERIFY(!r.isComplete());
    QVERIFY(polls > 3);
}

void TestDiffEngine::testArm64ImmediateIsNotAComment() {
    DiffOptions o; o.ignoreComments = true; o.ignoreWhitespace = true; o.dialect = AsmDialect::ARM64;
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("\tadd x0, x1, #42", o), QString("add x0, x1, #42"));
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("\tmov w0, #0 // zero", o), QString("mov w0, #0"));
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("\tret ; apple style", o), QString("ret"));
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("# whole line comment", o), QString(""));
    // Different immediates must therefore produce a difference.
    auto r = AssemblyDiffProcessor::computeDiff("add x0, x1, #42\n", "add x0, x1, #43\n", o);
    QVERIFY(r.hasDifferences());
}

void TestDiffEngine::testX86CommentsAndStrings() {
    DiffOptions o; o.ignoreComments = true; o.dialect = AsmDialect::X86ATT;
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("\tmovl $1, %eax # note", o), QString("movl $1, %eax"));
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("\t.string \"a#b\"", o), QString(".string \"a#b\""));
}

void TestDiffEngine::testJvmAndPythonNotStrippedLikeX86() {
    DiffOptions o; o.ignoreComments = true; o.dialect = AsmDialect::JvmBytecode;
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("1: invokespecial #1 // Method java/lang/Object.\"<init>\":()V", o),
             QString("1: invokespecial #1"));   // '#1' constant pool ref kept
    o.dialect = AsmDialect::CPythonBytecode;
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("LOAD_CONST 1 (# not a comment)", o),
             QString("LOAD_CONST 1 (# not a comment)"));
}

void TestDiffEngine::testUnknownDialectIsConservative() {
    DiffOptions o; o.ignoreComments = true; o.normalizeLabels = true;  // dialect Unknown
    QCOMPARE(AssemblyDiffProcessor::normalizeAssemblyLine("mov x # y", o), QString("mov x # y"));
    auto r = AssemblyDiffProcessor::computeDiff(".L1:\n", ".L2:\n", o);
    QVERIFY(r.hasDifferences());   // no label renaming without known syntax
}

void TestDiffEngine::testLabelRenamingKeepsJumpTargetsDistinct() {
    DiffOptions o; o.normalizeLabels = true; o.dialect = AsmDialect::X86ATT;
    // Same structure, different label numbers: identical.
    QString a = "\tjne .L5\n\tnop\n.L5:\n\tret\n";
    QString b = "\tjne .L9\n\tnop\n.L9:\n\tret\n";
    QVERIFY(!AssemblyDiffProcessor::computeDiff(a, b, o).hasDifferences());
    // Different targets must NOT collapse (a blanket ".L_lbl" replacement would hide this).
    QString c = "\tjne .L5\n\tjmp .L6\n.L5:\n.L6:\n\tret\n";
    QString d = "\tjne .L6\n\tjmp .L5\n.L5:\n.L6:\n\tret\n";
    DiffOptions o2 = o;
    // c: jne->first label, jmp->second; d: jne->second, jmp->first => structurally different
    // (canonical order of first appearance: c: L5=0,L6=1 ; d: L6=0,L5=1 => jne .Lc0/jmp .Lc1 in both, definitions differ)
    auto r = AssemblyDiffProcessor::computeDiff(c, d, o2);
    QVERIFY(r.hasDifferences());
}

void TestDiffEngine::testUnifiedDiffRoundtrip() {
    QRandomGenerator rng(777);
    for (int round = 0; round < 300; ++round) {
        const int alphabet = 2 + rng.bounded(8);
        QString a = randomText(rng, 60, alphabet, rng.bounded(2));
        QString b = randomText(rng, 60, alphabet, rng.bounded(2));
        DiffResult r = AssemblyDiffProcessor::computeDiff(a, b);
        QString err;
        QString patch = AssemblyDiffProcessor::toUnifiedDiff(r, "a", "b", &err);
        QVERIFY2(err.isEmpty(), qPrintable(err));
        if (a == b) { QVERIFY(patch.contains("--- a")); continue; }
        QString applied, perr;
        QVERIFY2(applyPatch(a, patch, &applied, &perr),
                 qPrintable(QString("round %1: %2\nA=%3\nB=%4\n%5").arg(round).arg(perr, a, b, patch)));
        QCOMPARE(applied, b);
    }
}

void TestDiffEngine::testUnifiedDiffRoundtripWithSystemPatch() {
    QProcess probe;
    probe.start("patch", {"--version"});
    if (!probe.waitForFinished(3000) || probe.exitCode() != 0) QSKIP("system `patch` not available", SkipAll);
    QTemporaryDir dir;
    QString a = "main:\n\tmov eax, 0\n\tadd eax, 2\n\tret\n";
    QString b = "main:\n\tmov eax, 1\n\tret\n\tnop\n";
    DiffResult r = AssemblyDiffProcessor::computeDiff(a, b);
    QString patch = AssemblyDiffProcessor::toUnifiedDiff(r, "file", "file");
    QFile fa(dir.path() + "/file"); QVERIFY(fa.open(QIODevice::WriteOnly)); fa.write(a.toUtf8()); fa.close();
    QFile fp(dir.path() + "/p.diff"); QVERIFY(fp.open(QIODevice::WriteOnly)); fp.write(patch.toUtf8()); fp.close();
    QProcess p; p.setWorkingDirectory(dir.path());
    p.start("patch", {"file", "p.diff"});
    QVERIFY(p.waitForFinished(5000));
    QCOMPARE(p.exitCode(), 0);
    QFile fr(dir.path() + "/file"); QVERIFY(fr.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(fr.readAll()), b);
}

void TestDiffEngine::testIncompleteDiffExport() {
    DiffOptions opt; opt.maxDiffSteps = 10;
    QStringList a, b;
    for (int i = 0; i < 500; ++i) { a << QString("a%1").arg(i % 3); b << QString("b%1").arg(i % 4); }
    DiffResult r = AssemblyDiffProcessor::computeDiff(a.join('\n'), b.join('\n'), opt);
    QVERIFY(!r.isComplete());
    QString err;
    QVERIFY(AssemblyDiffProcessor::toUnifiedDiff(r, "a", "b", &err).isEmpty());
    QVERIFY(!err.isEmpty());
    QString marked = AssemblyDiffProcessor::toUnifiedDiff(r, "a", "b", &err, true);
    QVERIFY(marked.startsWith("# OCTOS INCOMPLETE DIFF"));
    QString out, perr;
    QVERIFY(!applyPatch(a.join('\n'), marked, &out, &perr));   // refuses to be applied
}

OCTOS_TEST_MAIN(TestDiffEngine)
#include "test_diff_engine.moc"

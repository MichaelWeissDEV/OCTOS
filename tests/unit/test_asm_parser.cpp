// Structural tests of the assembly parser: not "does it crash", but "is the graph right".
#include "../TestSupport.h"
#include <QTest>
#include <QFile>
#include <QRandomGenerator>
#include "parser/AssemblyParser.h"

namespace {
QString readData(const QString& name) {
    QFile f(QString(OCTOS_TEST_DATA_DIR) + "/asm/" + name);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

const Function* findFn(const AssemblyModule& m, const QString& name) {
    for (const Function& f : m.functions) if (f.name == name) return &f;
    return nullptr;
}

QString dump(const Function& fn) {
    QString s = QString("FUNC %1 blocks=%2 insts=%3\n").arg(fn.name).arg(fn.basicBlocks.size()).arg(fn.totalInstructions());
    for (const auto& b : fn.basicBlocks) {
        s += QString("  bb%1 %2 [%3] n=%4 ->").arg(b.id).arg(b.name, b.labels.join(","), QString::number(b.instructions.size()));
        for (int ei : b.successors) {
            const auto& e = fn.edges[ei];
            s += QString(" %1:%2(%3%4)").arg(edgeKindToString(e.kind)).arg(e.to).arg(e.targetName, e.leavesFunction ? " LEAVES" : "");
        }
        s += "\n";
    }
    return s;
}

int countKind(const Function& fn, InstructionKind k) {
    int n = 0;
    for (const auto& b : fn.basicBlocks) for (const auto& i : b.instructions) if (i.kind == k) ++n;
    return n;
}

int countEdges(const Function& fn, EdgeKind k) {
    int n = 0;
    for (const auto& e : fn.edges) if (e.kind == k) ++n;
    return n;
}

// Invariants every correct CFG must satisfy.
void checkInvariants(const Function& fn, QStringList* problems) {
    for (const auto& e : fn.edges) {
        if (e.from < 0 || e.from >= fn.basicBlocks.size()) problems->append(fn.name + ": edge from out of range");
        if (e.to >= fn.basicBlocks.size()) problems->append(fn.name + ": edge to out of range");
        if (e.kind == EdgeKind::IndirectUnknown && (e.to != -1 || !e.targetName.isEmpty()))
            problems->append(fn.name + ": indirect edge has a concrete target");
        if (e.kind == EdgeKind::Fallthrough && e.to != e.from + 1)
            problems->append(fn.name + ": fallthrough is not to the next block");
    }
    for (int b = 0; b < fn.basicBlocks.size(); ++b) {
        const auto& bb = fn.basicBlocks[b];
        if (bb.id != b) problems->append(fn.name + ": unstable block id");
        if (bb.instructions.isEmpty()) { problems->append(fn.name + ": empty block"); continue; }
        // A terminator may only appear as the last instruction of a block.
        for (int i = 0; i + 1 < bb.instructions.size(); ++i)
            if (bb.instructions[i].isTerminator()) problems->append(fn.name + ": terminator inside block");
        const InstructionKind lk = bb.instructions.last().kind;
        int succ = bb.successors.size();
        const bool hasNext = b + 1 < fn.basicBlocks.size();
        switch (lk) {
            case InstructionKind::Return:
            case InstructionKind::Trap:
                if (succ != 0) problems->append(fn.name + ": return/trap block has successors");
                break;
            case InstructionKind::ConditionalBranch:
                if (succ != (hasNext ? 2 : 1)) problems->append(fn.name + ": conditional branch must have taken+fallthrough");
                break;
            case InstructionKind::UnconditionalBranch:
            case InstructionKind::IndirectBranch:
                if (succ != 1) problems->append(fn.name + ": jump must have exactly one edge");
                break;
            default:
                if (hasNext && succ != 1) problems->append(fn.name + ": non-terminated block must fall through");
        }
        // successors/predecessors consistency
        for (int ei : bb.successors) if (fn.edges[ei].from != b) problems->append(fn.name + ": successor list inconsistent");
        for (int ei : bb.predecessors) if (fn.edges[ei].to != b) problems->append(fn.name + ": predecessor list inconsistent");
    }
}
}

class TestAsmParser : public QObject {
    Q_OBJECT
private slots:
    void testDumpCorpus();
    void testCorpusInvariants_data();
    void testCorpusInvariants();
    void testSimpleFunctionSingleBlock();
    void testIfElseDiamond();
    void testLoopHasBackEdge();
    void testSwitchIndirectJumpHasNoConcreteEdge();
    void testTailCallLeavesFunction();
    void testCallsDoNotSplitBlocks();
    void testFunctionBoundariesUseTypeAndSize();
    void testNonFunctionGlobalLabelDoesNotStartFunction();
    void testLocalLabelsAndInnerNonLocalLabel();
    void testDataLabelsInRodataAreNotCode();
    void testOperandClassificationATT();
    void testOperandClassificationIntel();
    void testArm64StyleImmediateNotRegister();
    void testSourceMapping();
    void testUnknownDirectivesAndGarbage();
    void testIntelSyntaxCorpus();
    void testMetricsFromModel();
    void testRobustnessRandomInput();
};

void TestAsmParser::testDumpCorpus() {
    if (qEnvironmentVariableIsEmpty("OCTOS_DUMP")) QSKIP("set OCTOS_DUMP=1 to print the parsed corpus", SkipAll);
    for (const QString& f : {"clang_x86_64_att_O0.s", "clang_x86_64_att_O3.s"}) {
        auto m = AssemblyParser::parseX86_64(readData(f));
        qInfo().noquote() << "=====" << f << "unparsed" << m.unparsedLines.size() << m.warnings;
        for (const auto& fn : m.functions) qInfo().noquote() << dump(fn);
    }
}

void TestAsmParser::testCorpusInvariants_data() {
    QTest::addColumn<QString>("file");
    QTest::newRow("clang att O0") << "clang_x86_64_att_O0.s";
    QTest::newRow("clang att O3") << "clang_x86_64_att_O3.s";
    QTest::newRow("clang intel O3") << "clang_x86_64_intel_O3.s";
}

void TestAsmParser::testCorpusInvariants() {
    QFETCH(QString, file);
    const QString text = readData(file);
    QVERIFY2(!text.isEmpty(), "corpus file missing");
    auto m = AssemblyParser::parseX86_64(text);
    QVERIFY(!m.truncated);
    // exactly the functions the source defines (static local_fn is inlined at -O3)
    for (const char* name : {"simple_add", "if_else", "loop_sum", "sw", "sw_call", "tail_call", "call_twice", "indirect", "uses_local"})
        QVERIFY2(findFn(m, name), name);
    QStringList problems;
    for (const auto& fn : m.functions) checkInvariants(fn, &problems);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join("\n")));
    for (const auto& fn : m.functions) QVERIFY2(fn.declaredAsFunction, qPrintable(fn.name));
}

void TestAsmParser::testSimpleFunctionSingleBlock() {
    for (const char* file : {"clang_x86_64_att_O0.s", "clang_x86_64_att_O3.s"}) {
        auto m = AssemblyParser::parseX86_64(readData(file));
        const Function* f = findFn(m, "simple_add");
        QVERIFY(f);
        QCOMPARE(f->basicBlocks.size(), 1);
        QCOMPARE(f->edges.size(), 0);
        QCOMPARE(countKind(*f, InstructionKind::Return), 1);
        QVERIFY(f->isGlobal);
    }
}

void TestAsmParser::testIfElseDiamond() {
    auto m = AssemblyParser::parseX86_64(readData("clang_x86_64_att_O0.s"));
    const Function* f = findFn(m, "if_else");
    QVERIFY(f);
    QVERIFY2(f->basicBlocks.size() >= 5, qPrintable(dump(*f)));
    QVERIFY(countKind(*f, InstructionKind::ConditionalBranch) >= 2);
    // every conditional branch contributes one Taken and one Fallthrough edge
    QCOMPARE(countEdges(*f, EdgeKind::Taken), countKind(*f, InstructionKind::ConditionalBranch));
    // each Taken edge goes to a resolved block of this function
    for (const auto& e : f->edges) if (e.kind == EdgeKind::Taken) QVERIFY(e.to >= 0 && !e.leavesFunction);
    // exit block has a return and several predecessors (merge point)
    int retBlock = -1;
    for (const auto& b : f->basicBlocks) if (b.instructions.last().kind == InstructionKind::Return) retBlock = b.id;
    QVERIFY(retBlock >= 0);
    QVERIFY(f->basicBlocks[retBlock].predecessors.size() >= 2);
}

void TestAsmParser::testLoopHasBackEdge() {
    auto m = AssemblyParser::parseX86_64(readData("clang_x86_64_att_O0.s"));
    const Function* f = findFn(m, "loop_sum");
    QVERIFY(f);
    bool back = false;
    for (const auto& e : f->edges) if (e.to >= 0 && e.to <= e.from && e.kind != EdgeKind::Fallthrough) back = true;
    QVERIFY2(back, qPrintable(dump(*f)));
    // O3 also has a loop
    auto m3 = AssemblyParser::parseX86_64(readData("clang_x86_64_att_O3.s"));
    const Function* f3 = findFn(m3, "loop_sum");
    QVERIFY(f3);
    bool back3 = false;
    for (const auto& e : f3->edges) if (e.to >= 0 && e.to <= e.from && e.kind != EdgeKind::Fallthrough) back3 = true;
    QVERIFY2(back3, qPrintable(dump(*f3)));
}

void TestAsmParser::testSwitchIndirectJumpHasNoConcreteEdge() {
    for (const char* file : {"clang_x86_64_att_O0.s", "clang_x86_64_att_O3.s"}) {
        auto m = AssemblyParser::parseX86_64(readData(file));
        const Function* f = findFn(m, "sw_call");
        QVERIFY(f);
        QVERIFY2(countKind(*f, InstructionKind::IndirectBranch) >= 1, qPrintable(dump(*f)));
        QCOMPARE(countEdges(*f, EdgeKind::IndirectUnknown), countKind(*f, InstructionKind::IndirectBranch));
        for (const auto& e : f->edges) if (e.kind == EdgeKind::IndirectUnknown) { QCOMPARE(e.to, -1); QVERIFY(e.targetName.isEmpty()); }
    }
}

void TestAsmParser::testTailCallLeavesFunction() {
    auto m = AssemblyParser::parseX86_64(readData("clang_x86_64_att_O3.s"));
    const Function* f = findFn(m, "tail_call");
    QVERIFY(f);
    bool found = false;
    for (const auto& e : f->edges)
        if (e.kind == EdgeKind::Unconditional && e.leavesFunction && e.to == -1 && e.targetName.contains("helper")) found = true;
    QVERIFY2(found, qPrintable(dump(*f)));
    QCOMPARE(countKind(*f, InstructionKind::Return), 0);   // ends in a jump, not a ret
}

void TestAsmParser::testCallsDoNotSplitBlocks() {
    auto m = AssemblyParser::parseX86_64(readData("clang_x86_64_att_O3.s"));
    const Function* f = findFn(m, "call_twice");
    QVERIFY(f);
    QVERIFY(countKind(*f, InstructionKind::Call) >= 2);
    QCOMPARE(f->basicBlocks.size(), 1);      // two calls + ret: one straight-line block
    QCOMPARE(f->edges.size(), 0);
}

void TestAsmParser::testFunctionBoundariesUseTypeAndSize() {
    const QString text =
        "\t.text\n\t.globl\tfoo\n\t.type\tfoo,@function\nfoo:\n\tmovl\t$1, %eax\n\tret\n\t.size\tfoo, 6\n"
        "\t.p2align 4\n"
        "\t.type\tbar,@function\nbar:\n\tret\n\t.size\tbar, .-bar\n";
    auto m = AssemblyParser::parseX86_64(text);
    QCOMPARE(m.functions.size(), 2);
    QCOMPARE(m.functions[0].name, QString("foo"));
    QVERIFY(m.functions[0].isGlobal);
    QCOMPARE(m.functions[0].sizeBytes, qint64(6));
    QVERIFY(!m.functions[1].isGlobal);
    QCOMPARE(m.functions[1].sizeBytes, qint64(-1));    // `.-bar` cannot be resolved from text
    auto metrics = computeAssemblyMetrics(m);
    QVERIFY(!metrics.codeSizeKnown);                    // no fabricated total
}

void TestAsmParser::testNonFunctionGlobalLabelDoesNotStartFunction() {
    // `data_marker` is a plain global label inside foo: it must not split the function.
    const QString text =
        "\t.type\tfoo,@function\nfoo:\n\tcmpl\t$0, %edi\n\tje\tdata_marker\n\tmovl\t$1, %eax\ndata_marker:\n\tret\n\t.size\tfoo, 3\n";
    auto m = AssemblyParser::parseX86_64(text);
    QCOMPARE(m.functions.size(), 1);
    QCOMPARE(m.functions[0].basicBlocks.size(), 3);
    QCOMPARE(m.functions[0].basicBlocks[2].name, QString("data_marker"));
}

void TestAsmParser::testLocalLabelsAndInnerNonLocalLabel() {
    // unreferenced local labels are aliases, not block boundaries
    const QString text =
        "\t.type\tf,@function\nf:\n.Lfunc_begin0:\n\tmovl\t$1, %eax\n.Ltmp0:\n\taddl\t$2, %eax\n\tret\n";
    auto m = AssemblyParser::parseX86_64(text);
    QCOMPARE(m.functions.size(), 1);
    QCOMPARE(m.functions[0].basicBlocks.size(), 1);
    QVERIFY(m.functions[0].basicBlocks[0].labels.contains(".Ltmp0"));
    // numeric local labels with 1f/1b references
    const QString num =
        "g:\n\tdecl\t%edi\n1:\n\tdecl\t%edi\n\tjne\t1b\n\tjmp\t2f\n\tnop\n2:\n\tret\n";
    auto mn = AssemblyParser::parseX86_64(num);
    QCOMPARE(mn.functions.size(), 1);
    QStringList problems; checkInvariants(mn.functions[0], &problems);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join("\n") + dump(mn.functions[0])));
    bool back = false;
    for (const auto& e : mn.functions[0].edges) if (e.kind == EdgeKind::Taken && e.to >= 0 && e.to <= e.from) back = true;
    QVERIFY2(back, qPrintable(dump(mn.functions[0])));
}

void TestAsmParser::testDataLabelsInRodataAreNotCode() {
    const QString text =
        "\t.text\n\t.type\tf,@function\nf:\n\tret\n\t.size\tf, 1\n"
        "\t.section\t.rodata\n.LJTI0_0:\n\t.quad\t.LBB0_1\n.LC0:\n\t.string \"a # b\"\n"
        "\t.text\n\t.type\tg,@function\ng:\n\tret\n";
    auto m = AssemblyParser::parseX86_64(text);
    QCOMPARE(m.functions.size(), 2);
    QCOMPARE(m.functions[0].basicBlocks.size(), 1);
    int dataLabels = 0;
    for (const auto& l : m.labels) if (!l.inCodeSection) ++dataLabels;
    QCOMPARE(dataLabels, 2);
}

void TestAsmParser::testOperandClassificationATT() {
    auto op = [](const char* t, bool branch = false) { return AssemblyParser::parseOperand(t, AsmSyntax::ATT, branch); };
    QCOMPARE(op("%rax").kind, OperandKind::Register);
    QCOMPARE(op("%r10d").registerName, QString("r10d"));
    QCOMPARE(op("%xmm1").kind, OperandKind::Register);
    QCOMPARE(op("$42").kind, OperandKind::Immediate);
    QCOMPARE(op("$-1").kind, OperandKind::Immediate);
    QCOMPARE(op("$.LC0").symbol, QString(".LC0"));
    QCOMPARE(op("-4(%rbp)").kind, OperandKind::Memory);
    QCOMPARE(op("(%rax,%rcx,4)").kind, OperandKind::Memory);
    QCOMPARE(op(".LC0(%rip)").symbol, QString(".LC0"));
    QCOMPARE(op("foo@GOTPCREL(%rip)").relocation, QString("GOTPCREL"));
    QCOMPARE(op("%fs:40").kind, OperandKind::Memory);
    QCOMPARE(op("some_global").kind, OperandKind::Memory);           // AT&T: bare symbol addresses memory
    QCOMPARE(op(".L3", true).kind, OperandKind::Symbol);
    QCOMPARE(op("helper@PLT", true).kind, OperandKind::Relocation);
    QCOMPARE(op("helper@PLT", true).symbol, QString("helper"));
    QVERIFY(op("*%rax", true).indirect);
    QCOMPARE(op("*%rax", true).kind, OperandKind::Register);
    QCOMPARE(op("*.LJTI0_0(,%rax,8)", true).kind, OperandKind::Memory);
    // things that merely START like a register name must not be registers
    QCOMPARE(op("result_value").kind, OperandKind::Memory);
    QCOMPARE(op("rax_counter", true).kind, OperandKind::Symbol);
    // unknown stays unknown, with the raw text preserved
    Operand u = op("@@weird{}");
    QCOMPARE(u.kind, OperandKind::Unknown);
    QCOMPARE(u.rawText, QString("@@weird{}"));
    QVERIFY(!u.isRegister && !u.isMemory && !u.isImmediate && !u.isSymbol);
}

void TestAsmParser::testOperandClassificationIntel() {
    auto op = [](const char* t, bool branch = false) { return AssemblyParser::parseOperand(t, AsmSyntax::Intel, branch); };
    QCOMPARE(op("eax").kind, OperandKind::Register);
    QCOMPARE(op("r10d").kind, OperandKind::Register);
    QCOMPARE(op("DWORD PTR [rbp-4]").kind, OperandKind::Memory);
    QCOMPARE(op("[rip+.LC0]").kind, OperandKind::Memory);
    QCOMPARE(op("42").kind, OperandKind::Immediate);
    QCOMPARE(op("0x2a").kind, OperandKind::Immediate);
    QCOMPARE(op("OFFSET .LC0").symbol, QString(".LC0"));
    QCOMPARE(op(".L5", true).kind, OperandKind::Symbol);
    QCOMPARE(op("rax_counter").kind, OperandKind::Unknown);          // not guessed
}

void TestAsmParser::testArm64StyleImmediateNotRegister() {
    // The old heuristic classified anything starting with r/e/x/w as a register.
    auto a = AssemblyParser::parseOperand("result", AsmSyntax::ATT, false);
    QVERIFY(!a.isRegister);
    auto b = AssemblyParser::parseOperand("#42", AsmSyntax::ATT, false);
    QVERIFY(!b.isRegister);               // '#' is not an x86 immediate marker
    QCOMPARE(b.kind, OperandKind::Unknown);
}

void TestAsmParser::testSourceMapping() {
    const QString text =
        "\t.file\t\"x.c\"\n\t.file\t1 \"/src\" \"x.c\"\n\t.file\t2 \"/inc/y.h\"\n"
        "\t.type\tf,@function\nf:\n"
        "\t.loc\t1 10 5 prologue_end\n\tmovl\t$1, %eax\n"
        "\t.loc\t2 3 0\n\tnop\n"
        "\t.loc\t7 99 1\n\tnop\n"        // file id 7 was never declared
        "\t.loc\tx y z\n\tret\n";        // garbage
    auto m = AssemblyParser::parseX86_64(text);
    QCOMPARE(m.functions.size(), 1);
    const auto& ins = m.functions[0].basicBlocks[0].instructions;
    QCOMPARE(ins.size(), 4);
    QCOMPARE(ins[0].sourceLoc.line, 10);
    QCOMPARE(ins[0].sourceLoc.column, 5);
    QCOMPARE(ins[0].sourceLoc.fileName, QString("/src/x.c"));
    QVERIFY(ins[0].sourceLoc.fileKnown);
    QCOMPARE(ins[1].sourceLoc.fileName, QString("/inc/y.h"));
    // undeclared file id: the line is kept, the file is explicitly unknown
    QCOMPARE(ins[2].sourceLoc.line, 99);
    QVERIFY(!ins[2].sourceLoc.fileKnown);
    QVERIFY(ins[2].sourceLoc.fileName.isEmpty());
    // unparsable .loc: mapping becomes unknown instead of reusing a stale one
    QVERIFY(!ins[3].sourceLoc.isValid());
    QVERIFY(!m.warnings.isEmpty());   // the garbage .loc was reported
    // original assembly line numbers are preserved
    QCOMPARE(ins[0].originalLineNumber, 7);
}

void TestAsmParser::testUnknownDirectivesAndGarbage() {
    const QString text =
        "\t.some_future_directive 1,2,3\n\t.type\tf,@function\nf:\n\t.cfi_startproc\n\tmovl\t$1, %eax\n"
        "\t%%%garbage%%%\n\t{}\n\tret\n\t.cfi_endproc\n";
    auto m = AssemblyParser::parseX86_64(text);
    QCOMPARE(m.functions.size(), 1);
    QCOMPARE(m.functions[0].totalInstructions(), 2);
    QVERIFY(m.unparsedLines.size() >= 2);
    QVERIFY(m.globalDirectives.join('\n').contains(".some_future_directive"));
}

void TestAsmParser::testIntelSyntaxCorpus() {
    auto m = AssemblyParser::parseX86_64(readData("clang_x86_64_intel_O3.s"));
    QCOMPARE(m.syntax, AsmSyntax::Intel);
    QStringList problems;
    for (const auto& fn : m.functions) checkInvariants(fn, &problems);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join("\n")));
    const Function* f = findFn(m, "sw_call");
    QVERIFY(f);
    QVERIFY(countKind(*f, InstructionKind::IndirectBranch) >= 1);
    const Function* t = findFn(m, "tail_call");
    QVERIFY(t);
    QCOMPARE(countKind(*t, InstructionKind::Return), 0);
}

void TestAsmParser::testMetricsFromModel() {
    auto m = AssemblyParser::parseX86_64(readData("clang_x86_64_att_O3.s"));
    auto x = computeAssemblyMetrics(m);
    QCOMPARE(x.functionCount, m.functions.size());
    int blocks = 0, insts = 0, ret = 0, cond = 0;
    for (const auto& fn : m.functions) {
        blocks += fn.basicBlocks.size(); insts += fn.totalInstructions();
        ret += countKind(fn, InstructionKind::Return); cond += countKind(fn, InstructionKind::ConditionalBranch);
    }
    QCOMPARE(x.basicBlockCount, blocks);
    QCOMPARE(x.instructionCount, insts);
    QCOMPARE(x.returns, ret);
    QCOMPARE(x.conditionalJumps, cond);
    QVERIFY(x.tailCalls >= 1);
    QVERIFY(x.instructionFrequencies.value("retq") + x.instructionFrequencies.value("ret") >= 1);
    QVERIFY(!x.codeSizeKnown);
}

void TestAsmParser::testRobustnessRandomInput() {
    QRandomGenerator rng(4242);
    static const QStringList atoms = {"\t", " ", "\n", ":", ",", "(", ")", "[", "]", "%rax", "$", "*", ".L1", "jmp", "jne", "call",
                                      "ret", ".type f,@function", ".loc 1 2", "\"", "#", ";", "@PLT", "0x", "\xff\xfe", "f:", ".size", ".file 1 \"a\""};
    for (int round = 0; round < 300; ++round) {
        QString s;
        const int n = rng.bounded(200);
        for (int i = 0; i < n; ++i) s += atoms[rng.bounded(atoms.size())];
        auto m = AssemblyParser::parseX86_64(s);
        QStringList problems;
        for (const auto& fn : m.functions) checkInvariants(fn, &problems);
        QVERIFY2(problems.isEmpty(), qPrintable(s + "\n" + problems.join("\n")));
    }
    // extreme line, many labels, invalid UTF-16 surrogate
    QString longLine = QString(1000000, 'a');
    auto m1 = AssemblyParser::parseX86_64(longLine + "\nret\n");
    QVERIFY(m1.truncated);
    QString many;
    for (int i = 0; i < 20000; ++i) many += "f:\n.L1:\n.L1:\n\tjmp .L1\n";
    auto m2 = AssemblyParser::parseX86_64(many);
    QVERIFY(m2.functions.size() > 0);
    QString bad = QString::fromUtf8("\xff\xfe\xfd:\n\tret\n");
    auto m3 = AssemblyParser::parseX86_64(bad);
    Q_UNUSED(m3);
}

OCTOS_TEST_MAIN(TestAsmParser)
#include "test_asm_parser.moc"

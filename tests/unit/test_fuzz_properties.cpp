// The fuzz-target bodies run as an ordinary unit test on structured pseudo-random and mutated inputs
// (real libFuzzer targets: tests/fuzz, option OCTOS_BUILD_FUZZERS). A violated property aborts.
#include "../TestSupport.h"
#include <QTest>
#include <QFile>
#include <QRandomGenerator>
#include "../fuzz/fuzz_bodies.h"

namespace {
QByteArray corpus(const char* name) {
    QFile f(QString(OCTOS_TEST_DATA_DIR) + "/asm/" + name);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

QByteArray mutate(QRandomGenerator& rng, QByteArray d) {
    const int edits = 1 + rng.bounded(8);
    for (int e = 0; e < edits && !d.isEmpty(); ++e) {
        const int pos = rng.bounded(d.size());
        switch (rng.bounded(6)) {
            case 0: d[pos] = char(rng.bounded(256)); break;                     // flip a byte (also invalid UTF-8)
            case 1: d.remove(pos, 1 + rng.bounded(40)); break;
            case 2: d.insert(pos, QByteArray(1 + rng.bounded(30), char(rng.bounded(256)))); break;
            case 3: d.truncate(pos); break;
            case 4: d.insert(pos, d.mid(rng.bounded(d.size()), rng.bounded(200))); break;   // repeat a chunk
            default: d.insert(pos, "\xed\xa0\x80\xff\xfe"); break;               // surrogate halves / invalid bytes
        }
    }
    return d;
}

template <typename F>
void run(F body, const QVector<QByteArray>& seeds, int rounds, quint32 seed) {
    QRandomGenerator rng(seed);
    for (const QByteArray& s : seeds) body(reinterpret_cast<const uint8_t*>(s.constData()), size_t(s.size()));
    for (int i = 0; i < rounds; ++i) {
        const QByteArray base = seeds[rng.bounded(seeds.size())];
        const QByteArray m = mutate(rng, base);
        body(reinterpret_cast<const uint8_t*>(m.constData()), size_t(m.size()));
    }
}
}

class TestFuzzProperties : public QObject {
    Q_OBJECT
private slots:
    void testAsmParserProperties();
    void testOperandParserProperties();
    void testDiffEngineProperties();
    void testWorkspaceJsonProperties();
    void testCompilerArgumentProperties();
    void testPathologicalInputs();
};

void TestFuzzProperties::testAsmParserProperties() {
    QVector<QByteArray> seeds = {corpus("clang_x86_64_att_O0.s").left(40000), corpus("clang_x86_64_att_O3.s"),
                                 corpus("clang_x86_64_intel_O3.s"), "", "f:\n\tret\n", "\t.type f,@function\nf:\n1:\n\tjne 1b\n\tjmp *%rax\n"};
    run(fuzz::asmParser, seeds, 600, 1);
}

void TestFuzzProperties::testOperandParserProperties() {
    QVector<QByteArray> seeds = {"%rax", "$42", "-4(%rbp)", "foo@PLT", "*%rax", "DWORD PTR [rbp-4]", "OFFSET .LC0", "%fs:40", "", "((((", "$$$", "\xff\xfe"};
    run(fuzz::operandParser, seeds, 4000, 2);
}

void TestFuzzProperties::testDiffEngineProperties() {
    auto mk = [](quint8 cfg, const QByteArray& a, const QByteArray& b) { return QByteArray(1, char(cfg)) + a + QByteArray(1, '\0') + b; };
    QByteArray big, big2;
    for (int i = 0; i < 300; ++i) { big += "line" + QByteArray::number(i % 17) + "\n"; big2 += "line" + QByteArray::number((i * 7) % 19) + "\n"; }
    QVector<QByteArray> seeds = {mk(0, "a\nb\nc\n", "a\nx\nc\n"), mk(0x80, big, big2), mk(0x40, big, big2), mk(7, "mov x # c\n.L1:\n", "mov x\n.L9:\n"),
                                 mk(0x29, "add x0, x1, #5 // c\n", "add x0, x1, #6\n"), mk(0, "", ""), mk(0, "x", "x\n"), mk(0xff, big, "")};
    run(fuzz::diffEngine, seeds, 1500, 3);
}

void TestFuzzProperties::testWorkspaceJsonProperties() {
    Workspace ws("seed");
    SourceFile f; f.path = "a/b.cpp"; f.content = "x"; ws.addSourceFile(f);
    CompilerProfile p; p.id = "p"; p.name = "P"; ws.addProfile(p);
    BuildTarget t; t.name = "t"; t.mainSourceFile = "a/b.cpp"; t.sourceFiles = {"a/b.cpp"}; ws.addTarget(t);
    QVector<QByteArray> seeds = {ws.toJson(), "{}", "[]", "null", "{\"format_version\":\"octos-project-v1\",\"source_files\":[{\"path\":\"../x\"}]}"};
    run(fuzz::workspaceJson, seeds, 4000, 4);
}

void TestFuzzProperties::testCompilerArgumentProperties() {
    QVector<QByteArray> seeds = {"-O3 -Wall", "-O -C opt-level=3 --emit=obj --crate-type=bin --crate-type lib --edition 2018 -g", "-o /tmp/x; rm -rf / `id` $(id)",
                                 "\"unterminated", "'a b' \"c d\" -I/x", "-C", "--emit", "--crate-type", "-masm=intel -S -c", ""};
    run(fuzz::compilerArgs, seeds, 3000, 5);
}

void TestFuzzProperties::testPathologicalInputs() {
    // very long lines, deep nesting, huge repetition: bounded work and no crash
    auto feed = [](void (*body)(const uint8_t*, size_t), const QByteArray& d) { body(reinterpret_cast<const uint8_t*>(d.constData()), size_t(d.size())); };
    QByteArray longLine(500000, 'a');
    feed(fuzz::asmParser, longLine);
    feed(fuzz::operandParser, QByteArray(3000, '('));
    feed(fuzz::operandParser, QByteArray(3000, ')'));
    QByteArray labels;
    for (int i = 0; i < 4000; ++i) labels += ".L1:\n.L1:\n\tjmp .L1\n";
    feed(fuzz::asmParser, labels);
    QByteArray nested = QByteArray(20000, '[') + QByteArray(20000, ']');
    feed(fuzz::workspaceJson, nested);                  // deep JSON nesting must be rejected, not overflow the stack
    feed(fuzz::compilerArgs, QByteArray(8000, '"'));
    QByteArray manyLines;
    for (int i = 0; i < 15000; ++i) manyLines += "same\n";
    feed(fuzz::diffEngine, QByteArray(1, '\x80') + manyLines + QByteArray(1, '\0') + manyLines);
}

OCTOS_TEST_MAIN(TestFuzzProperties)
#include "test_fuzz_properties.moc"

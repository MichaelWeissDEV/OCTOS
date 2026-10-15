// Shared bodies of the fuzz targets. They are compiled both into libFuzzer executables
// (tests/fuzz/*.cpp, option OCTOS_BUILD_FUZZERS) and into the regular `test_fuzz_properties`
// unit test, which feeds them pseudo-random and mutated inputs. A violated property aborts.
#ifndef OCTOS_FUZZ_BODIES_H
#define OCTOS_FUZZ_BODIES_H

#include <QByteArray>
#include <QString>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "parser/AssemblyParser.h"
#include "utils/AssemblyDiffProcessor.h"
#include "core/WorkspaceModel.h"
#include "strategies/StrategyUtils.h"
#include "strategies/LanguageStrategyRegistry.h"

#define FUZZ_CHECK(cond)                                                                     \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            std::fprintf(stderr, "FUZZ PROPERTY VIOLATED: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
            std::abort();                                                                    \
        }                                                                                    \
    } while (0)

namespace fuzz {

inline QString text(const uint8_t* data, size_t size) {
    return QString::fromUtf8(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
}

// Invariants of any model: blocks, edges and operands are consistent.
inline void checkModel(const AssemblyModule& m) {
    for (const Function& fn : m.functions) {
        for (int b = 0; b < fn.basicBlocks.size(); ++b) {
            const BasicBlock& bb = fn.basicBlocks[b];
            FUZZ_CHECK(bb.id == b);
            FUZZ_CHECK(!bb.instructions.isEmpty());
            for (int i = 0; i + 1 < bb.instructions.size(); ++i) FUZZ_CHECK(!bb.instructions[i].isTerminator());
            for (const Instruction& in : bb.instructions) {
                FUZZ_CHECK(in.originalLineNumber >= 1);
                for (const Operand& op : in.operands) {
                    FUZZ_CHECK(op.isRegister == (op.kind == OperandKind::Register));
                    FUZZ_CHECK(op.isMemory == (op.kind == OperandKind::Memory));
                    FUZZ_CHECK(op.isImmediate == (op.kind == OperandKind::Immediate));
                }
            }
        }
        for (const BlockEdge& e : fn.edges) {
            FUZZ_CHECK(e.from >= 0 && e.from < fn.basicBlocks.size());
            FUZZ_CHECK(e.to < fn.basicBlocks.size());
            if (e.kind == EdgeKind::IndirectUnknown) FUZZ_CHECK(e.to == -1 && e.targetName.isEmpty());
            if (e.kind == EdgeKind::Fallthrough) FUZZ_CHECK(e.to == e.from + 1);
        }
    }
    const AssemblyMetrics mt = computeAssemblyMetrics(m);
    FUZZ_CHECK(mt.functionCount == m.functions.size());
    FUZZ_CHECK(mt.instructionCount == m.totalInstructions());
}

// Any input (x86 in both syntaxes, AArch64, auto-detected) produces a consistent model.
inline void asmParser(const uint8_t* data, size_t size) {
    if (size > (1u << 20)) return;
    const QString t = text(data, size);
    ParseLimits limits;
    limits.maxLines = 5000;       // bounded work per input
    limits.maxLineLength = 4096;
    for (AsmSyntax syn : {AsmSyntax::Auto, AsmSyntax::ATT, AsmSyntax::Intel})
        checkModel(AssemblyParser::parseX86_64(t, syn, limits));
    checkModel(AssemblyParser::parseARM64(t, limits));
    checkModel(AssemblyParser::parse(t, TargetArchitecture::Unknown, limits));
}

inline void operandParser(const uint8_t* data, size_t size) {
    if (size > 4096) return;
    const QString t = text(data, size);
    for (bool branch : {false, true}) {
        const Operand a = AssemblyParser::parseOperandARM64(t, branch);
        FUZZ_CHECK(a.rawText == t.trimmed());
        if (a.kind == OperandKind::Unknown) FUZZ_CHECK(!a.isRegister && !a.isMemory && !a.isImmediate && !a.isSymbol && !a.isRelocation);
    }
    for (AsmSyntax syn : {AsmSyntax::ATT, AsmSyntax::Intel})
        for (bool branch : {false, true}) {
            const Operand op = AssemblyParser::parseOperand(t, syn, branch);
            FUZZ_CHECK(op.rawText == t.trimmed());                      // original text is always preserved
            const int kinds = int(op.isRegister) + int(op.isMemory) + int(op.isImmediate) + int(op.isSymbol) + int(op.isRelocation);
            FUZZ_CHECK(kinds <= 1);
            if (op.kind == OperandKind::Unknown) FUZZ_CHECK(kinds == 0);  // unknown carries no invented properties
        }
    AssemblyParser::splitOperands(t);
}

// Both texts are split at the first NUL byte. A complete diff must reproduce both inputs exactly.
inline void diffEngine(const uint8_t* data, size_t size) {
    if (size < 2 || size > (1u << 18)) return;
    const uint8_t cfg = data[0];
    const QByteArray rest(reinterpret_cast<const char*>(data + 1), static_cast<qsizetype>(size - 1));
    const int cut = rest.indexOf('\0');
    const QString a = QString::fromUtf8(cut < 0 ? rest : rest.left(cut));
    const QString b = cut < 0 ? QString() : QString::fromUtf8(rest.mid(cut + 1));

    DiffOptions opt;
    opt.ignoreWhitespace = cfg & 1;
    opt.ignoreComments = cfg & 2;
    opt.normalizeLabels = cfg & 4;
    opt.dialect = static_cast<AsmDialect>((cfg >> 3) % 6);
    opt.maxDiffSteps = (cfg & 0x80) ? 200 : 2000000;                    // sometimes force the approximate path
    opt.maxLines = 20000;

    int polls = 0;
    const int cancelAfter = (cfg & 0x40) ? 3 : -1;                       // abrupt cancellation
    auto cancel = [&]() { return cancelAfter >= 0 && ++polls > cancelAfter; };
    const DiffResult r = AssemblyDiffProcessor::computeDiff(a, b, opt, cancel);

    if (r.isCancelled) { FUZZ_CHECK(!r.isComplete()); return; }
    if (r.isTruncated && r.entries.isEmpty()) return;                    // refused (limits): nothing to reproduce
    QStringList recA, recB;
    for (const DiffEntry& e : r.entries) {
        if (e.type != DiffLineType::Added) recA << e.textA;
        if (e.type != DiffLineType::Removed) recB << e.textB;
    }
    auto lines = [](const QString& s) { QStringList l = s.split('\n'); if (!l.isEmpty() && l.last().isEmpty()) l.removeLast(); return l; };
    FUZZ_CHECK(recA == lines(a));
    FUZZ_CHECK(recB == lines(b));
    QString err;
    const QString patch = AssemblyDiffProcessor::toUnifiedDiff(r, "a", "b", &err);
    if (!r.isComplete()) FUZZ_CHECK(patch.isEmpty() && !err.isEmpty());  // never a misleading complete patch
}

inline void workspaceJson(const uint8_t* data, size_t size) {
    if (size > (1u << 20)) return;
    QString err;
    const Workspace ws = Workspace::fromJson(QByteArray(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size)), &err);
    if (!err.isEmpty()) return;
    FUZZ_CHECK(ws.validate().isEmpty());                                  // accepted projects are always valid
    QString err2;
    const Workspace again = Workspace::fromJson(ws.toJson(), &err2);
    FUZZ_CHECK(err2.isEmpty());
    FUZZ_CHECK(again.sourceFiles().size() == ws.sourceFiles().size());
    FUZZ_CHECK(again.profiles().size() == ws.profiles().size());
    for (const SourceFile& f : ws.sourceFiles()) FUZZ_CHECK(Workspace::isSafeRelativePath(f.path));
}

// User flags go through the argument splitter and every strategy: nothing may crash, the output file
// must stay under our control and Rust must receive at most one value per setting.
inline void compilerArgs(const uint8_t* data, size_t size) {
    if (size > 8192) return;
    const QString flags = text(data, size);
    const QStringList split = StrategyUtils::splitCompilerArguments(flags);
    const QStringList clean = StrategyUtils::sanitizeUserFlags(split);
    for (const QString& f : clean) {
        for (QChar c : QString(";|&$`><(){}\n\r")) FUZZ_CHECK(!f.contains(c));   // no shell metacharacters survive
    }
    auto& reg = LanguageStrategyRegistry::instance();
    for (Language lang : {Language::Cpp, Language::C, Language::Rust, Language::Java, Language::Python, Language::CSharp, Language::Ada}) {
        const ILanguageStrategy* s = reg.getStrategy(lang, "image:latest");
        if (!s) continue;
        CompilationRequest req;
        req.language = lang;
        req.userArgs = flags;
        req.sourceCode = "fn main() {}";
        const QStringList args = s->buildCompilerArgs(req, "/input/source", "/output");
        FUZZ_CHECK(!args.isEmpty());
        if (lang == Language::Rust) {
            int nEmit = 0, nCrate = 0, nEdition = 0;
            for (const QString& a : args) {
                // exact flag names only: an unknown flag that merely starts like one is the user's problem
                if (a == "--emit" || a.startsWith("--emit=")) ++nEmit;
                if (a == "--crate-type" || a.startsWith("--crate-type=")) ++nCrate;
                if (a == "--edition" || a.startsWith("--edition=")) ++nEdition;
            }
            if (nEmit != 1 || nCrate != 1 || nEdition != 1)
                std::fprintf(stderr, "flags: %s\nargs: %s\n", qPrintable(flags.toUtf8().toPercentEncoding()), qPrintable(args.join(" | ")));
            FUZZ_CHECK(nEmit == 1);
            FUZZ_CHECK(nCrate == 1);
            FUZZ_CHECK(nEdition == 1);
            FUZZ_CHECK(args.contains("--emit=asm"));
        }
    }
}

}  // namespace fuzz

#endif  // OCTOS_FUZZ_BODIES_H

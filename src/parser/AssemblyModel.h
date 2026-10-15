#ifndef ASSEMBLYMODEL_H
#define ASSEMBLYMODEL_H

#include <QHash>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

enum class TargetArchitecture {
    X86_64,
    ARM64,
    Unknown
};

inline QString architectureToString(TargetArchitecture arch) {
    switch (arch) {
        case TargetArchitecture::X86_64: return "x86-64";
        case TargetArchitecture::ARM64:  return "ARM64";
        case TargetArchitecture::Unknown: return "Unknown";
    }
    return "Unknown";
}

// x86 assembly syntax of the parsed text.
enum class AsmSyntax { Auto, ATT, Intel };

struct SourceLocation {
    QString fileName;     // resolved through the .file table; empty if the file id is unknown
    int fileId{-1};       // id as written in .loc (-1: none)
    int line{0};
    int column{0};
    bool fileKnown{false};  // false => fileName is unknown (never a guessed name)

    bool isValid() const { return line > 0; }
};

enum class OperandKind {
    Register,
    Immediate,
    Memory,
    Symbol,      // direct symbol/label reference (branch target)
    Relocation,  // symbol with relocation operator, e.g. foo@PLT
    Unknown      // not recognised: no semantic properties are claimed
};

struct Operand {
    QString rawText;                 // always the original text (trimmed)
    OperandKind kind{OperandKind::Unknown};
    QString registerName;            // for Register (without '%')
    QString symbol;                  // referenced symbol for Symbol/Relocation/Immediate/Memory, if any
    QString relocation;              // e.g. "PLT", "GOTPCREL" when present
    bool indirect{false};            // AT&T '*' prefix (indirect branch/call operand)

    // Convenience flags derived from `kind`
    bool isRegister{false};
    bool isMemory{false};
    bool isImmediate{false};
    bool isSymbol{false};
    bool isRelocation{false};
};

enum class InstructionKind {
    Normal,
    ConditionalBranch,
    UnconditionalBranch,   // direct jmp
    IndirectBranch,        // jmp *reg / jmp *mem (also jump tables)
    Call,                  // direct call
    IndirectCall,
    Return,
    Trap,                  // ud2/hlt: terminates the block, no successors
    Unknown                // could not be classified (never treated as control flow)
};

inline QString instructionKindToString(InstructionKind k) {
    switch (k) {
        case InstructionKind::Normal: return "Normal";
        case InstructionKind::ConditionalBranch: return "ConditionalBranch";
        case InstructionKind::UnconditionalBranch: return "UnconditionalBranch";
        case InstructionKind::IndirectBranch: return "IndirectBranch";
        case InstructionKind::Call: return "Call";
        case InstructionKind::IndirectCall: return "IndirectCall";
        case InstructionKind::Return: return "Return";
        case InstructionKind::Trap: return "Trap";
        case InstructionKind::Unknown: return "Unknown";
    }
    return "Unknown";
}

struct Instruction {
    QString mnemonic;
    QString prefix;                 // lock/rep/notrack/... if present
    QVector<Operand> operands;
    QString rawText;
    SourceLocation sourceLoc;
    int originalLineNumber{0};      // 1-based line in the assembly text
    InstructionKind kind{InstructionKind::Normal};
    bool isBranch{false};           // any jump or call
    QString branchTarget;           // direct target text; empty for indirect/none

    bool isTerminator() const {
        return kind == InstructionKind::ConditionalBranch || kind == InstructionKind::UnconditionalBranch ||
               kind == InstructionKind::IndirectBranch || kind == InstructionKind::Return ||
               kind == InstructionKind::Trap;
    }
};

struct Label {
    QString name;
    bool isLocal{false};            // assembler-local / compiler generated (.L*, numeric)
    int originalLineNumber{0};
    QString section;
    bool inCodeSection{false};
};

enum class EdgeKind {
    Taken,          // conditional branch taken
    Fallthrough,    // next block in program order
    Unconditional,  // direct jmp
    IndirectUnknown // computed jump: target not resolvable, NO concrete target
};

inline QString edgeKindToString(EdgeKind k) {
    switch (k) {
        case EdgeKind::Taken: return "Taken";
        case EdgeKind::Fallthrough: return "Fallthrough";
        case EdgeKind::Unconditional: return "Unconditional";
        case EdgeKind::IndirectUnknown: return "IndirectUnknown";
    }
    return "Unknown";
}

struct BlockEdge {
    int from{-1};          // block id (index in Function::basicBlocks)
    int to{-1};            // block id, or -1 if the target is outside the function / unresolved
    EdgeKind kind{EdgeKind::Fallthrough};
    QString targetName;    // textual target (empty for Fallthrough and IndirectUnknown)
    bool leavesFunction{false};  // direct jump to a symbol that is not a block of this function (tail call)
};

struct BasicBlock {
    int id{-1};                 // stable: index within the function
    QString name;
    QStringList labels;         // all labels (aliases) that point at this block's start
    QVector<Instruction> instructions;
    QStringList outgoingJumps;  // names of successor blocks (compat)
    QStringList incomingJumps;  // names of predecessor blocks (compat)
    QVector<int> successors;    // indices into Function::edges
    QVector<int> predecessors;  // indices into Function::edges
    int startLineNumber{0};
    int endLineNumber{0};
};

struct Function {
    QString name;
    bool isGlobal{false};
    bool declaredAsFunction{false};   // had `.type name, @function`
    QString section;
    QVector<BasicBlock> basicBlocks;
    QVector<BlockEdge> edges;
    int startLineNumber{0};
    int endLineNumber{0};
    qint64 sizeBytes{-1};             // only from a numeric `.size`; -1 = unknown

    int totalInstructions() const {
        int count = 0;
        for (const auto& bb : basicBlocks) count += bb.instructions.size();
        return count;
    }
};

struct AssemblyModule {
    TargetArchitecture architecture{TargetArchitecture::X86_64};
    AsmSyntax syntax{AsmSyntax::Auto};
    QVector<Function> functions;
    QVector<Label> labels;
    QStringList globalDirectives;
    QStringList unparsedLines;
    QMap<int, QString> sourceFiles;   // .file id -> path
    QStringList warnings;
    bool truncated{false};            // input limits were hit: the model is incomplete

    int totalInstructions() const {
        int count = 0;
        for (const auto& fn : functions) count += fn.totalInstructions();
        return count;
    }

    QMap<QString, int> instructionFrequencies() const {
        QMap<QString, int> freqs;
        for (const auto& fn : functions)
            for (const auto& bb : fn.basicBlocks)
                for (const auto& inst : bb.instructions)
                    if (!inst.mnemonic.isEmpty()) freqs[inst.mnemonic.toLower()]++;
        return freqs;
    }
};

// Metrics computed from the structured model only (no re-parsing, no regexes on raw text).
struct AssemblyMetrics {
    int functionCount{0};
    int basicBlockCount{0};
    int instructionCount{0};
    int edgeCount{0};
    int conditionalJumps{0};
    int unconditionalJumps{0};   // direct jmp (includes tail calls)
    int indirectJumps{0};
    int calls{0};                // direct + indirect
    int returns{0};
    int traps{0};
    int tailCalls{0};            // unconditional jumps that leave the function
    QMap<QString, int> instructionFrequencies;
    // Code size is NOT derivable from assembly text. It is only reported if every function has a
    // literal `.size`; otherwise codeSizeKnown stays false and codeSizeBytes is meaningless.
    bool codeSizeKnown{false};
    qint64 codeSizeBytes{0};
};

AssemblyMetrics computeAssemblyMetrics(const AssemblyModule& module);

#endif // ASSEMBLYMODEL_H

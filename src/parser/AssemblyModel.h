#ifndef ASSEMBLYMODEL_H
#define ASSEMBLYMODEL_H

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

struct SourceLocation {
    QString fileName;
    int line{0};
    int column{0};

    bool isValid() const { return line > 0; }
};

struct Operand {
    QString rawText;
    bool isRegister{false};
    bool isMemory{false};
    bool isImmediate{false};
};

struct Instruction {
    QString mnemonic;
    QVector<Operand> operands;
    QString rawText;
    SourceLocation sourceLoc;
    int originalLineNumber{0};
    bool isBranch{false};
    QString branchTarget;
};

struct Label {
    QString name;
    bool isLocal{false};
    int originalLineNumber{0};
};

struct BasicBlock {
    QString name;
    QVector<Instruction> instructions;
    QStringList outgoingJumps;
    QStringList incomingJumps;
    int startLineNumber{0};
};

struct Function {
    QString name;
    QVector<BasicBlock> basicBlocks;
    int startLineNumber{0};
    int endLineNumber{0};

    int totalInstructions() const {
        int count = 0;
        for (const auto& bb : basicBlocks) {
            count += bb.instructions.size();
        }
        return count;
    }
};

struct AssemblyModule {
    TargetArchitecture architecture{TargetArchitecture::X86_64};
    QVector<Function> functions;
    QVector<Label> labels;
    QStringList globalDirectives;
    QStringList unparsedLines;

    int totalInstructions() const {
        int count = 0;
        for (const auto& fn : functions) {
            count += fn.totalInstructions();
        }
        return count;
    }

    QMap<QString, int> instructionFrequencies() const {
        QMap<QString, int> freqs;
        for (const auto& fn : functions) {
            for (const auto& bb : fn.basicBlocks) {
                for (const auto& inst : bb.instructions) {
                    if (!inst.mnemonic.isEmpty()) {
                        freqs[inst.mnemonic.toLower()]++;
                    }
                }
            }
        }
        return freqs;
    }
};

#endif // ASSEMBLYMODEL_H

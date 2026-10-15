#ifndef ASSEMBLYPARSER_H
#define ASSEMBLYPARSER_H

#include "AssemblyModel.h"

struct ParseLimits {
    int maxLines{2000000};
    int maxLineLength{65536};
};

/**
 * Structured parser for GNU-assembler style x86-64 output (GCC/Clang, AT&T or Intel syntax).
 * Produces functions, basic blocks and an explicit control-flow graph. Anything it cannot
 * interpret is kept verbatim (unparsedLines / OperandKind::Unknown) instead of being guessed.
 */
class AssemblyParser {
public:
    static AssemblyModule parseX86_64(const QString& assemblyText,
                                      AsmSyntax syntax = AsmSyntax::Auto,
                                      const ParseLimits& limits = ParseLimits());

    // AArch64 (GNU assembler syntax, as emitted by GCC and Clang for Linux and by Clang for Apple).
    static AssemblyModule parseARM64(const QString& assemblyText, const ParseLimits& limits = ParseLimits());

    // Dispatches on the architecture; Unknown detects it from the text (x86-64 if ambiguous).
    static AssemblyModule parse(const QString& assemblyText, TargetArchitecture arch = TargetArchitecture::Unknown,
                                const ParseLimits& limits = ParseLimits());
    // Heuristic detection from directives, register names and instruction forms.
    static TargetArchitecture detectArchitecture(const QString& assemblyText);

    static Operand parseOperandARM64(const QString& rawOperand, bool branchContext = false);
    static InstructionKind classifyMnemonicARM64(const QString& mnemonic, const QVector<Operand>& operands);

    // Exposed for testing and for the analysis UI.
    static Operand parseOperand(const QString& rawOperand, AsmSyntax syntax, bool branchContext = false);
    static InstructionKind classifyMnemonic(const QString& mnemonic, const QVector<Operand>& operands,
                                            AsmSyntax syntax);
    static QStringList splitOperands(const QString& operandText);

private:
    static AssemblyModule parseImpl(const QString& text, TargetArchitecture arch, AsmSyntax syntaxHint, const ParseLimits& limits);
};

#endif // ASSEMBLYPARSER_H

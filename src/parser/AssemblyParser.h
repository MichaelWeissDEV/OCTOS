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

    // Exposed for testing and for the analysis UI.
    static Operand parseOperand(const QString& rawOperand, AsmSyntax syntax, bool branchContext = false);
    static InstructionKind classifyMnemonic(const QString& mnemonic, const QVector<Operand>& operands,
                                            AsmSyntax syntax);
    static QStringList splitOperands(const QString& operandText);
};

#endif // ASSEMBLYPARSER_H

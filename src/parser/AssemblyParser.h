#ifndef ASSEMBLYPARSER_H
#define ASSEMBLYPARSER_H

#include "AssemblyModel.h"

class AssemblyParser {
public:
    static AssemblyModule parseX86_64(const QString& assemblyText);

private:
    static bool parseInstruction(const QString& line, int lineNum,
                                 Instruction* outInst, const SourceLocation& currentLoc);
    static Operand parseOperand(const QString& rawOperand);
};

#endif // ASSEMBLYPARSER_H

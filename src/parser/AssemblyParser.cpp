#include "AssemblyParser.h"
#include <QRegularExpression>

Operand AssemblyParser::parseOperand(const QString& rawOperand) {
    Operand op;
    op.rawText = rawOperand.trimmed();

    if (op.rawText.startsWith('%') || op.rawText.startsWith('r') || op.rawText.startsWith('e') ||
        op.rawText.startsWith('x') || op.rawText.startsWith('w')) {
        op.isRegister = true;
    } else if (op.rawText.contains('(') || op.rawText.contains('[') || op.rawText.contains(')')) {
        op.isMemory = true;
    } else if (op.rawText.startsWith('$') || op.rawText.startsWith('#') ||
               (op.rawText.size() > 0 && (op.rawText[0].isDigit() || op.rawText[0] == '-'))) {
        op.isImmediate = true;
    }

    return op;
}

bool AssemblyParser::parseInstruction(const QString& line, int lineNum,
                                      Instruction* outInst, const SourceLocation& currentLoc) {
    if (!outInst) return false;

    QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('.') || trimmed.startsWith('#') || trimmed.startsWith(';')) {
        return false;
    }

    // Strip inline comment
    int commentIdx = trimmed.indexOf('#');
    if (commentIdx < 0) commentIdx = trimmed.indexOf(';');
    if (commentIdx >= 0) {
        trimmed = trimmed.left(commentIdx).trimmed();
    }

    if (trimmed.endsWith(':')) {
        return false; // It's a label
    }

    // Extract mnemonic
    int spaceIdx = trimmed.indexOf(QRegularExpression(R"(\s)"));
    QString mnemonic;
    QString operandsStr;

    if (spaceIdx < 0) {
        mnemonic = trimmed;
    } else {
        mnemonic = trimmed.left(spaceIdx);
        operandsStr = trimmed.mid(spaceIdx + 1).trimmed();
    }

    outInst->mnemonic = mnemonic;
    outInst->rawText = line;
    outInst->sourceLoc = currentLoc;
    outInst->originalLineNumber = lineNum;

    // Check if it's a branch instruction
    QString mLower = mnemonic.toLower();
    if (mLower.startsWith('j') || mLower == "call" || mLower == "b" || mLower == "bl") {
        outInst->isBranch = true;
        outInst->branchTarget = operandsStr.trimmed();
    }

    // Split operands respecting memory operand commas like (%rdi, %rsi, 4)
    if (!operandsStr.isEmpty()) {
        QString currentOp;
        int parenDepth = 0;
        int bracketDepth = 0;

        for (int i = 0; i < operandsStr.size(); ++i) {
            QChar c = operandsStr[i];
            if (c == '(') parenDepth++;
            else if (c == ')') parenDepth--;
            else if (c == '[') bracketDepth++;
            else if (c == ']') bracketDepth--;

            if (c == ',' && parenDepth == 0 && bracketDepth == 0) {
                outInst->operands.append(parseOperand(currentOp));
                currentOp.clear();
            } else {
                currentOp.append(c);
            }
        }
        if (!currentOp.trimmed().isEmpty()) {
            outInst->operands.append(parseOperand(currentOp));
        }
    }

    return true;
}

AssemblyModule AssemblyParser::parseX86_64(const QString& assemblyText) {
    AssemblyModule module;
    module.architecture = TargetArchitecture::X86_64;

    QStringList lines = assemblyText.split('\n');

    Function currentFunction;
    bool inFunction = false;
    BasicBlock currentBlock;
    bool inBlock = false;
    SourceLocation currentLoc;

    auto finishBlock = [&]() {
        if (inBlock) {
            if (!currentBlock.instructions.isEmpty() || !currentBlock.name.isEmpty()) {
                currentFunction.basicBlocks.append(currentBlock);
            }
            currentBlock = BasicBlock();
            inBlock = false;
        }
    };

    auto finishFunction = [&]() {
        finishBlock();
        if (inFunction) {
            currentFunction.endLineNumber = lines.size();
            module.functions.append(currentFunction);
            currentFunction = Function();
            inFunction = false;
        }
    };

    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        int lineNum = lineIdx + 1;
        const QString& rawLine = lines[lineIdx];
        QString trimmed = rawLine.trimmed();

        if (trimmed.isEmpty()) continue;

        // Label definition
        if (trimmed.endsWith(':')) {
            QString labelName = trimmed.left(trimmed.size() - 1).trimmed();

            Label lbl;
            lbl.name = labelName;
            lbl.isLocal = labelName.startsWith('.');
            lbl.originalLineNumber = lineNum;
            module.labels.append(lbl);

            if (!lbl.isLocal) {
                // New Function
                finishFunction();
                inFunction = true;
                currentFunction.name = labelName;
                currentFunction.startLineNumber = lineNum;

                currentBlock.name = labelName;
                currentBlock.startLineNumber = lineNum;
                inBlock = true;
            } else {
                // Local basic block within current function
                if (!inFunction) {
                    inFunction = true;
                    currentFunction.name = "global_scope";
                    currentFunction.startLineNumber = lineNum;
                }
                finishBlock();
                currentBlock.name = labelName;
                currentBlock.startLineNumber = lineNum;
                inBlock = true;
            }
            continue;
        }

        // Parse source location directives: .loc <file> <line> <column>
        if (trimmed.startsWith(".loc ")) {
            QStringList parts = trimmed.split(QRegularExpression(R"(\s+)"));
            if (parts.size() >= 3) {
                currentLoc.line = parts[2].toInt();
                if (parts.size() >= 4) {
                    currentLoc.column = parts[3].toInt();
                }
            }
            module.globalDirectives.append(rawLine);
            continue;
        }

        // Parse global directives
        if (trimmed.startsWith('.')) {
            module.globalDirectives.append(rawLine);
            continue;
        }

        // Try parsing as instruction
        Instruction inst;
        if (parseInstruction(rawLine, lineNum, &inst, currentLoc)) {
            if (!inFunction) {
                inFunction = true;
                currentFunction.name = "anonymous_function";
                currentFunction.startLineNumber = lineNum;
            }
            if (!inBlock) {
                currentBlock.name = currentFunction.name + "_entry";
                currentBlock.startLineNumber = lineNum;
                inBlock = true;
            }

            if (inst.isBranch && !inst.branchTarget.isEmpty()) {
                currentBlock.outgoingJumps.append(inst.branchTarget);
            }

            currentBlock.instructions.append(inst);
        } else {
            module.unparsedLines.append(rawLine);
        }
    }

    finishFunction();

    // Wire up incoming jumps across basic blocks
    for (auto& fn : module.functions) {
        for (const auto& bb : fn.basicBlocks) {
            for (const QString& target : bb.outgoingJumps) {
                for (auto& targetBb : fn.basicBlocks) {
                    if (targetBb.name == target) {
                        targetBb.incomingJumps.append(bb.name);
                    }
                }
            }
        }
    }

    return module;
}

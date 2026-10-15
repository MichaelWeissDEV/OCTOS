#include "AssemblyParser.h"
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace {

const QRegularExpression& x86RegisterRegex() {
    static const QRegularExpression re(
        R"(^(?:[re]?(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][lh]|(?:sil|dil|bpl|spl)|r(?:[89]|1[0-5])[dwb]?|[re]?ip|[cdefgs]s|[xyz]mm\d{1,2}|mm[0-7]|st(?:\(\d\))?|k[0-7]|cr\d{1,2}|dr\d{1,2}|e?flags|rflags)$)",
        QRegularExpression::CaseInsensitiveOption);
    return re;
}

const QRegularExpression& numberRegex() {
    static const QRegularExpression re(R"(^[-+]?(?:0[xX][0-9a-fA-F]+|[0-9]+|[0-9][0-9a-fA-F]*[hH])$)");
    return re;
}

const QRegularExpression& identRegex() {
    static const QRegularExpression re(R"(^[A-Za-z_.$][A-Za-z0-9_.$]*$)");
    return re;
}

const QSet<QString>& conditionCodes() {
    static const QSet<QString> s = {
        "a", "ae", "b", "be", "c", "e", "g", "ge", "l", "le", "na", "nae", "nb", "nbe", "nc", "ne", "ng",
        "nge", "nl", "nle", "no", "np", "ns", "nz", "o", "p", "pe", "po", "s", "z"};
    return s;
}

const QSet<QString>& prefixes() {
    static const QSet<QString> s = {"lock", "rep", "repe", "repz", "repne", "repnz", "notrack", "bnd",
                                    "data16", "data32", "addr32", "cs", "ds", "es", "fs", "gs", "ss",
                                    "xacquire", "xrelease"};
    return s;
}

// Cuts an x86 '#' comment, ignoring '#' inside string literals.
QString stripX86Comment(const QString& line) {
    bool inStr = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (inStr) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        if (c == '#') return line.left(i);
    }
    return line;
}

bool isLocalLabelName(const QString& name) {
    if (name.startsWith(".L")) return true;
    if (name.isEmpty()) return false;
    for (QChar c : name) if (!c.isDigit()) return false;
    return true;  // numeric local label
}

const QRegularExpression& wsRegex() {
    static const QRegularExpression re(R"(\s+)");
    return re;
}

QString unquote(QString s) {
    s = s.trimmed();
    if (s.size() >= 2 && s.startsWith('"') && s.endsWith('"')) s = s.mid(1, s.size() - 2);
    return s;
}

// Splits "a, "b,c", d" at top-level commas (outside quotes/brackets).
QStringList splitArgs(const QString& s) {
    QStringList out;
    QString cur;
    int depth = 0;
    bool inStr = false;
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s[i];
        if (inStr) {
            cur += c;
            if (c == '\\' && i + 1 < s.size()) cur += s[++i];
            else if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; cur += c; continue; }
        if (c == '(' || c == '[') ++depth;
        else if (c == ')' || c == ']') depth = qMax(0, depth - 1);
        if (c == ',' && depth == 0) { out << cur.trimmed(); cur.clear(); continue; }
        cur += c;
    }
    if (!cur.trimmed().isEmpty() || !out.isEmpty()) out << cur.trimmed();
    return out;
}

// Whitespace-separated tokens, quoted strings kept together (used by .file).
QStringList splitWhitespaceQuoted(const QString& s) {
    QStringList out;
    QString cur;
    bool inStr = false;
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s[i];
        if (inStr) {
            cur += c;
            if (c == '\\' && i + 1 < s.size()) cur += s[++i];
            else if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; cur += c; continue; }
        if (c.isSpace()) { if (!cur.isEmpty()) { out << cur; cur.clear(); } continue; }
        cur += c;
    }
    if (!cur.isEmpty()) out << cur;
    return out;
}

struct Item {
    bool isLabel{false};
    QString label;
    int line{0};
    Instruction inst;
};

void finalizeOperand(Operand& op) {
    op.isRegister = op.kind == OperandKind::Register;
    op.isMemory = op.kind == OperandKind::Memory;
    op.isImmediate = op.kind == OperandKind::Immediate;
    op.isSymbol = op.kind == OperandKind::Symbol;
    op.isRelocation = op.kind == OperandKind::Relocation;
}

QString leadingIdentifier(const QString& s) {
    static const QRegularExpression re(R"(^([A-Za-z_.$][A-Za-z0-9_.$]*))");
    auto m = re.match(s);
    return m.hasMatch() ? m.captured(1) : QString();
}

QString relocationOf(const QString& s) {
    static const QRegularExpression re(R"(@([A-Za-z]+))");
    auto m = re.match(s);
    return m.hasMatch() ? m.captured(1).toUpper() : QString();
}

} // namespace

QStringList AssemblyParser::splitOperands(const QString& operandText) {
    QStringList parts = splitArgs(operandText);
    parts.removeAll(QString());
    return parts;
}

Operand AssemblyParser::parseOperand(const QString& rawOperand, AsmSyntax syntax, bool branchContext) {
    Operand op;
    op.rawText = rawOperand.trimmed();
    QString t = op.rawText;
    op.kind = OperandKind::Unknown;

    auto done = [&]() { finalizeOperand(op); return op; };
    if (t.isEmpty()) return done();

    if (syntax == AsmSyntax::Intel) {
        if (t.startsWith("OFFSET ", Qt::CaseInsensitive)) {
            QString sym = t.mid(7).trimmed();
            if (sym.startsWith("FLAT:", Qt::CaseInsensitive)) sym = sym.mid(5);
            op.kind = OperandKind::Immediate;
            op.symbol = leadingIdentifier(sym);
            return done();
        }
        if (t.contains('[')) {
            op.kind = OperandKind::Memory;
            op.relocation = relocationOf(t);
            return done();
        }
        if (x86RegisterRegex().match(t).hasMatch()) {
            op.kind = OperandKind::Register;
            op.registerName = t.toLower();
            return done();
        }
        if (numberRegex().match(t).hasMatch()) {
            op.kind = OperandKind::Immediate;
            return done();
        }
        // sym@PLT or a bare identifier: only meaningful as a branch target
        if (branchContext) {
            const QString base = t.section('@', 0, 0);
            if (identRegex().match(base).hasMatch()) {
                op.symbol = base;
                op.relocation = relocationOf(t);
                op.kind = op.relocation.isEmpty() ? OperandKind::Symbol : OperandKind::Relocation;
                return done();
            }
        }
        return done();  // Unknown
    }

    // AT&T
    if (t.startsWith('*')) {
        op.indirect = true;
        t = t.mid(1).trimmed();
    }
    if (t.startsWith('%')) {
        static const QRegularExpression pureReg(R"(^%([A-Za-z][A-Za-z0-9]*(?:\(\d+\))?)$)");
        auto m = pureReg.match(t);
        if (m.hasMatch()) {
            op.kind = OperandKind::Register;
            op.registerName = m.captured(1).toLower();
        } else if (t.contains(':')) {
            op.kind = OperandKind::Memory;   // segment override, e.g. %fs:0x28
        }
        return done();
    }
    if (t.startsWith('$')) {
        op.kind = OperandKind::Immediate;
        const QString rest = t.mid(1).trimmed();
        op.symbol = leadingIdentifier(rest);
        op.relocation = relocationOf(rest);
        return done();
    }
    if (t.contains('(') && t.contains(')')) {
        op.kind = OperandKind::Memory;
        const QString disp = t.left(t.indexOf('(')).trimmed();
        op.symbol = leadingIdentifier(disp);
        op.relocation = relocationOf(disp);
        return done();
    }
    if (numberRegex().match(t).hasMatch()) {
        // A bare number is an absolute memory reference in AT&T; as a branch target it is an
        // absolute address we do not interpret.
        if (!branchContext) op.kind = OperandKind::Memory;
        return done();
    }
    const QString base = t.section('@', 0, 0);
    // identifier optionally with +/- constant offset
    const QString ident = leadingIdentifier(base);
    if (!ident.isEmpty()) {
        const QString tail = base.mid(ident.size()).trimmed();
        const bool plainOffset = tail.isEmpty() || numberRegex().match(tail).hasMatch();
        if (plainOffset) {
            op.symbol = ident;
            op.relocation = relocationOf(t);
            if (!op.relocation.isEmpty() && branchContext) op.kind = OperandKind::Relocation;
            else if (branchContext) op.kind = OperandKind::Symbol;
            else op.kind = OperandKind::Memory;  // AT&T: a bare symbol operand addresses memory
            return done();
        }
    }
    return done();  // Unknown
}

InstructionKind AssemblyParser::classifyMnemonic(const QString& mnemonic, const QVector<Operand>& operands,
                                                 AsmSyntax syntax) {
    const QString m = mnemonic.toLower();

    auto isIndirectOperand = [&]() {
        if (operands.isEmpty()) return false;
        const Operand& o = operands.first();
        if (syntax == AsmSyntax::Intel) return o.kind == OperandKind::Register || o.kind == OperandKind::Memory;
        return o.indirect;
    };

    static const QSet<QString> rets = {"ret", "retq", "retl", "retw", "lret", "lretq", "lretl", "retf", "retn",
                                       "iret", "iretq", "iretl", "iretw", "sysret", "sysretq", "sysretl",
                                       "sysexit", "sysexitq"};
    if (rets.contains(m)) return InstructionKind::Return;
    if (m == "ud2" || m == "ud1" || m == "ud0" || m == "hlt") return InstructionKind::Trap;

    if (m == "jmp" || m == "jmpq" || m == "jmpl" || m == "jmpw" || m == "ljmp" || m == "ljmpq" || m == "ljmpl") {
        if (operands.isEmpty()) return InstructionKind::Unknown;
        return isIndirectOperand() ? InstructionKind::IndirectBranch : InstructionKind::UnconditionalBranch;
    }
    if (m == "call" || m == "callq" || m == "calll" || m == "callw" || m == "lcall" || m == "lcallq" || m == "lcalll") {
        if (operands.isEmpty()) return InstructionKind::Unknown;
        return isIndirectOperand() ? InstructionKind::IndirectCall : InstructionKind::Call;
    }
    if (m.size() >= 2 && m[0] == 'j') {
        const QString cc = m.mid(1);
        if (conditionCodes().contains(cc) || cc == "cxz" || cc == "ecxz" || cc == "rcxz")
            return operands.isEmpty() ? InstructionKind::Unknown : InstructionKind::ConditionalBranch;
    }
    if (m == "loop" || m == "loope" || m == "loopne" || m == "loopz" || m == "loopnz")
        return operands.isEmpty() ? InstructionKind::Unknown : InstructionKind::ConditionalBranch;
    return InstructionKind::Normal;
}

namespace {

struct FileState {
    QMap<int, QString> files;
};

SourceLocation resolveLoc(const QMap<int, QString>& files, int fileId, int line, int col) {
    SourceLocation loc;
    loc.fileId = fileId;
    loc.line = line;
    loc.column = col;
    auto it = files.constFind(fileId);
    if (it != files.constEnd()) {
        loc.fileName = it.value();
        loc.fileKnown = true;
    }  // else: unknown stays unknown, no guess
    return loc;
}

void buildControlFlow(Function& fn, const QVector<Item>& items) {
    // Instruction view
    QVector<int> instItem;      // item index per instruction
    for (int i = 0; i < items.size(); ++i) if (!items[i].isLabel) instItem.append(i);
    const int n = instItem.size();
    if (n == 0) return;

    // Instruction index that follows each label item (== n if none)
    QVector<int> instsBefore(items.size() + 1, 0);
    for (int i = 0; i < items.size(); ++i) instsBefore[i + 1] = instsBefore[i] + (items[i].isLabel ? 0 : 1);

    QHash<QString, int> labelToInst;
    struct NumLabel { QString name; int item; int inst; };
    QVector<NumLabel> numeric;
    for (int i = 0; i < items.size(); ++i) {
        if (!items[i].isLabel) continue;
        const QString& name = items[i].label;
        const int k = instsBefore[i];
        if (!labelToInst.contains(name)) labelToInst.insert(name, k);
        bool allDigits = !name.isEmpty();
        for (QChar c : name) if (!c.isDigit()) { allDigits = false; break; }
        if (allDigits) numeric.append({name, i, k});
    }

    // returns instruction index (0..n; n == "after the last instruction"), or -1 if not in this function
    auto resolveTarget = [&](const QString& rawTarget, int fromItem) -> int {
        const QString t = rawTarget.trimmed();
        static const QRegularExpression numRef(R"(^(\d+)([fFbB])$)");
        auto m = numRef.match(t);
        if (m.hasMatch()) {
            const bool fwd = m.captured(2).toLower() == "f";
            int best = -1;
            for (const NumLabel& nl : numeric) {
                if (nl.name != m.captured(1)) continue;
                if (fwd && nl.item > fromItem) { best = nl.inst; break; }
                if (!fwd && nl.item < fromItem) best = nl.inst;
            }
            return best;
        }
        auto it = labelToInst.constFind(t);
        return it == labelToInst.constEnd() ? -1 : it.value();
    };

    // Leaders: function entry, resolved direct targets, successors of terminators.
    QSet<int> leaders;
    leaders.insert(0);
    QVector<int> resolved(n, -1);   // resolved target instruction for direct jumps
    for (int i = 0; i < n; ++i) {
        const Instruction& in = items[instItem[i]].inst;
        const bool direct = in.kind == InstructionKind::ConditionalBranch || in.kind == InstructionKind::UnconditionalBranch;
        if (direct && !in.branchTarget.isEmpty()) {
            resolved[i] = resolveTarget(in.branchTarget, instItem[i]);
            if (resolved[i] >= 0 && resolved[i] < n) leaders.insert(resolved[i]);
        }
        if (in.isTerminator() && i + 1 < n) leaders.insert(i + 1);
    }
    QVector<int> starts = leaders.values().toVector();
    std::sort(starts.begin(), starts.end());

    QVector<int> blockOfInst(n, 0);
    fn.basicBlocks.clear();
    for (int b = 0; b < starts.size(); ++b) {
        const int s = starts[b];
        const int e = (b + 1 < starts.size()) ? starts[b + 1] : n;  // exclusive
        BasicBlock bb;
        bb.id = b;
        for (int i = s; i < e; ++i) {
            bb.instructions.append(items[instItem[i]].inst);
            blockOfInst[i] = b;
        }
        bb.startLineNumber = items[instItem[s]].inst.originalLineNumber;
        bb.endLineNumber = items[instItem[e - 1]].inst.originalLineNumber;
        fn.basicBlocks.append(bb);
    }

    // Labels as block aliases
    QSet<QString> referenced;
    for (int i = 0; i < n; ++i) {
        const Instruction& in = items[instItem[i]].inst;
        if (!in.branchTarget.isEmpty() && (in.kind == InstructionKind::ConditionalBranch || in.kind == InstructionKind::UnconditionalBranch))
            referenced.insert(in.branchTarget.trimmed());
    }
    for (int i = 0; i < items.size(); ++i) {
        if (!items[i].isLabel) continue;
        const int k = instsBefore[i];
        if (k >= n) continue;  // label after the last instruction: no block to attach it to
        BasicBlock& bb = fn.basicBlocks[blockOfInst[k]];
        // A label that sits in the middle of a block (unreferenced) is still an alias of that block.
        bb.labels.append(items[i].label);
        if (bb.startLineNumber > items[i].line && k == starts[blockOfInst[k]]) bb.startLineNumber = items[i].line;
    }
    for (int b = 0; b < fn.basicBlocks.size(); ++b) {
        BasicBlock& bb = fn.basicBlocks[b];
        if (b == 0) {
            bb.name = fn.name;
        } else if (!bb.labels.isEmpty()) {
            bb.name = bb.labels.first();
            for (const QString& l : bb.labels) if (referenced.contains(l)) { bb.name = l; break; }
        } else {
            bb.name = QString("%1.bb%2").arg(fn.name).arg(b);
        }
    }

    // Edges
    auto addEdge = [&](int from, int to, EdgeKind kind, const QString& target, bool leaves) {
        BlockEdge e;
        e.from = from; e.to = to; e.kind = kind; e.targetName = target; e.leavesFunction = leaves;
        fn.edges.append(e);
        const int idx = fn.edges.size() - 1;
        fn.basicBlocks[from].successors.append(idx);
        if (to >= 0) fn.basicBlocks[to].predecessors.append(idx);
    };

    for (int b = 0; b < fn.basicBlocks.size(); ++b) {
        const int endInst = (b + 1 < starts.size()) ? starts[b + 1] - 1 : n - 1;
        const Instruction& last = items[instItem[endInst]].inst;
        const bool hasNext = b + 1 < fn.basicBlocks.size();
        auto targetBlock = [&](int instIdx) { return (instIdx >= 0 && instIdx < n) ? blockOfInst[instIdx] : -1; };

        switch (last.kind) {
            case InstructionKind::ConditionalBranch: {
                const int tb = targetBlock(resolved[endInst]);
                addEdge(b, tb, EdgeKind::Taken, last.branchTarget,
                        resolved[endInst] < 0 && !last.branchTarget.isEmpty());
                if (hasNext) addEdge(b, b + 1, EdgeKind::Fallthrough, QString(), false);
                break;
            }
            case InstructionKind::UnconditionalBranch: {
                const int tb = targetBlock(resolved[endInst]);
                addEdge(b, tb, EdgeKind::Unconditional, last.branchTarget,
                        resolved[endInst] < 0 && !last.branchTarget.isEmpty());
                break;
            }
            case InstructionKind::IndirectBranch:
                // Computed target: recorded as unknown, never as an invented concrete edge.
                addEdge(b, -1, EdgeKind::IndirectUnknown, QString(), false);
                break;
            case InstructionKind::Return:
            case InstructionKind::Trap:
                break;
            default:
                if (hasNext) addEdge(b, b + 1, EdgeKind::Fallthrough, QString(), false);
                break;
        }
    }

    // Compat name lists
    for (const BlockEdge& e : std::as_const(fn.edges)) {
        if (e.to >= 0) {
            fn.basicBlocks[e.from].outgoingJumps.append(fn.basicBlocks[e.to].name);
            fn.basicBlocks[e.to].incomingJumps.append(fn.basicBlocks[e.from].name);
        }
    }
}

} // namespace

AssemblyModule AssemblyParser::parseX86_64(const QString& assemblyText, AsmSyntax syntaxHint,
                                           const ParseLimits& limits) {
    AssemblyModule module;
    module.architecture = TargetArchitecture::X86_64;

    AsmSyntax syntax = syntaxHint;
    if (syntax == AsmSyntax::Auto) {
        syntax = assemblyText.contains(".intel_syntax") ? AsmSyntax::Intel : AsmSyntax::ATT;
    }
    module.syntax = syntax;

    const QStringList lines = assemblyText.split('\n');

    // Pre-scan: which symbols are declared as functions?
    QSet<QString> functionSyms;
    {
        static const QRegularExpression typeRe(R"(^\s*\.type\s+([^,\s]+)\s*,\s*[@%]function\b)");
        for (const QString& l : lines) {
            if (!l.contains(".type")) continue;
            auto m = typeRe.match(l);
            if (m.hasMatch()) functionSyms.insert(unquote(m.captured(1)));
        }
    }
    const bool hasTypeInfo = !functionSyms.isEmpty();

    QSet<QString> globals;
    QString section = ".text";
    bool sectionIsCode = true;
    QVector<QPair<QString, bool>> sectionStack;
    QPair<QString, bool> previousSection{".text", true};

    auto isCodeSection = [](const QString& name, const QString& flags) {
        return name.startsWith(".text") || name.startsWith("__TEXT") || name == ".init" || name == ".fini" ||
               flags.contains('x');
    };
    auto setSection = [&](const QString& name, bool code) {
        previousSection = {section, sectionIsCode};
        section = name;
        sectionIsCode = code;
    };

    Function fn;
    QVector<Item> items;
    bool inFunction = false;
    int fnLastLine = 0;
    SourceLocation currentLoc;
    int longLines = 0;

    auto finishFunction = [&]() {
        if (!inFunction) return;
        fn.isGlobal = globals.contains(fn.name);
        fn.endLineNumber = fnLastLine;
        buildControlFlow(fn, items);
        module.functions.append(fn);
        fn = Function();
        items.clear();
        inFunction = false;
    };

    auto beginFunction = [&](const QString& name, int line) {
        finishFunction();
        inFunction = true;
        fn = Function();
        fn.name = name;
        fn.startLineNumber = line;
        fn.section = section;
        fn.declaredAsFunction = functionSyms.contains(name);
        fnLastLine = line;
    };

    const int lineCount = qMin<qsizetype>(lines.size(), limits.maxLines);
    if (lines.size() > limits.maxLines) {
        module.truncated = true;
        module.warnings << QString("Input has %1 lines; only the first %2 were parsed").arg(lines.size()).arg(limits.maxLines);
    }

    for (int lineIdx = 0; lineIdx < lineCount; ++lineIdx) {
        const int lineNum = lineIdx + 1;
        const QString& rawLine = lines[lineIdx];

        if (rawLine.size() > limits.maxLineLength) {
            ++longLines;
            module.unparsedLines.append(rawLine.left(256) + "…");
            continue;
        }

        QString text = stripX86Comment(rawLine).trimmed();
        if (text.isEmpty()) continue;

        // Leading label definitions ("name:" possibly followed by an instruction)
        static const QRegularExpression labelRe(R"(^([A-Za-z_.$][A-Za-z0-9_.$@]*|\d+)\s*:(?!:))");
        for (;;) {
            auto m = labelRe.match(text);
            if (!m.hasMatch()) break;
            const QString name = m.captured(1);
            text = text.mid(m.capturedLength()).trimmed();

            Label lbl;
            lbl.name = name;
            lbl.isLocal = isLocalLabelName(name);
            lbl.originalLineNumber = lineNum;
            lbl.section = section;
            lbl.inCodeSection = sectionIsCode;
            module.labels.append(lbl);

            if (!sectionIsCode) continue;  // data label (jump table, string, debug info): not code structure

            const bool startsFunction = hasTypeInfo ? functionSyms.contains(name) : !lbl.isLocal;
            if (startsFunction) {
                beginFunction(name, lineNum);
            } else {
                if (!inFunction) beginFunction("global_scope", lineNum);
                Item it;
                it.isLabel = true;
                it.label = name;
                it.line = lineNum;
                items.append(it);
            }
            fnLastLine = lineNum;
        }
        if (text.isEmpty()) continue;

        // Directives
        if (text.startsWith('.')) {
            module.globalDirectives.append(rawLine);
            const QString dir = text.section(wsRegex(), 0, 0).toLower();
            const QString args = text.mid(dir.size()).trimmed();

            if (dir == ".text") { setSection(".text", true); }
            else if (dir == ".data" || dir == ".bss") { setSection(dir, false); }
            else if (dir == ".section" || dir == ".pushsection") {
                const QStringList a = splitArgs(args);
                const QString name = a.isEmpty() ? QString() : unquote(a[0]);
                const QString flags = a.size() > 1 ? a[1] : QString();
                if (dir == ".pushsection") sectionStack.append({section, sectionIsCode});
                setSection(name, isCodeSection(name, flags));
            } else if (dir == ".popsection") {
                if (!sectionStack.isEmpty()) {
                    auto p = sectionStack.takeLast();
                    section = p.first; sectionIsCode = p.second;
                }
            } else if (dir == ".previous") {
                std::swap(section, previousSection.first);
                std::swap(sectionIsCode, previousSection.second);
            } else if (dir == ".globl" || dir == ".global" || dir == ".weak") {
                for (const QString& s : splitArgs(args)) globals.insert(unquote(s));
            } else if (dir == ".size") {
                const QStringList a = splitArgs(args);
                if (inFunction && a.size() >= 2 && unquote(a[0]) == fn.name) {
                    bool ok = false;
                    const qint64 v = a[1].toLongLong(&ok);
                    if (ok) fn.sizeBytes = v;
                    fnLastLine = lineNum;
                    finishFunction();   // `.size` closes the function
                }
            } else if (dir == ".file") {
                const QStringList a = splitWhitespaceQuoted(args);
                bool ok = false;
                const int id = a.value(0).toInt(&ok);
                if (ok && a.size() >= 2) {
                    QString path;
                    if (a.size() >= 3) {
                        const QString d = unquote(a[1]);
                        const QString n = unquote(a[2]);
                        path = n.startsWith('/') || d.isEmpty() ? n : d + "/" + n;
                    } else {
                        path = unquote(a[1]);
                    }
                    module.sourceFiles.insert(id, path);
                }  // `.file "name"` (module source name) carries no DWARF id
            } else if (dir == ".loc") {
                const QStringList parts = args.split(wsRegex(), Qt::SkipEmptyParts);
                bool okF = false, okL = false;
                const int fileId = parts.value(0).toInt(&okF);
                const int line = parts.value(1).toInt(&okL);
                if (okF && okL && line >= 0) {  // line 0 = "no source line" in DWARF, a valid directive
                    bool okC = false;
                    int col = parts.value(2).toInt(&okC);
                    if (!okC) col = 0;
                    currentLoc = resolveLoc(module.sourceFiles, fileId, line, col);
                } else {
                    currentLoc = SourceLocation();   // unparsable mapping: unknown, not guessed
                    module.warnings << QString("Line %1: could not interpret .loc").arg(lineNum);
                }
            } else if (dir == ".intel_syntax") {
                if (syntaxHint == AsmSyntax::Auto) { syntax = AsmSyntax::Intel; module.syntax = syntax; }
            } else if (dir == ".att_syntax") {
                if (syntaxHint == AsmSyntax::Auto) { syntax = AsmSyntax::ATT; module.syntax = syntax; }
            }
            continue;
        }

        // Instruction
        if (!sectionIsCode) { module.unparsedLines.append(rawLine); continue; }

        QString rest = text;
        Instruction inst;
        // prefixes
        for (;;) {
            const QString first = rest.section(wsRegex(), 0, 0);
            const QString after = rest.mid(first.size()).trimmed();
            if (!after.isEmpty() && prefixes().contains(first.toLower())) {
                inst.prefix += (inst.prefix.isEmpty() ? "" : " ") + first.toLower();
                rest = after;
            } else break;
        }
        const QString mnemonic = rest.section(wsRegex(), 0, 0);
        static const QRegularExpression mnemRe(R"(^[A-Za-z][A-Za-z0-9_.]*$)");
        if (!mnemRe.match(mnemonic).hasMatch()) {
            module.unparsedLines.append(rawLine);
            continue;
        }
        const QString operandText = rest.mid(mnemonic.size()).trimmed();

        inst.mnemonic = mnemonic;
        inst.rawText = rawLine;
        inst.sourceLoc = currentLoc;
        inst.originalLineNumber = lineNum;

        // Operands. Whether an operand is a branch target depends on the mnemonic.
        const QString mLower = mnemonic.toLower();
        const bool branchLike = mLower.startsWith('j') || mLower.startsWith("call") || mLower.startsWith("lcall") ||
                                mLower.startsWith("loop") || mLower.startsWith("ljmp");
        for (const QString& o : splitOperands(operandText)) {
            inst.operands.append(parseOperand(o, syntax, branchLike));
        }
        inst.kind = classifyMnemonic(mnemonic, inst.operands, syntax);
        inst.isBranch = inst.kind == InstructionKind::ConditionalBranch || inst.kind == InstructionKind::UnconditionalBranch ||
                        inst.kind == InstructionKind::IndirectBranch || inst.kind == InstructionKind::Call ||
                        inst.kind == InstructionKind::IndirectCall;
        if ((inst.kind == InstructionKind::ConditionalBranch || inst.kind == InstructionKind::UnconditionalBranch ||
             inst.kind == InstructionKind::Call) && !inst.operands.isEmpty()) {
            const Operand& o = inst.operands.first();
            inst.branchTarget = (o.kind == OperandKind::Symbol || o.kind == OperandKind::Relocation || o.kind == OperandKind::Unknown)
                                    ? (o.symbol.isEmpty() ? o.rawText : (o.kind == OperandKind::Symbol ? o.rawText : o.symbol))
                                    : QString();
        }

        if (!inFunction) beginFunction("anonymous_function", lineNum);
        Item it;
        it.isLabel = false;
        it.line = lineNum;
        it.inst = inst;
        items.append(it);
        fnLastLine = lineNum;
    }

    if (longLines > 0) {
        module.truncated = true;
        module.warnings << QString("%1 line(s) exceeded the maximum line length and were skipped").arg(longLines);
    }

    finishFunction();
    return module;
}

AssemblyMetrics computeAssemblyMetrics(const AssemblyModule& module) {
    AssemblyMetrics m;
    m.functionCount = module.functions.size();
    bool allSizesKnown = !module.functions.isEmpty();
    for (const Function& fn : module.functions) {
        m.basicBlockCount += fn.basicBlocks.size();
        m.edgeCount += fn.edges.size();
        if (fn.sizeBytes >= 0) m.codeSizeBytes += fn.sizeBytes; else allSizesKnown = false;
        for (const BlockEdge& e : fn.edges) {
            if (e.leavesFunction && e.kind == EdgeKind::Unconditional) ++m.tailCalls;
        }
        for (const BasicBlock& bb : fn.basicBlocks) {
            for (const Instruction& in : bb.instructions) {
                ++m.instructionCount;
                if (!in.mnemonic.isEmpty()) m.instructionFrequencies[in.mnemonic.toLower()]++;
                switch (in.kind) {
                    case InstructionKind::ConditionalBranch: ++m.conditionalJumps; break;
                    case InstructionKind::UnconditionalBranch: ++m.unconditionalJumps; break;
                    case InstructionKind::IndirectBranch: ++m.indirectJumps; break;
                    case InstructionKind::Call:
                    case InstructionKind::IndirectCall: ++m.calls; break;
                    case InstructionKind::Return: ++m.returns; break;
                    case InstructionKind::Trap: ++m.traps; break;
                    default: break;
                }
            }
        }
    }
    m.codeSizeKnown = allSizesKnown;
    if (!allSizesKnown) m.codeSizeBytes = 0;
    return m;
}

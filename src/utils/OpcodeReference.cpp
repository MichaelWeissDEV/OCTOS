#include "OpcodeReference.h"
#include <QRegularExpression>

OpcodeReference& OpcodeReference::instance() {
    static OpcodeReference inst;
    return inst;
}

OpcodeReference::OpcodeReference() {
    // ==========================================
    // x86 and x86-64 Instructions
    // ==========================================
    registerOpcode("mov", "x86-64", "Move data between registers or memory", "None", "mov dst, src");
    registerOpcode("movq", "x86-64", "Move 64-bit quadword data", "None", "movq dst, src");
    registerOpcode("movl", "x86-64", "Move 32-bit doubleword data (clears high 32 bits of dest reg)", "None", "movl dst, src");
    registerOpcode("movzbl", "x86-64", "Move byte to doubleword with zero-extension", "None", "movzbl src8, dst32");
    registerOpcode("movzwl", "x86-64", "Move word to doubleword with zero-extension", "None", "movzwl src16, dst32");
    registerOpcode("movsbl", "x86-64", "Move byte to doubleword with sign-extension", "None", "movsbl src8, dst32");
    registerOpcode("movslq", "x86-64", "Move doubleword to quadword with sign-extension", "None", "movslq src32, dst64");
    registerOpcode("lea", "x86-64", "Load Effective Address (computes memory offset without dereferencing)", "None", "lea dst, [base + index*scale + disp]");
    registerOpcode("leaq", "x86-64", "Load Effective Address 64-bit (often used for fast arithmetic)", "None", "leaq (%rdi,%rsi,4), %rax");

    registerOpcode("push", "x86-64", "Push value onto call stack (decrements RSP)", "None", "push src");
    registerOpcode("pushq", "x86-64", "Push 64-bit quadword onto call stack", "None", "pushq %rbp");
    registerOpcode("pop", "x86-64", "Pop value from stack into destination (increments RSP)", "None", "pop dst");
    registerOpcode("popq", "x86-64", "Pop 64-bit value from stack into register", "None", "popq %rbp");

    registerOpcode("add", "x86-64", "Add two integer operands (dst = dst + src)", "CF, OF, SF, ZF, AF, PF", "add dst, src");
    registerOpcode("addl", "x86-64", "Add 32-bit doubleword integers", "CF, OF, SF, ZF, AF, PF", "addl %esi, %eax");
    registerOpcode("addq", "x86-64", "Add 64-bit quadword integers", "CF, OF, SF, ZF, AF, PF", "addq %rsi, %rax");
    registerOpcode("sub", "x86-64", "Subtract source from destination (dst = dst - src)", "CF, OF, SF, ZF, AF, PF", "sub dst, src");
    registerOpcode("subl", "x86-64", "Subtract 32-bit doubleword integers", "CF, OF, SF, ZF, AF, PF", "subl %esi, %eax");
    registerOpcode("subq", "x86-64", "Subtract 64-bit quadword integers", "CF, OF, SF, ZF, AF, PF", "subq %rsi, %rax");

    registerOpcode("imul", "x86-64", "Signed integer multiply", "CF, OF (SF, ZF, AF, PF undefined)", "imul dst, src");
    registerOpcode("idiv", "x86-64", "Signed integer divide (divide RDX:RAX by operand)", "Flags undefined", "idiv src");
    registerOpcode("div", "x86-64", "Unsigned integer divide", "Flags undefined", "div src");

    registerOpcode("xor", "x86-64", "Bitwise exclusive OR (frequently used to zero registers)", "CF=0, OF=0, SF, ZF, PF", "xor dst, src");
    registerOpcode("xorl", "x86-64", "Bitwise XOR 32-bit (zeros register and clears upper 32 bits in 64-bit mode)", "CF=0, OF=0, SF, ZF, PF", "xorl %eax, %eax");
    registerOpcode("xorq", "x86-64", "Bitwise XOR 64-bit", "CF=0, OF=0, SF, ZF, PF", "xorq %rax, %rax");
    registerOpcode("and", "x86-64", "Bitwise logical AND", "CF=0, OF=0, SF, ZF, PF", "and dst, src");
    registerOpcode("andl", "x86-64", "Bitwise logical AND 32-bit", "CF=0, OF=0, SF, ZF, PF", "andl %esi, %eax");
    registerOpcode("or", "x86-64", "Bitwise logical OR", "CF=0, OF=0, SF, ZF, PF", "or dst, src");
    registerOpcode("not", "x86-64", "One's complement negation (inverts all bits)", "None", "not dst");

    registerOpcode("cmp", "x86-64", "Compare operands (computes op1 - op2 and updates flags without saving result)", "CF, OF, SF, ZF, AF, PF", "cmp op1, op2");
    registerOpcode("cmpl", "x86-64", "Compare 32-bit operands", "CF, OF, SF, ZF, AF, PF", "cmpl %esi, %edi");
    registerOpcode("cmpq", "x86-64", "Compare 64-bit operands", "CF, OF, SF, ZF, AF, PF", "cmpq %rsi, %rdi");
    registerOpcode("test", "x86-64", "Logical compare (computes op1 AND op2 and sets flags without storing result)", "CF=0, OF=0, SF, ZF, PF", "test op1, op2");
    registerOpcode("testl", "x86-64", "Logical compare 32-bit operands", "CF=0, OF=0, SF, ZF, PF", "testl %eax, %eax");
    registerOpcode("testq", "x86-64", "Logical compare 64-bit operands", "CF=0, OF=0, SF, ZF, PF", "testq %rax, %rax");

    registerOpcode("jmp", "x86-64", "Unconditional direct or indirect jump", "None", "jmp target");
    registerOpcode("je", "x86-64", "Jump if Equal / Jump if Zero (ZF = 1)", "None", "je target");
    registerOpcode("jne", "x86-64", "Jump if Not Equal / Jump if Not Zero (ZF = 0)", "None", "jne target");
    registerOpcode("jg", "x86-64", "Jump if Greater (signed: ZF = 0 and SF = OF)", "None", "jg target");
    registerOpcode("jge", "x86-64", "Jump if Greater or Equal (signed: SF = OF)", "None", "jge target");
    registerOpcode("jl", "x86-64", "Jump if Less (signed: SF != OF)", "None", "jl target");
    registerOpcode("jle", "x86-64", "Jump if Less or Equal (signed: ZF = 1 or SF != OF)", "None", "jle target");
    registerOpcode("ja", "x86-64", "Jump if Above (unsigned: CF = 0 and ZF = 0)", "None", "ja target");
    registerOpcode("jae", "x86-64", "Jump if Above or Equal (unsigned: CF = 0)", "None", "jae target");
    registerOpcode("jb", "x86-64", "Jump if Below (unsigned: CF = 1)", "None", "jb target");
    registerOpcode("jbe", "x86-64", "Jump if Below or Equal (unsigned: CF = 1 or ZF = 1)", "None", "jbe target");

    registerOpcode("sete", "x86-64", "Set byte if Equal / Zero (ZF = 1)", "None", "sete dst8");
    registerOpcode("setne", "x86-64", "Set byte if Not Equal / Not Zero (ZF = 0)", "None", "setne dst8");
    registerOpcode("call", "x86-64", "Call procedure (pushes return RIP and jumps to target)", "None", "call target");
    registerOpcode("callq", "x86-64", "Call procedure 64-bit", "None", "callq target");
    registerOpcode("ret", "x86-64", "Return from procedure (pops return address from stack)", "None", "ret");
    registerOpcode("retq", "x86-64", "Return from 64-bit procedure", "None", "retq");
    registerOpcode("nop", "x86-64", "No Operation", "None", "nop");
    registerOpcode("syscall", "x86-64", "Fast system call into OS kernel", "RCX, R11 modified", "syscall");

    // ==========================================
    // ARM64 (AArch64) Instructions
    // ==========================================
    registerOpcode("ldr", "ARM64", "Load register from memory address", "None", "ldr Xt, [Xn, #offset]");
    registerOpcode("str", "ARM64", "Store register to memory address", "None", "str Xt, [Xn, #offset]");
    registerOpcode("ldp", "ARM64", "Load Pair of registers from consecutive memory locations", "None", "ldp X1, X2, [SP], #16");
    registerOpcode("stp", "ARM64", "Store Pair of registers to consecutive memory locations", "None", "stp X1, X2, [SP, #-16]!");

    registerOpcode("b", "ARM64", "Branch unconditionally to target label", "None", "b target");
    registerOpcode("bl", "ARM64", "Branch with Link (calls function, saves return address in LR/X30)", "None", "bl target");
    registerOpcode("blr", "ARM64", "Branch with Link to Register", "None", "blr Xn");
    registerOpcode("br", "ARM64", "Branch to Register", "None", "br Xn");
    registerOpcode("cbz", "ARM64", "Compare and Branch if Zero (single instruction branch)", "None", "cbz Xt, target");
    registerOpcode("cbnz", "ARM64", "Compare and Branch if Nonzero", "None", "cbnz Xt, target");
    registerOpcode("b.eq", "ARM64", "Branch if condition code Equal (Z = 1)", "None", "b.eq target");
    registerOpcode("b.ne", "ARM64", "Branch if condition code Not Equal (Z = 0)", "None", "b.ne target");
    registerOpcode("b.lt", "ARM64", "Branch if condition code Less Than (N != V)", "None", "b.lt target");
    registerOpcode("b.le", "ARM64", "Branch if condition code Less Than or Equal", "None", "b.le target");
    registerOpcode("b.gt", "ARM64", "Branch if condition code Greater Than", "None", "b.gt target");
    registerOpcode("b.ge", "ARM64", "Branch if condition code Greater Than or Equal", "None", "b.ge target");

    registerOpcode("svc", "ARM64", "Supervisor Call (system call into OS kernel)", "None", "svc #0");
}

void OpcodeReference::registerOpcode(const QString& mnemonic, const QString& arch,
                                     const QString& summary, const QString& flags,
                                     const QString& syntax) {
    OpcodeInfo info;
    info.mnemonic = mnemonic.toLower();
    info.architecture = arch;
    info.summary = summary;
    info.flagsAffected = flags;
    info.syntaxExample = syntax;

    QString archKey = arch.toLower();
    m_archOpcodes[archKey][info.mnemonic] = info;
    m_allOpcodes[info.mnemonic] = info;
}

QStringList OpcodeReference::supportedArchitectures() const {
    return m_archOpcodes.keys();
}

bool OpcodeReference::hasOpcode(const QString& mnemonic, const QString& arch) const {
    QString m = mnemonic.toLower().trimmed();
    if (!arch.isEmpty()) {
        QString a = arch.toLower().trimmed();
        return m_archOpcodes.value(a).contains(m);
    }
    return m_allOpcodes.contains(m);
}

OpcodeInfo OpcodeReference::getOpcodeInfo(const QString& mnemonic, const QString& arch) const {
    QString m = mnemonic.toLower().trimmed();
    if (!arch.isEmpty()) {
        QString a = arch.toLower().trimmed();
        if (m_archOpcodes.contains(a) && m_archOpcodes[a].contains(m)) {
            return m_archOpcodes[a].value(m);
        }
    }
    return m_allOpcodes.value(m);
}

bool OpcodeReference::extractMnemonicAtColumn(const QString& lineText, int column,
                                              QString* outMnemonic, QString* detectedArch) {
    if (column < 0 || column >= lineText.size()) {
        return false;
    }

    // 1. Comments: check if column is inside a comment
    int hashIdx = lineText.indexOf('#');
    int semiIdx = lineText.indexOf(';');
    int slashIdx = lineText.indexOf("//");

    int commentStart = -1;
    if (hashIdx >= 0) commentStart = (commentStart < 0) ? hashIdx : std::min(commentStart, hashIdx);
    if (semiIdx >= 0) commentStart = (commentStart < 0) ? semiIdx : std::min(commentStart, semiIdx);
    if (slashIdx >= 0) commentStart = (commentStart < 0) ? slashIdx : std::min(commentStart, slashIdx);

    if (commentStart >= 0 && column >= commentStart) {
        return false; // Cursor is inside a comment!
    }

    QString codePart = (commentStart >= 0) ? lineText.left(commentStart) : lineText;

    // 2. Directives check: if line starts with directive '.', ignore
    QString trimmed = codePart.trimmed();
    if (trimmed.startsWith('.')) {
        return false;
    }

    // 3. Labels: skip label prefix (e.g. "main:" or ".L12:")
    int colonIdx = codePart.indexOf(':');
    int startSearch = 0;
    if (colonIdx >= 0) {
        if (column <= colonIdx) {
            return false; // Cursor is on the label!
        }
        startSearch = colonIdx + 1;
    }

    // 4. Find the first token after startSearch (the mnemonic)
    int mnemonicStart = -1;
    int mnemonicEnd = -1;

    for (int i = startSearch; i < codePart.size(); ++i) {
        QChar c = codePart[i];
        if (!c.isSpace()) {
            mnemonicStart = i;
            break;
        }
    }

    if (mnemonicStart < 0) {
        return false;
    }

    for (int i = mnemonicStart; i < codePart.size(); ++i) {
        QChar c = codePart[i];
        if (c.isSpace() || c == ',' || c == '\t') {
            mnemonicEnd = i;
            break;
        }
    }
    if (mnemonicEnd < 0) {
        mnemonicEnd = codePart.size();
    }

    // Directives after label (e.g. "label: .byte 0")
    if (codePart[mnemonicStart] == '.') {
        return false;
    }

    // Check if column is strictly within the mnemonic's character range!
    if (column < mnemonicStart || column >= mnemonicEnd) {
        return false; // Column is on operands or whitespace!
    }

    QString candidate = codePart.mid(mnemonicStart, mnemonicEnd - mnemonicStart).toLower();

    if (OpcodeReference::instance().hasOpcode(candidate)) {
        if (outMnemonic) *outMnemonic = candidate;
        if (detectedArch) {
            OpcodeInfo info = OpcodeReference::instance().getOpcodeInfo(candidate);
            *detectedArch = info.architecture;
        }
        return true;
    }

    return false;
}

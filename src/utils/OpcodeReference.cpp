#include "OpcodeReference.h"

OpcodeReference& OpcodeReference::instance() {
    static OpcodeReference inst;
    return inst;
}

OpcodeReference::OpcodeReference() {
    // x86 / x86-64 Instructions
    registerOpcode("mov", "x86", "Move data between registers or memory", "None", "mov dst, src");
    registerOpcode("movq", "x86-64", "Move 64-bit quadword data", "None", "movq dst, src");
    registerOpcode("movl", "x86", "Move 32-bit doubleword data", "None", "movl dst, src");
    registerOpcode("lea", "x86", "Load Effective Address (computes memory offset without dereferencing)", "None", "lea dst, [base + index*scale + disp]");
    registerOpcode("push", "x86", "Push word, doubleword or quadword onto stack", "None", "push src");
    registerOpcode("pushq", "x86-64", "Push 64-bit quadword onto stack", "None", "pushq src");
    registerOpcode("pop", "x86", "Pop value from stack into register or memory", "None", "pop dst");
    registerOpcode("popq", "x86-64", "Pop 64-bit value from stack into register", "None", "popq dst");
    registerOpcode("add", "x86", "Add two operands", "CF, OF, SF, ZF, AF, PF", "add dst, src");
    registerOpcode("addl", "x86", "Add 32-bit values", "CF, OF, SF, ZF, AF, PF", "addl dst, src");
    registerOpcode("addq", "x86-64", "Add 64-bit values", "CF, OF, SF, ZF, AF, PF", "addq dst, src");
    registerOpcode("sub", "x86", "Subtract source from destination", "CF, OF, SF, ZF, AF, PF", "sub dst, src");
    registerOpcode("subq", "x86-64", "Subtract 64-bit value", "CF, OF, SF, ZF, AF, PF", "subq dst, src");
    registerOpcode("imul", "x86", "Signed integer multiply", "CF, OF (SF, ZF, AF, PF undefined)", "imul dst, src");
    registerOpcode("idiv", "x86", "Signed divide", "Flags undefined", "idiv src");
    registerOpcode("xor", "x86", "Bitwise exclusive OR (frequently used to zero registers)", "CF=0, OF=0, SF, ZF, PF", "xor dst, src");
    registerOpcode("xorl", "x86", "Bitwise XOR 32-bit (zeros register and clears upper 32-bits in 64-bit mode)", "CF=0, OF=0, SF, ZF, PF", "xorl %eax, %eax");
    registerOpcode("and", "x86", "Bitwise logical AND", "CF=0, OF=0, SF, ZF, PF", "and dst, src");
    registerOpcode("or", "x86", "Bitwise logical OR", "CF=0, OF=0, SF, ZF, PF", "or dst, src");
    registerOpcode("cmp", "x86", "Compare two operands (computes dst - src and sets flags)", "CF, OF, SF, ZF, AF, PF", "cmp op1, op2");
    registerOpcode("cmpl", "x86", "Compare 32-bit operands", "CF, OF, SF, ZF, AF, PF", "cmpl op1, op2");
    registerOpcode("test", "x86", "Logical compare (computes op1 AND op2 and sets flags without saving result)", "CF=0, OF=0, SF, ZF, PF", "test op1, op2");
    registerOpcode("jmp", "x86", "Unconditional jump to target address/label", "None", "jmp target");
    registerOpcode("je", "x86", "Jump if Equal (ZF = 1)", "None", "je target");
    registerOpcode("jne", "x86", "Jump if Not Equal (ZF = 0)", "None", "jne target");
    registerOpcode("jg", "x86", "Jump if Greater (signed: ZF = 0 and SF = OF)", "None", "jg target");
    registerOpcode("jge", "x86", "Jump if Greater or Equal (signed: SF = OF)", "None", "jge target");
    registerOpcode("jl", "x86", "Jump if Less (signed: SF != OF)", "None", "jl target");
    registerOpcode("jle", "x86", "Jump if Less or Equal (signed: ZF = 1 or SF != OF)", "None", "jle target");
    registerOpcode("call", "x86", "Call procedure (pushes return address and jumps to target)", "None", "call target");
    registerOpcode("ret", "x86", "Return from procedure (pops return address into RIP/EIP)", "None", "ret");
    registerOpcode("retq", "x86-64", "Return from 64-bit procedure", "None", "retq");
    registerOpcode("nop", "x86", "No Operation (one-byte delay instruction)", "None", "nop");
    registerOpcode("syscall", "x86-64", "Fast System Call into OS kernel", "RCX, R11 modified", "syscall");

    // ARM64 Instructions
    registerOpcode("ldr", "ARM64", "Load register from memory", "None", "ldr Xt, [Xn, #offset]");
    registerOpcode("str", "ARM64", "Store register to memory", "None", "str Xt, [Xn, #offset]");
    registerOpcode("stp", "ARM64", "Store Pair of registers to memory (commonly in function prologue)", "None", "stp X1, X2, [SP, #-16]!");
    registerOpcode("ldp", "ARM64", "Load Pair of registers from memory (commonly in function epilogue)", "None", "ldp X1, X2, [SP], #16");
    registerOpcode("b", "ARM64", "Branch unconditionally to target label", "None", "b target");
    registerOpcode("bl", "ARM64", "Branch with Link (calls subroutine, stores return address in LR/X30)", "None", "bl target");
    registerOpcode("cbz", "ARM64", "Compare and Branch if Zero", "None", "cbz Xt, target");
    registerOpcode("cbnz", "ARM64", "Compare and Branch if Nonzero", "None", "cbnz Xt, target");
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
    m_opcodes[info.mnemonic] = info;
}

bool OpcodeReference::hasOpcode(const QString& mnemonic) const {
    return m_opcodes.contains(mnemonic.toLower().trimmed());
}

OpcodeInfo OpcodeReference::getOpcodeInfo(const QString& mnemonic) const {
    return m_opcodes.value(mnemonic.toLower().trimmed());
}

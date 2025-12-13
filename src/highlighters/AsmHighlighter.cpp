# 1 "./src/highlighters/AsmHighlighter.cpp"
#include "AsmHighlighter.h"

AsmHighlighter::AsmHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    m_instructionFormat.setForeground(QColor("#569CD6"));
    m_instructionFormat.setFontWeight(QFont::Bold);

    QStringList instructions = {
        "mov",   "movzx",  "movsx", "movsxd", "movabs",  "push",   "pop",     "call",   "ret",
        "leave", "jmp",    "je",    "jne",    "jz",      "jnz",    "jg",      "jge",    "jl",
        "jle",   "ja",     "jae",   "jb",     "jbe",     "js",     "jns",     "cmp",    "test",
        "add",   "sub",    "mul",   "imul",   "div",     "idiv",   "inc",     "dec",    "neg",
        "and",   "or",     "xor",   "not",    "shl",     "shr",    "sal",     "sar",    "rol",
        "ror",   "lea",    "nop",   "int",    "syscall", "cdq",    "cqo",     "cbw",    "cwde",
        "cdqe",  "rep",    "repe",  "repne",  "repz",    "repnz",  "movs",    "movsb",  "movsw",
        "movsd", "movsq",  "stos",  "stosb",  "stosw",   "stosd",  "stosq",   "lods",   "lodsb",
        "lodsw", "lodsd",  "lodsq", "cmps",   "cmpsb",   "cmpsw",  "cmpsd",   "cmpsq",  "scas",
        "scasb", "scasw",  "scasd", "scasq",  "sete",    "setne",  "setg",    "setge",  "setl",
        "setle", "seta",   "setae", "setb",   "setbe",   "cmove",  "cmovne",  "cmovg",  "cmovge",
        "cmovl", "cmovle", "cmova", "cmovae", "cmovb",   "cmovbe", "endbr64", "endbr32"};

    QString instructionPattern = "\\b(" + instructions.join("|") + ")\\b";
    rule.pattern =
        QRegularExpression(instructionPattern, QRegularExpression::CaseInsensitiveOption);
    rule.format = m_instructionFormat;
    m_rules.append(rule);

    m_registerFormat.setForeground(QColor("#C586C0"));

    QStringList registers = {

        "rax",    "rbx",    "rcx",   "rdx",   "rsi",   "rdi",   "rbp",   "rsp",   "r8",    "r9",
        "r10",    "r11",    "r12",   "r13",   "r14",   "r15",

        "eax",    "ebx",    "ecx",   "edx",   "esi",   "edi",   "ebp",   "esp",   "r8d",   "r9d",
        "r10d",   "r11d",   "r12d",  "r13d",  "r14d",  "r15d",

        "ax",     "bx",     "cx",    "dx",    "si",    "di",    "bp",    "sp",    "r8w",   "r9w",
        "r10w",   "r11w",   "r12w",  "r13w",  "r14w",  "r15w",

        "al",     "bl",     "cl",    "dl",    "sil",   "dil",   "bpl",   "spl",   "ah",    "bh",
        "ch",     "dh",     "r8b",   "r9b",   "r10b",  "r11b",  "r12b",  "r13b",  "r14b",  "r15b",

        "cs",     "ds",     "es",    "fs",    "gs",    "ss",

        "rip",    "eip",    "ip",

        "rflags", "eflags", "flags",

        "xmm0",   "xmm1",   "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",  "xmm8",  "xmm9",
        "xmm10",  "xmm11",  "xmm12", "xmm13", "xmm14", "xmm15", "ymm0",  "ymm1",  "ymm2",  "ymm3",
        "ymm4",   "ymm5",   "ymm6",  "ymm7",  "ymm8",  "ymm9",  "ymm10", "ymm11", "ymm12", "ymm13",
        "ymm14",  "ymm15"};

    QString registerPattern = "\\b(" + registers.join("|") + ")\\b";
    rule.pattern = QRegularExpression(registerPattern, QRegularExpression::CaseInsensitiveOption);
    rule.format = m_registerFormat;
    m_rules.append(rule);

    m_numberFormat.setForeground(QColor("#CE9178"));

    rule.pattern = QRegularExpression("\\b(0x[0-9a-fA-F]+|\\d+)\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    m_labelFormat.setForeground(QColor("#6A9955"));
    m_labelFormat.setFontWeight(QFont::Bold);

    rule.pattern = QRegularExpression("^\\s*[._a-zA-Z][._a-zA-Z0-9]*:");
    rule.format = m_labelFormat;
    m_rules.append(rule);

    m_commentFormat.setForeground(QColor("#6A9955"));
    m_commentFormat.setFontItalic(true);

    rule.pattern = QRegularExpression("[#;].*$");
    rule.format = m_commentFormat;
    m_rules.append(rule);
}

void AsmHighlighter::highlightBlock(const QString &text) {
    for (const HighlightingRule &rule : qAsConst(m_rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

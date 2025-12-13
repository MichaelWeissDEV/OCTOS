# 1 "./src/highlighters/CppHighlighter.cpp"
#include "CppHighlighter.h"

CppHighlighter::CppHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    m_keywordFormat.setForeground(QColor("#569CD6"));
    m_keywordFormat.setFontWeight(QFont::Bold);

    QStringList keywords = {"asm",
                            "auto",
                            "bool",
                            "break",
                            "case",
                            "catch",
                            "char",
                            "class",
                            "const",
                            "constexpr",
                            "continue",
                            "default",
                            "delete",
                            "do",
                            "double",
                            "dynamic_cast",
                            "else",
                            "enum",
                            "explicit",
                            "export",
                            "extern",
                            "false",
                            "float",
                            "for",
                            "friend",
                            "goto",
                            "if",
                            "inline",
                            "int",
                            "long",
                            "mutable",
                            "namespace",
                            "new",
                            "noexcept",
                            "nullptr",
                            "operator",
                            "private",
                            "protected",
                            "public",
                            "register",
                            "reinterpret_cast",
                            "return",
                            "short",
                            "signed",
                            "sizeof",
                            "static",
                            "static_cast",
                            "struct",
                            "switch",
                            "template",
                            "this",
                            "throw",
                            "true",
                            "try",
                            "typedef",
                            "typeid",
                            "typename",
                            "union",
                            "unsigned",
                            "using",
                            "virtual",
                            "void",
                            "volatile",
                            "wchar_t",
                            "while"};

    for (const QString& keyword : keywords) {
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    m_typeFormat.setForeground(QColor("#4EC9B0"));

    QStringList types = {"std",      "vector",  "string", "map",        "set",
                         "queue",    "stack",   "pair",   "unique_ptr", "shared_ptr",
                         "optional", "variant", "array",  "deque"};

    for (const QString& type : types) {
        rule.pattern = QRegularExpression("\\b" + type + "\\b");
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    m_stringFormat.setForeground(QColor("#CE9178"));
    rule.pattern = QRegularExpression("\"(?:[^\\\\\"]|\\\\.)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("'(?:[^\\\\']|\\\\.)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    m_numberFormat.setForeground(QColor("#B5CEA8"));
    rule.pattern = QRegularExpression("\\b(0x[0-9a-fA-F]+|\\d+\\.?\\d*|\\d+)\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    m_commentFormat.setForeground(QColor("#6A9955"));
    m_commentFormat.setFontItalic(true);

    rule.pattern = QRegularExpression("//.*$");
    rule.format = m_commentFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("/\\*.*?\\*/");
    rule.pattern.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
    rule.format = m_commentFormat;
    m_rules.append(rule);

    m_functionFormat.setForeground(QColor("#DCDCAA"));
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);

    m_preprocessorFormat.setForeground(QColor("#9CDCFE"));
    m_preprocessorFormat.setFontWeight(QFont::Bold);

    rule.pattern = QRegularExpression(
        "^\\s*#\\s*(?:include|define|ifdef|ifndef|endif|if|else|elif|pragma).*$");
    rule.format = m_preprocessorFormat;
    m_rules.append(rule);
}

void CppHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : qAsConst(m_rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    QRegularExpression startExpression("/\\*");
    QRegularExpression endExpression("\\*/");

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(startExpression);
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = endExpression.match(text, startIndex);
        int endIndex = endMatch.capturedStart();
        int commentLength;

        if (endIndex == -1) {
            this->setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, m_commentFormat);
        startIndex = text.indexOf(startExpression, startIndex + commentLength);
    }
}

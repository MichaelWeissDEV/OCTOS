#include "CppHighlighter.h"
#include "../ui/Theme.h"

CppHighlighter::CppHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
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
    rule.role = SyntaxRole::Keyword;
        m_rules.append(rule);
    }

    m_typeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));

    QStringList types = {"std",      "vector",  "string", "map",        "set",
                         "queue",    "stack",   "pair",   "unique_ptr", "shared_ptr",
                         "optional", "variant", "array",  "deque"};

    for (const QString& type : types) {
        rule.pattern = QRegularExpression("\\b" + type + "\\b");
        rule.format = m_typeFormat;
    rule.role = SyntaxRole::Type;
        m_rules.append(rule);
    }

    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    rule.pattern = QRegularExpression("\"(?:[^\\\\\"]|\\\\.)*\"");
    rule.format = m_stringFormat;
    rule.role = SyntaxRole::String;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("'(?:[^\\\\']|\\\\.)*'");
    rule.format = m_stringFormat;
    rule.role = SyntaxRole::String;
    m_rules.append(rule);

    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    rule.pattern = QRegularExpression("\\b(0x[0-9a-fA-F]+|\\d+\\.?\\d*|\\d+)\\b");
    rule.format = m_numberFormat;
    rule.role = SyntaxRole::Number;
    m_rules.append(rule);

    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
    m_commentFormat.setFontItalic(true);

    rule.pattern = QRegularExpression("//.*$");
    rule.format = m_commentFormat;
    rule.role = SyntaxRole::Comment;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("/\\*.*?\\*/");
    rule.pattern.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
    rule.format = m_commentFormat;
    rule.role = SyntaxRole::Comment;
    m_rules.append(rule);

    m_functionFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\()");
    rule.format = m_functionFormat;
    rule.role = SyntaxRole::Function;
    m_rules.append(rule);

    m_preprocessorFormat.setForeground(Theme::syntaxColor(SyntaxRole::Preprocessor));
    m_preprocessorFormat.setFontWeight(QFont::Bold);

    rule.pattern = QRegularExpression(
        "^\\s*#\\s*(?:include|define|ifdef|ifndef|endif|if|else|elif|pragma).*$");
    rule.format = m_preprocessorFormat;
    rule.role = SyntaxRole::Preprocessor;
    m_rules.append(rule);
}

#include <utility>

void CppHighlighter::recolorFormats() {
    for (auto& rule : m_rules) rule.format.setForeground(Theme::syntaxColor(rule.role));
    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
    m_typeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));
    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
    m_functionFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    m_preprocessorFormat.setForeground(Theme::syntaxColor(SyntaxRole::Preprocessor));
}

void CppHighlighter::applyTheme() {
    for (auto& rule : m_rules) rule.format.setForeground(Theme::syntaxColor(rule.role));
    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
    m_typeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));
    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
    m_functionFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    m_preprocessorFormat.setForeground(Theme::syntaxColor(SyntaxRole::Preprocessor));
    m_themeGeneration = Theme::generation();
    recolorFormats();
    rehighlight();
}

void CppHighlighter::highlightBlock(const QString& text) {
    if (m_themeGeneration != Theme::generation()) { m_themeGeneration = Theme::generation(); recolorFormats(); }
    for (const HighlightingRule& rule : std::as_const(m_rules)) {
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

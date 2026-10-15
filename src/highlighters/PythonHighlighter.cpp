#include "PythonHighlighter.h"
#include "../ui/Theme.h"
#include <utility>

PythonHighlighter::PythonHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    // Keywords
    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
    m_keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        "\\band\\b", "\\bas\\b", "\\bassert\\b", "\\basync\\b", "\\bawait\\b",
        "\\bbreak\\b", "\\bclass\\b", "\\bcontinue\\b", "\\bdef\\b", "\\bdel\\b",
        "\\belif\\b", "\\belse\\b", "\\bexcept\\b", "\\bfinally\\b", "\\bfor\\b",
        "\\bfrom\\b", "\\bglobal\\b", "\\bif\\b", "\\bimport\\b", "\\bin\\b",
        "\\bis\\b", "\\blambda\\b", "\\bnonlocal\\b", "\\bnot\\b", "\\bor\\b",
        "\\bpass\\b", "\\braise\\b", "\\breturn\\b", "\\btry\\b", "\\bwhile\\b",
        "\\bwith\\b", "\\byield\\b", "\\bTrue\\b", "\\bFalse\\b", "\\bNone\\b"
    };
    for (const QString& pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
    rule.role = SyntaxRole::Keyword;
        m_rules.append(rule);
    }

    // Builtins
    m_builtinFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));
    const QString builtinPatterns[] = {
        "\\bprint\\b", "\\blen\\b", "\\brange\\b", "\\bint\\b", "\\bfloat\\b",
        "\\bstr\\b", "\\blist\\b", "\\bdict\\b", "\\bset\\b", "\\btuple\\b",
        "\\bbool\\b", "\\btype\\b", "\\bisinstance\\b", "\\bopen\\b", "\\bzip\\b",
        "\\bmap\\b", "\\bfilter\\b", "\\bsorted\\b", "\\benumerate\\b", "\\bsuper\\b"
    };
    for (const QString& pattern : builtinPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_builtinFormat;
    rule.role = SyntaxRole::Type;
        m_rules.append(rule);
    }

    // Decorators (@classmethod, @staticmethod, etc.)
    m_decoratorFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    rule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_.]*");
    rule.format = m_decoratorFormat;
    rule.role = SyntaxRole::Function;
    m_rules.append(rule);

    // Numbers
    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    rule.pattern = QRegularExpression("\\b(?:0[xX][0-9a-fA-F]+|0[bB][01]+|0[oO][0-7]+|[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)\\b");
    rule.format = m_numberFormat;
    rule.role = SyntaxRole::Number;
    m_rules.append(rule);

    // Strings
    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    rule.pattern = QRegularExpression(R"([fFrRbBuU]?"(?:[^"\\]|\\.)*"|[fFrRbBuU]?'(?:[^'\\]|\\.)*')");
    rule.format = m_stringFormat;
    rule.role = SyntaxRole::String;
    m_rules.append(rule);

    // Comments
    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
    m_commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("#.*$");
    rule.format = m_commentFormat;
    rule.role = SyntaxRole::Comment;
    m_rules.append(rule);
}

void PythonHighlighter::recolorFormats() {
    for (auto& rule : m_rules) rule.format.setForeground(Theme::syntaxColor(rule.role));
    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
    m_builtinFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));
    m_decoratorFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
}

void PythonHighlighter::applyTheme() {
    for (auto& rule : m_rules) rule.format.setForeground(Theme::syntaxColor(rule.role));
    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
    m_builtinFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));
    m_decoratorFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
    m_themeGeneration = Theme::generation();
    recolorFormats();
    rehighlight();
}

void PythonHighlighter::highlightBlock(const QString& text) {
    if (m_themeGeneration != Theme::generation()) { m_themeGeneration = Theme::generation(); recolorFormats(); }
    for (const HighlightingRule& rule : std::as_const(m_rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

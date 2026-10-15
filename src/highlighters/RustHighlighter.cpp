#include "RustHighlighter.h"
#include "../ui/Theme.h"
#include <utility>

RustHighlighter::RustHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    // Keywords
    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
    m_keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        "\\basync\\b", "\\bawait\\b", "\\bas\\b", "\\bbreak\\b", "\\bconst\\b",
        "\\bcontinue\\b", "\\bcrate\\b", "\\bdyn\\b", "\\belse\\b", "\\benum\\b",
        "\\bextern\\b", "\\bfalse\\b", "\\bfn\\b", "\\bfor\\b", "\\bif\\b",
        "\\bimpl\\b", "\\bin\\b", "\\blet\\b", "\\bloop\\b", "\\bmatch\\b",
        "\\bmod\\b", "\\bmove\\b", "\\bmut\\b", "\\bpub\\b", "\\bref\\b",
        "\\breturn\\b", "\\bself\\b", "\\bSelf\\b", "\\bstatic\\b", "\\bstruct\\b",
        "\\bsuper\\b", "\\btrait\\b", "\\btrue\\b", "\\btype\\b", "\\bunsafe\\b",
        "\\buse\\b", "\\bwhere\\b", "\\bwhile\\b"
    };
    for (const QString& pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
    rule.role = SyntaxRole::Keyword;
        m_rules.append(rule);
    }

    // Built-in types
    m_typeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));
    const QString typePatterns[] = {
        "\\bi8\\b", "\\bi16\\b", "\\bi32\\b", "\\bi64\\b", "\\bi128\\b", "\\bisize\\b",
        "\\bu8\\b", "\\bu16\\b", "\\bu32\\b", "\\bu64\\b", "\\bu128\\b", "\\busize\\b",
        "\\bf32\\b", "\\bf64\\b", "\\bbool\\b", "\\bchar\\b", "\\bstr\\b",
        "\\bString\\b", "\\bVec\\b", "\\bOption\\b", "\\bResult\\b", "\\bBox\\b",
        "\\bRc\\b", "\\bArc\\b"
    };
    for (const QString& pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_typeFormat;
    rule.role = SyntaxRole::Type;
        m_rules.append(rule);
    }

    // Numbers
    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    rule.pattern = QRegularExpression("\\b(?:0x[0-9a-fA-F_]+|0b[01_]+|0o[0-7_]+|[0-9][0-9_]*(?:\\.[0-9_]+)?(?:[eE][+-]?[0-9_]+)?(?:[iuf](?:8|16|32|64|128|size))?)\\b");
    rule.format = m_numberFormat;
    rule.role = SyntaxRole::Number;
    m_rules.append(rule);

    // Macros (e.g. println!, vec!, format!)
    m_macroFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*!");
    rule.format = m_macroFormat;
    rule.role = SyntaxRole::Function;
    m_rules.append(rule);

    // Attributes (e.g. #[inline], #![no_std])
    m_attributeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Register));
    rule.pattern = QRegularExpression("#!?\\[[^\\]]*\\]");
    rule.format = m_attributeFormat;
    rule.role = SyntaxRole::Register;
    m_rules.append(rule);

    // Strings
    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    rule.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*")");
    rule.format = m_stringFormat;
    rule.role = SyntaxRole::String;
    m_rules.append(rule);

    // Single line comments
    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
    m_commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("//.*$");
    rule.format = m_commentFormat;
    rule.role = SyntaxRole::Comment;
    m_rules.append(rule);
}

void RustHighlighter::recolorFormats() {
    for (auto& rule : m_rules) rule.format.setForeground(Theme::syntaxColor(rule.role));
    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
    m_typeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));
    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    m_macroFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    m_attributeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Register));
    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
}

void RustHighlighter::applyTheme() {
    for (auto& rule : m_rules) rule.format.setForeground(Theme::syntaxColor(rule.role));
    m_keywordFormat.setForeground(Theme::syntaxColor(SyntaxRole::Keyword));
    m_typeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Type));
    m_numberFormat.setForeground(Theme::syntaxColor(SyntaxRole::Number));
    m_macroFormat.setForeground(Theme::syntaxColor(SyntaxRole::Function));
    m_attributeFormat.setForeground(Theme::syntaxColor(SyntaxRole::Register));
    m_stringFormat.setForeground(Theme::syntaxColor(SyntaxRole::String));
    m_commentFormat.setForeground(Theme::syntaxColor(SyntaxRole::Comment));
    m_themeGeneration = Theme::generation();
    recolorFormats();
    rehighlight();
}

void RustHighlighter::highlightBlock(const QString& text) {
    if (m_themeGeneration != Theme::generation()) { m_themeGeneration = Theme::generation(); recolorFormats(); }
    for (const HighlightingRule& rule : std::as_const(m_rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

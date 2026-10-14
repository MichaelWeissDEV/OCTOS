#include "PythonHighlighter.h"
#include <utility>

PythonHighlighter::PythonHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    // Keywords
    m_keywordFormat.setForeground(QColor("#c586c0"));
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
        m_rules.append(rule);
    }

    // Builtins
    m_builtinFormat.setForeground(QColor("#4ec9b0"));
    const QString builtinPatterns[] = {
        "\\bprint\\b", "\\blen\\b", "\\brange\\b", "\\bint\\b", "\\bfloat\\b",
        "\\bstr\\b", "\\blist\\b", "\\bdict\\b", "\\bset\\b", "\\btuple\\b",
        "\\bbool\\b", "\\btype\\b", "\\bisinstance\\b", "\\bopen\\b", "\\bzip\\b",
        "\\bmap\\b", "\\bfilter\\b", "\\bsorted\\b", "\\benumerate\\b", "\\bsuper\\b"
    };
    for (const QString& pattern : builtinPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_builtinFormat;
        m_rules.append(rule);
    }

    // Decorators (@classmethod, @staticmethod, etc.)
    m_decoratorFormat.setForeground(QColor("#dcdcaa"));
    rule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_.]*");
    rule.format = m_decoratorFormat;
    m_rules.append(rule);

    // Numbers
    m_numberFormat.setForeground(QColor("#b5cea8"));
    rule.pattern = QRegularExpression("\\b(?:0[xX][0-9a-fA-F]+|0[bB][01]+|0[oO][0-7]+|[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    m_stringFormat.setForeground(QColor("#ce9178"));
    rule.pattern = QRegularExpression(R"([fFrRbBuU]?"(?:[^"\\]|\\.)*"|[fFrRbBuU]?'(?:[^'\\]|\\.)*')");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    m_commentFormat.setForeground(QColor("#6a9955"));
    m_commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("#.*$");
    rule.format = m_commentFormat;
    m_rules.append(rule);
}

void PythonHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : std::as_const(m_rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

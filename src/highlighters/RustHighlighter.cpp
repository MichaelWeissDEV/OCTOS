#include "RustHighlighter.h"
#include <utility>

RustHighlighter::RustHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    // Keywords
    m_keywordFormat.setForeground(QColor("#569cd6"));
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
        m_rules.append(rule);
    }

    // Built-in types
    m_typeFormat.setForeground(QColor("#4ec9b0"));
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
        m_rules.append(rule);
    }

    // Numbers
    m_numberFormat.setForeground(QColor("#b5cea8"));
    rule.pattern = QRegularExpression("\\b(?:0x[0-9a-fA-F_]+|0b[01_]+|0o[0-7_]+|[0-9][0-9_]*(?:\\.[0-9_]+)?(?:[eE][+-]?[0-9_]+)?(?:[iuf](?:8|16|32|64|128|size))?)\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Macros (e.g. println!, vec!, format!)
    m_macroFormat.setForeground(QColor("#dcdcaa"));
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*!");
    rule.format = m_macroFormat;
    m_rules.append(rule);

    // Attributes (e.g. #[inline], #![no_std])
    m_attributeFormat.setForeground(QColor("#c586c0"));
    rule.pattern = QRegularExpression("#!?\\[[^\\]]*\\]");
    rule.format = m_attributeFormat;
    m_rules.append(rule);

    // Strings
    m_stringFormat.setForeground(QColor("#ce9178"));
    rule.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*")");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Single line comments
    m_commentFormat.setForeground(QColor("#6a9955"));
    m_commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("//.*$");
    rule.format = m_commentFormat;
    m_rules.append(rule);
}

void RustHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : std::as_const(m_rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

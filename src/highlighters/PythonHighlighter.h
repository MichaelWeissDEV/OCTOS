#ifndef PYTHONHIGHLIGHTER_H
#define PYTHONHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include "../ui/Theme.h"
#include <QVector>

class PythonHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit PythonHighlighter(QTextDocument* parent = nullptr);
    // Re-reads the theme colors and rehighlights the document.
    void applyTheme();

protected:
    void highlightBlock(const QString& text) override;

private:
    void recolorFormats();
    int m_themeGeneration{-1};
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
        SyntaxRole role{SyntaxRole::Keyword};
    };
    QVector<HighlightingRule> m_rules;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_builtinFormat;
    QTextCharFormat m_decoratorFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_numberFormat;
};

#endif // PYTHONHIGHLIGHTER_H

#ifndef ASMHIGHLIGHTER_H
#define ASMHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include "../ui/Theme.h"
#include <QVector>

class AsmHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

   public:
    explicit AsmHighlighter(QTextDocument *parent = nullptr);
    // Re-reads the theme colors and rehighlights the document.
    void applyTheme();

   protected:
    void highlightBlock(const QString &text) override;

   private:
    void recolorFormats();
    int m_themeGeneration{-1};
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
        SyntaxRole role{SyntaxRole::Keyword};
    };

    QVector<HighlightingRule> m_rules;

    QTextCharFormat m_instructionFormat;
    QTextCharFormat m_registerFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_labelFormat;
    QTextCharFormat m_commentFormat;
};

#endif

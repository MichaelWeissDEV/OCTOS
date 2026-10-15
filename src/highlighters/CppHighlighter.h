#ifndef CPPHIGHLIGHTER_H
#define CPPHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include "../ui/Theme.h"
#include <QVector>

class CppHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

   public:
    explicit CppHighlighter(QTextDocument* parent = nullptr);
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
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_preprocessorFormat;
};

#endif

# 1 "./src/highlighters/CppHighlighter.h"
#ifndef CPPHIGHLIGHTER_H
#define CPPHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

class CppHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

   public:
    explicit CppHighlighter(QTextDocument* parent = nullptr);

   protected:
    void highlightBlock(const QString& text) override;

   private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
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

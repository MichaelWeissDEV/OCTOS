# 1 "./src/highlighters/AsmHighlighter.h"
#ifndef ASMHIGHLIGHTER_H
#define ASMHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

class AsmHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

   public:
    explicit AsmHighlighter(QTextDocument *parent = nullptr);

   protected:
    void highlightBlock(const QString &text) override;

   private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> m_rules;

    QTextCharFormat m_instructionFormat;
    QTextCharFormat m_registerFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_labelFormat;
    QTextCharFormat m_commentFormat;
};

#endif

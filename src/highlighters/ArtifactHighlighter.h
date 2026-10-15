#ifndef ARTIFACTHIGHLIGHTER_H
#define ARTIFACTHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

#include "../core/CompilationArtifact.h"
#include "../ui/Theme.h"

// Highlighter for the non-native artifact types: LLVM IR, JVM bytecode (javap), CPython bytecode (dis), CIL.
// Colors come from the theme tokens and follow theme changes.
class ArtifactHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    ArtifactHighlighter(QTextDocument* parent, ArtifactType type);

    ArtifactType artifactType() const { return m_type; }
    void applyTheme();

protected:
    void highlightBlock(const QString& text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        SyntaxRole role{SyntaxRole::Keyword};
        bool bold{false};
        bool italic{false};
        int captureGroup{0};
    };

    void addRule(const QString& pattern, SyntaxRole role, bool bold = false, bool italic = false,
                 int group = 0, QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption);
    void buildRules();

    ArtifactType m_type;
    QVector<Rule> m_rules;
    int m_themeGeneration{-1};
};

#endif // ARTIFACTHIGHLIGHTER_H

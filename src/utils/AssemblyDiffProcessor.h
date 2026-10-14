#ifndef ASSEMBLYDIFFPROCESSOR_H
#define ASSEMBLYDIFFPROCESSOR_H

#include <functional>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

enum class DiffLineType {
    Unchanged,
    Added,
    Removed,
    Modified
};

struct DiffEntry {
    DiffLineType type{DiffLineType::Unchanged};
    QString textA;
    QString textB;
    int lineIndexA{-1}; // 0-indexed, -1 if not applicable
    int lineIndexB{-1}; // 0-indexed, -1 if not applicable
};

struct DiffOptions {
    bool ignoreWhitespace{false};
    bool ignoreComments{false};
    bool normalizeLabels{false};
    int maxDiffSteps{20000000};
    int maxLines{100000};
};

struct DiffResult {
    QVector<DiffEntry> entries;
    int additions{0};
    int deletions{0};
    int modifications{0};
    int unchanged{0};
    bool isTruncated{false};
    bool isCancelled{false};
    qint64 durationMs{0};
    QString statusMessage;

    bool hasDifferences() const {
        return additions > 0 || deletions > 0 || modifications > 0;
    }
};

class AssemblyDiffProcessor {
public:
    static DiffResult computeDiff(const QString& assemblyA, const QString& assemblyB,
                                 const DiffOptions& options = DiffOptions(),
                                 const std::function<bool()>& isCancelled = nullptr);

    static QString toUnifiedDiff(const DiffResult& result,
                                 const QString& labelA = "Pane A",
                                 const QString& labelB = "Pane B");

    static QString normalizeAssemblyLine(const QString& line, const DiffOptions& options);
};

#endif // ASSEMBLYDIFFPROCESSOR_H

#ifndef ASSEMBLYDIFFPROCESSOR_H
#define ASSEMBLYDIFFPROCESSOR_H

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
    int lineIndexA{-1};
    int lineIndexB{-1};
};

struct DiffResult {
    QVector<DiffEntry> entries;
    int additions{0};
    int deletions{0};
    int modifications{0};
    int unchanged{0};

    bool hasDifferences() const {
        return additions > 0 || deletions > 0 || modifications > 0;
    }
};

class AssemblyDiffProcessor {
public:
    static DiffResult computeDiff(const QString& assemblyA, const QString& assemblyB);
};

#endif // ASSEMBLYDIFFPROCESSOR_H

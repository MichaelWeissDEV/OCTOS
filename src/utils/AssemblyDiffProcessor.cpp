#include "AssemblyDiffProcessor.h"
#include <QVector>

DiffResult AssemblyDiffProcessor::computeDiff(const QString& assemblyA, const QString& assemblyB) {
    DiffResult result;
    QStringList linesA = assemblyA.split('\n');
    QStringList linesB = assemblyB.split('\n');

    int n = linesA.size();
    int m = linesB.size();

    // Standard Longest Common Subsequence (LCS) matrix
    QVector<QVector<int>> dp(n + 1, QVector<int>(m + 1, 0));

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (linesA[i - 1].trimmed() == linesB[j - 1].trimmed()) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = qMax(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // Backtrack to build the diff entries
    QVector<DiffEntry> reversedEntries;
    int i = n;
    int j = m;

    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && linesA[i - 1].trimmed() == linesB[j - 1].trimmed()) {
            DiffEntry entry;
            entry.type = DiffLineType::Unchanged;
            entry.textA = linesA[i - 1];
            entry.textB = linesB[j - 1];
            entry.lineIndexA = i - 1;
            entry.lineIndexB = j - 1;
            reversedEntries.append(entry);
            result.unchanged++;
            --i;
            --j;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            DiffEntry entry;
            entry.type = DiffLineType::Added;
            entry.textA = "";
            entry.textB = linesB[j - 1];
            entry.lineIndexA = -1;
            entry.lineIndexB = j - 1;
            reversedEntries.append(entry);
            result.additions++;
            --j;
        } else if (i > 0 && (j == 0 || dp[i][j - 1] < dp[i - 1][j])) {
            DiffEntry entry;
            entry.type = DiffLineType::Removed;
            entry.textA = linesA[i - 1];
            entry.textB = "";
            entry.lineIndexA = i - 1;
            entry.lineIndexB = -1;
            reversedEntries.append(entry);
            result.deletions++;
            --i;
        }
    }

    // Reverse to get correct order
    for (int k = reversedEntries.size() - 1; k >= 0; --k) {
        result.entries.append(reversedEntries[k]);
    }

    return result;
}

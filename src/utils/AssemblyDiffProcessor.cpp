#include "AssemblyDiffProcessor.h"
#include <QElapsedTimer>
#include <QHash>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>

namespace {

struct LineToken {
    QString raw;
    int id{0};
    int originalIndex{0};
};

// String Interner / Line Hasher
class LineInterner {
public:
    int intern(const QString& line) {
        auto it = m_map.constFind(line);
        if (it != m_map.constEnd()) {
            return it.value();
        }
        int newId = ++m_nextId;
        m_map.insert(line, newId);
        return newId;
    }

private:
    QHash<QString, int> m_map;
    int m_nextId{0};
};

} // namespace

QString AssemblyDiffProcessor::normalizeAssemblyLine(const QString& line, const DiffOptions& options) {
    QString res = line;

    if (options.ignoreComments) {
        // Strip comments starting with # or ; or //
        int hashIdx = res.indexOf('#');
        int semiIdx = res.indexOf(';');
        int slashIdx = res.indexOf("//");

        int cutIdx = -1;
        if (hashIdx >= 0) cutIdx = (cutIdx < 0) ? hashIdx : std::min(cutIdx, hashIdx);
        if (semiIdx >= 0) cutIdx = (cutIdx < 0) ? semiIdx : std::min(cutIdx, semiIdx);
        if (slashIdx >= 0) cutIdx = (cutIdx < 0) ? slashIdx : std::min(cutIdx, slashIdx);

        if (cutIdx >= 0) {
            res = res.left(cutIdx);
        }
    }

    if (options.normalizeLabels) {
        // Replace compiler generated labels like .L123, .LFB45, .Ltext0 with generic .L_label
        static const QRegularExpression labelRegex(R"(\.L[A-Za-z0-9_]+)");
        res.replace(labelRegex, ".L_lbl");
    }

    if (options.ignoreWhitespace) {
        res = res.simplified();
    } else {
        res = res.trimmed();
    }

    return res;
}

DiffResult AssemblyDiffProcessor::computeDiff(const QString& assemblyA, const QString& assemblyB,
                                              const DiffOptions& options,
                                              const std::function<bool()>& isCancelled) {
    QElapsedTimer timer;
    timer.start();

    DiffResult result;

    if (isCancelled && isCancelled()) {
        result.isCancelled = true;
        result.statusMessage = "Diff computation cancelled";
        result.durationMs = timer.elapsed();
        return result;
    }

    QStringList linesA = assemblyA.split('\n');
    QStringList linesB = assemblyB.split('\n');

    // Remove trailing empty line if string ended with newline
    if (!linesA.isEmpty() && linesA.last().isEmpty()) linesA.removeLast();
    if (!linesB.isEmpty() && linesB.last().isEmpty()) linesB.removeLast();

    int nA = linesA.size();
    int nB = linesB.size();

    // Check line bounds
    if (nA > options.maxLines || nB > options.maxLines) {
        result.isTruncated = true;
        result.statusMessage = QString("Input exceeds line limit of %1 lines").arg(options.maxLines);
        // Truncate lines for processing
        if (nA > options.maxLines) linesA = linesA.mid(0, options.maxLines);
        if (nB > options.maxLines) linesB = linesB.mid(0, options.maxLines);
        nA = linesA.size();
        nB = linesB.size();
    }

    LineInterner interner;
    QVector<LineToken> tokensA;
    tokensA.reserve(nA);
    for (int i = 0; i < nA; ++i) {
        QString norm = normalizeAssemblyLine(linesA[i], options);
        tokensA.append({linesA[i], interner.intern(norm), i});
    }

    QVector<LineToken> tokensB;
    tokensB.reserve(nB);
    for (int i = 0; i < nB; ++i) {
        QString norm = normalizeAssemblyLine(linesB[i], options);
        tokensB.append({linesB[i], interner.intern(norm), i});
    }

    // 1. Prefix trimming
    int start = 0;
    while (start < nA && start < nB && tokensA[start].id == tokensB[start].id) {
        DiffEntry entry;
        entry.type = DiffLineType::Unchanged;
        entry.textA = tokensA[start].raw;
        entry.textB = tokensB[start].raw;
        entry.lineIndexA = tokensA[start].originalIndex;
        entry.lineIndexB = tokensB[start].originalIndex;
        result.entries.append(entry);
        result.unchanged++;
        ++start;
    }

    // 2. Suffix trimming
    int endA = nA - 1;
    int endB = nB - 1;
    QVector<DiffEntry> suffixEntries;

    while (endA >= start && endB >= start && tokensA[endA].id == tokensB[endB].id) {
        DiffEntry entry;
        entry.type = DiffLineType::Unchanged;
        entry.textA = tokensA[endA].raw;
        entry.textB = tokensB[endB].raw;
        entry.lineIndexA = tokensA[endA].originalIndex;
        entry.lineIndexB = tokensB[endB].originalIndex;
        suffixEntries.append(entry);
        result.unchanged++;
        --endA;
        --endB;
    }
    std::reverse(suffixEntries.begin(), suffixEntries.end());

    int subA = endA - start + 1;
    int subB = endB - start + 1;

    if (subA == 0 && subB == 0) {
        // Entire input was matched via prefix/suffix trimming!
        result.entries.append(suffixEntries);
        result.durationMs = timer.elapsed();
        return result;
    }

    if (subA == 0) {
        // Pure additions
        for (int j = start; j <= endB; ++j) {
            DiffEntry entry;
            entry.type = DiffLineType::Added;
            entry.textA = "";
            entry.textB = tokensB[j].raw;
            entry.lineIndexA = -1;
            entry.lineIndexB = tokensB[j].originalIndex;
            result.entries.append(entry);
            result.additions++;
        }
        result.entries.append(suffixEntries);
        result.durationMs = timer.elapsed();
        return result;
    }

    if (subB == 0) {
        // Pure deletions
        for (int i = start; i <= endA; ++i) {
            DiffEntry entry;
            entry.type = DiffLineType::Removed;
            entry.textA = tokensA[i].raw;
            entry.textB = "";
            entry.lineIndexA = tokensA[i].originalIndex;
            entry.lineIndexB = -1;
            result.entries.append(entry);
            result.deletions++;
        }
        result.entries.append(suffixEntries);
        result.durationMs = timer.elapsed();
        return result;
    }

    // 3. Myers Diff on remaining middle slice
    int maxD = subA + subB;
    int vOffset = maxD;
    int vSize = 2 * maxD + 2;

    qint64 totalBudget = static_cast<qint64>(options.maxDiffSteps);
    bool hitBudget = false;

    QVector<int> v(vSize, 0);
    v[1 + vOffset] = 0;
    QVector<QVector<int>> trace;

    int finalD = -1;
    qint64 steps = 0;

    for (int d = 0; d <= maxD; ++d) {
        if (isCancelled && isCancelled()) {
            result.isCancelled = true;
            result.statusMessage = "Diff computation cancelled";
            result.durationMs = timer.elapsed();
            return result;
        }

        for (int k = -d; k <= d; k += 2) {
            ++steps;
            if (steps > totalBudget) {
                hitBudget = true;
                break;
            }

            int kIdx = k + vOffset;
            int x = 0;
            if (k == -d || (k != d && v[kIdx - 1] < v[kIdx + 1])) {
                x = v[kIdx + 1];
            } else {
                x = v[kIdx - 1] + 1;
            }

            int y = x - k;

            // Follow diagonal snake while elements match
            while (x < subA && y < subB && tokensA[start + x].id == tokensB[start + y].id) {
                ++x;
                ++y;
            }

            v[kIdx] = x;

            if (x >= subA && y >= subB) {
                finalD = d;
                break;
            }
        }

        trace.append(v);

        if (finalD >= 0 || hitBudget) {
            break;
        }
    }

    if (hitBudget || finalD < 0) {
        result.isTruncated = true;
        result.statusMessage = "Diff step limit reached; displaying block-level difference";

        for (int i = start; i <= endA; ++i) {
            DiffEntry entry;
            entry.type = DiffLineType::Removed;
            entry.textA = tokensA[i].raw;
            entry.textB = "";
            entry.lineIndexA = tokensA[i].originalIndex;
            entry.lineIndexB = -1;
            result.entries.append(entry);
            result.deletions++;
        }
        for (int j = start; j <= endB; ++j) {
            DiffEntry entry;
            entry.type = DiffLineType::Added;
            entry.textA = "";
            entry.textB = tokensB[j].raw;
            entry.lineIndexA = -1;
            entry.lineIndexB = tokensB[j].originalIndex;
            result.entries.append(entry);
            result.additions++;
        }
        result.entries.append(suffixEntries);
        result.durationMs = timer.elapsed();
        return result;
    }

    // Backtrack trace to reconstruct edit script
    QVector<DiffEntry> middleReversed;
    int curX = subA;
    int curY = subB;

    for (int d = finalD; d > 0; --d) {
        const QVector<int>& prevV = trace[d - 1];
        int k = curX - curY;

        int prevK = 0;
        if (k == -d || (k != d && prevV[k - 1 + vOffset] < prevV[k + 1 + vOffset])) {
            prevK = k + 1;
        } else {
            prevK = k - 1;
        }

        int prevX = prevV[prevK + vOffset];
        int prevY = prevX - prevK;
        int midX = (prevK == k + 1) ? prevX : prevX + 1;
        int midY = midX - k;

        // Diagonal snake
        while (curX > midX && curY > midY) {
            --curX;
            --curY;
            DiffEntry entry;
            entry.type = DiffLineType::Unchanged;
            entry.textA = tokensA[start + curX].raw;
            entry.textB = tokensB[start + curY].raw;
            entry.lineIndexA = tokensA[start + curX].originalIndex;
            entry.lineIndexB = tokensB[start + curY].originalIndex;
            middleReversed.append(entry);
            result.unchanged++;
        }

        DiffEntry entry;
        if (midX == prevX) {
            // Insertion in B
            entry.type = DiffLineType::Added;
            entry.textA = "";
            entry.textB = tokensB[start + prevY].raw;
            entry.lineIndexA = -1;
            entry.lineIndexB = tokensB[start + prevY].originalIndex;
            middleReversed.append(entry);
            result.additions++;
        } else {
            // Deletion from A
            entry.type = DiffLineType::Removed;
            entry.textA = tokensA[start + prevX].raw;
            entry.textB = "";
            entry.lineIndexA = tokensA[start + prevX].originalIndex;
            entry.lineIndexB = -1;
            middleReversed.append(entry);
            result.deletions++;
        }

        curX = prevX;
        curY = prevY;
    }

    // Remaining initial diagonal at d=0
    while (curX > 0 && curY > 0) {
        --curX;
        --curY;
        DiffEntry entry;
        entry.type = DiffLineType::Unchanged;
        entry.textA = tokensA[start + curX].raw;
        entry.textB = tokensB[start + curY].raw;
        entry.lineIndexA = tokensA[start + curX].originalIndex;
        entry.lineIndexB = tokensB[start + curY].originalIndex;
        middleReversed.append(entry);
        result.unchanged++;
    }

    // Append reversed middle entries in correct forward order
    for (int idx = middleReversed.size() - 1; idx >= 0; --idx) {
        result.entries.append(middleReversed[idx]);
    }

    // Append unchanged suffix
    result.entries.append(suffixEntries);

    result.durationMs = timer.elapsed();
    return result;
}

QString AssemblyDiffProcessor::toUnifiedDiff(const DiffResult& result,
                                             const QString& labelA,
                                             const QString& labelB) {
    QString out;
    QTextStream ts(&out);

    ts << "--- " << labelA << "\n";
    ts << "+++ " << labelB << "\n";
    ts << "@@ -1," << result.unchanged + result.deletions
       << " +1," << result.unchanged + result.additions << " @@\n";

    for (const DiffEntry& entry : result.entries) {
        switch (entry.type) {
            case DiffLineType::Unchanged:
                ts << " " << entry.textA << "\n";
                break;
            case DiffLineType::Added:
                ts << "+" << entry.textB << "\n";
                break;
            case DiffLineType::Removed:
                ts << "-" << entry.textA << "\n";
                break;
            case DiffLineType::Modified:
                ts << "-" << entry.textA << "\n";
                ts << "+" << entry.textB << "\n";
                break;
        }
    }

    return out;
}

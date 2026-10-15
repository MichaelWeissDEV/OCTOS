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

// Syntax family of the text being compared. Comment stripping and label handling are only
// applied for dialects whose rules are known; Unknown means conservative whitespace-only handling.
enum class AsmDialect {
    Unknown,
    X86ATT,      // GNU as, AT&T operand order: '#' starts a comment
    X86Intel,    // GNU as / clang, Intel syntax: '#' starts a comment
    ARM64,       // '//' (and Apple ';') start comments; '#' is an immediate prefix, NOT a comment
    JvmBytecode, // javap: '//' comments; '#12' are constant pool references
    CPythonBytecode // dis output has no comment syntax
};

struct DiffOptions {
    bool ignoreWhitespace{false};
    bool ignoreComments{false};
    bool normalizeLabels{false};   // alpha-renames compiler labels by order of first appearance
    AsmDialect dialect{AsmDialect::Unknown};
    int maxDiffSteps{20000000};    // work budget (iterations + snake steps) of the Myers search
    int maxLines{100000};          // per side
    qint64 maxInputBytes{64LL * 1024 * 1024};  // per side
    qint64 maxMemoryBytes{512LL * 1024 * 1024}; // estimated working set budget
};

struct DiffResult {
    QVector<DiffEntry> entries;
    int additions{0};
    int deletions{0};
    int modifications{0};
    int unchanged{0};
    bool isTruncated{false};   // input or search was cut short: result is NOT a full comparison
    bool isApproximate{false}; // entries are a coarse block-level substitute, not a minimal diff
    bool isCancelled{false};
    qint64 durationMs{0};
    QString statusMessage;
    bool endsWithNewlineA{true};
    bool endsWithNewlineB{true};
    int linesA{0};
    int linesB{0};
    qint64 estimatedPeakBytes{0};

    // True only if the diff is a complete, exact comparison of both inputs.
    bool isComplete() const { return !isTruncated && !isApproximate && !isCancelled; }

    bool hasDifferences() const {
        return additions > 0 || deletions > 0 || modifications > 0;
    }
};

class AssemblyDiffProcessor {
public:
    static DiffResult computeDiff(const QString& assemblyA, const QString& assemblyB,
                                 const DiffOptions& options = DiffOptions(),
                                 const std::function<bool()>& isCancelled = nullptr);

    /**
     * Exports a valid unified diff (3 lines of context) that turns text A into text B.
     * For incomplete/approximate/cancelled results no patch is produced: an empty string is
     * returned and `error` explains why - unless `allowIncomplete` is set, in which case the
     * output starts with "# OCTOS INCOMPLETE DIFF" banner lines that make it unusable as a patch.
     */
    static QString toUnifiedDiff(const DiffResult& result,
                                 const QString& labelA = "Pane A",
                                 const QString& labelB = "Pane B",
                                 QString* error = nullptr,
                                 bool allowIncomplete = false,
                                 int contextLines = 3);

    /// Stateless per-line normalization (comments, whitespace). Label renaming needs document
    /// context and is performed by computeDiff().
    static QString normalizeAssemblyLine(const QString& line, const DiffOptions& options);

    /// Best-effort dialect detection for native assembly text (x86 vs ARM64). Returns
    /// Unknown if the evidence is ambiguous so that no comment rules are guessed.
    static AsmDialect detectNativeDialect(const QString& assembly, bool intelSyntaxHint);

    /// Removes a trailing comment according to the dialect's rules (string/char-literal aware).
    static QString stripComment(const QString& line, AsmDialect dialect);
};

#endif // ASSEMBLYDIFFPROCESSOR_H

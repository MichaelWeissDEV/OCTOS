#include "AssemblyDiffProcessor.h"
#include <QElapsedTimer>
#include <QHash>
#include <QRegularExpression>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace {

// String Interner / Line Hasher
class LineInterner {
public:
    int intern(const QString& line) {
        auto it = m_map.constFind(line);
        if (it != m_map.constEnd()) return it.value();
        int newId = ++m_nextId;
        m_map.insert(line, newId);
        return newId;
    }
private:
    QHash<QString, int> m_map;
    int m_nextId{0};
};

bool isGasDialect(AsmDialect d) {
    return d == AsmDialect::X86ATT || d == AsmDialect::X86Intel || d == AsmDialect::ARM64;
}

// Compiler-generated local labels (GNU .L*, Apple L*BB*/Ltmp*/LCPI*).
const QRegularExpression& labelRegex() {
    static const QRegularExpression re(R"((?<![A-Za-z0-9_$.])(\.L[A-Za-z0-9_$.]+|LBB\d+_\d+|Ltmp\d+|LCPI\d+_\d+))");
    return re;
}

// Linear-space Myers (divide and conquer on the middle snake). Memory is O(N+M): two V arrays
// and two flag arrays, independent of the number of edits.
class LinearMyers {
public:
    LinearMyers(const std::vector<int>& a, const std::vector<int>& b, qint64 budget,
                const std::function<bool()>& cancel)
        : A(a), B(b), m_budget(budget), m_cancel(cancel) {
        const size_t n = a.size() + b.size();
        m_vf.assign(n + 4, 0);
        m_vb.assign(n + 4, 0);
        m_del.assign(a.size(), 0);
        m_ins.assign(b.size(), 0);
    }

    enum Status { Ok, BudgetExceeded, Cancelled };

    Status run() {
        solve(0, (int)A.size(), 0, (int)B.size(), 0);
        return m_status;
    }

    const std::vector<uint8_t>& deleted() const { return m_del; }
    const std::vector<uint8_t>& inserted() const { return m_ins; }

private:
    const std::vector<int>& A;
    const std::vector<int>& B;
    std::vector<int> m_vf, m_vb;
    std::vector<uint8_t> m_del, m_ins;
    qint64 m_budget;
    qint64 m_work{0};
    qint64 m_sinceCheck{0};
    const std::function<bool()>& m_cancel;
    Status m_status{Ok};

    bool tick(qint64 n) {
        m_work += n;
        m_sinceCheck += n;
        if (m_work > m_budget) { m_status = BudgetExceeded; return false; }
        if (m_sinceCheck >= 4096) {  // cancellation also inside long loops
            m_sinceCheck = 0;
            if (m_cancel && m_cancel()) { m_status = Cancelled; return false; }
        }
        return true;
    }

    void markDel(int lo, int hi) { for (int i = lo; i < hi; ++i) m_del[i] = 1; }
    void markIns(int lo, int hi) { for (int j = lo; j < hi; ++j) m_ins[j] = 1; }

    void solve(int aLo, int aHi, int bLo, int bHi, int depth) {
        if (m_status != Ok) return;
        while (aLo < aHi && bLo < bHi && A[aLo] == B[bLo]) { ++aLo; ++bLo; }
        while (aLo < aHi && bLo < bHi && A[aHi - 1] == B[bHi - 1]) { --aHi; --bHi; }
        if (aLo == aHi) { markIns(bLo, bHi); return; }
        if (bLo == bHi) { markDel(aLo, aHi); return; }
        if (depth > 200) { markDel(aLo, aHi); markIns(bLo, bHi); return; }  // safety net

        int xs, ys, xe, ye;
        if (!middleSnake(aLo, aHi, bLo, bHi, xs, ys, xe, ye)) return;
        xs += aLo; xe += aLo; ys += bLo; ye += bLo;
        // Both halves must be strictly smaller; otherwise fall back to a block replacement.
        if ((xs == aLo && ys == bLo && xe == aHi && ye == bHi) ||
            (xs == aLo && ys == bLo && xe == aLo && ye == bLo)) {
            markDel(aLo, aHi); markIns(bLo, bHi);
            return;
        }
        solve(aLo, xs, bLo, ys, depth + 1);
        solve(xe, aHi, ye, bHi, depth + 1);
    }

    // Myers' "middle snake": returns (xs,ys)-(xe,ye) relative to the sub-range origin.
    bool middleSnake(int aLo, int aHi, int bLo, int bHi, int& xs, int& ys, int& xe, int& ye) {
        const int N = aHi - aLo, M = bHi - bLo;
        const int delta = N - M;
        const bool odd = (delta & 1) != 0;
        const int maxD = (N + M + 1) / 2;
        const int off = maxD + 1;
        int* vf = m_vf.data();
        int* vb = m_vb.data();
        vf[off + 1] = 0;
        vb[off + 1] = 0;

        for (int d = 0; d <= maxD; ++d) {
            // forward
            for (int k = -d; k <= d; k += 2) {
                int x = (k == -d || (k != d && vf[off + k - 1] < vf[off + k + 1])) ? vf[off + k + 1]
                                                                                     : vf[off + k - 1] + 1;
                int y = x - k;
                const int x0 = x, y0 = y;
                while (x < N && y < M && A[aLo + x] == B[bLo + y]) { ++x; ++y; }
                if (!tick(1 + (x - x0))) return false;
                vf[off + k] = x;
                if (odd && k >= delta - (d - 1) && k <= delta + (d - 1)) {
                    if (vf[off + k] + vb[off + (delta - k)] >= N) {
                        xs = x0; ys = y0; xe = x; ye = y;
                        return true;
                    }
                }
            }
            // backward (on reversed sequences)
            for (int k = -d; k <= d; k += 2) {
                int x = (k == -d || (k != d && vb[off + k - 1] < vb[off + k + 1])) ? vb[off + k + 1]
                                                                                     : vb[off + k - 1] + 1;
                int y = x - k;
                const int x0 = x, y0 = y;
                while (x < N && y < M && A[aHi - 1 - x] == B[bHi - 1 - y]) { ++x; ++y; }
                if (!tick(1 + (x - x0))) return false;
                vb[off + k] = x;
                if (!odd && k >= delta - d && k <= delta + d) {
                    if (vb[off + k] + vf[off + (delta - k)] >= N) {
                        xs = N - x; ys = M - y; xe = N - x0; ye = M - y0;
                        return true;
                    }
                }
            }
        }
        // Unreachable for valid input; treat as "no progress" so the caller replaces the block.
        xs = 0; ys = 0; xe = 0; ye = 0;
        return true;
    }
};

struct Op {
    enum Kind : uint8_t { Equal, Delete, Insert } kind;
    int a;  // index into A lines, -1 for Insert
    int b;  // index into B lines, -1 for Delete
};

} // namespace

AsmDialect AssemblyDiffProcessor::detectNativeDialect(const QString& text, bool intelHint) {
    // Look at a bounded prefix: enough for directives and the first instructions.
    const QString head = text.left(16384);
    static const QRegularExpression armDirective(R"(^\s*\.(arch\s+armv8|cpu\s+(generic|cortex)))", QRegularExpression::MultilineOption);
    static const QRegularExpression armInsn(R"(^\s*(stp|ldp|adrp|ldr|str|bl|cbz|cbnz|b\.[a-z]{2})\s+[^\n]*\b[xw]\d{1,2}\b)", QRegularExpression::MultilineOption);
    static const QRegularExpression x86Reg(R"(%[re]?(ax|bx|cx|dx|si|di|sp|bp)\b|\b(r|e)?(ax|bx|cx|dx|rsp|rbp)\b\s*,|\.intel_syntax)");
    const bool arm = armDirective.match(head).hasMatch() || armInsn.match(head).hasMatch();
    const bool x86 = x86Reg.match(head).hasMatch();
    if (arm && !x86) return AsmDialect::ARM64;
    if (x86 && !arm) {
        const bool intel = head.contains(".intel_syntax") || (intelHint && !head.contains('%'));
        return intel ? AsmDialect::X86Intel : AsmDialect::X86ATT;
    }
    return AsmDialect::Unknown;
}

QString AssemblyDiffProcessor::stripComment(const QString& line, AsmDialect dialect) {
    if (dialect == AsmDialect::Unknown || dialect == AsmDialect::CPythonBytecode) return line;

    bool inStr = false, inChr = false;
    int firstNonSpace = -1;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (firstNonSpace < 0 && !c.isSpace()) firstNonSpace = i;
        if (inStr) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        (void)inChr;

        switch (dialect) {
            case AsmDialect::X86ATT:
            case AsmDialect::X86Intel:
                if (c == '#') return line.left(i);
                break;
            case AsmDialect::ARM64:
                if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') return line.left(i);
                if (c == ';') return line.left(i);           // Apple/Mach-O comment marker
                if (c == '#' && i == firstNonSpace) return QString();  // '#' elsewhere is an immediate
                break;
            case AsmDialect::JvmBytecode:
                if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') return line.left(i);
                break;
            default:
                break;
        }
    }
    return line;
}

QString AssemblyDiffProcessor::normalizeAssemblyLine(const QString& line, const DiffOptions& options) {
    QString res = line;
    if (options.ignoreComments) res = stripComment(res, options.dialect);
    return options.ignoreWhitespace ? res.simplified() : res.trimmed();
}

DiffResult AssemblyDiffProcessor::computeDiff(const QString& assemblyA, const QString& assemblyB,
                                              const DiffOptions& options,
                                              const std::function<bool()>& isCancelled) {
    QElapsedTimer timer;
    timer.start();
    DiffResult result;

    auto done = [&]() { result.durationMs = timer.elapsed(); return result; };
    auto cancelledNow = [&]() { return isCancelled && isCancelled(); };

    if (cancelledNow()) {
        result.isCancelled = true;
        result.statusMessage = "Diff computation cancelled";
        return done();
    }

    // Hard input bound before any allocation proportional to the input.
    if ((qint64)assemblyA.size() * 2 > options.maxInputBytes || (qint64)assemblyB.size() * 2 > options.maxInputBytes) {
        result.isTruncated = true;
        result.statusMessage = QString("Input exceeds the size limit of %1 bytes; no comparison was performed")
                                   .arg(options.maxInputBytes);
        return done();
    }

    QStringList linesA = assemblyA.split('\n');
    QStringList linesB = assemblyB.split('\n');
    // A text ending in '\n' yields a trailing empty element; that is not a line.
    result.endsWithNewlineA = assemblyA.isEmpty() || assemblyA.endsWith('\n');
    result.endsWithNewlineB = assemblyB.isEmpty() || assemblyB.endsWith('\n');
    if (!linesA.isEmpty() && linesA.last().isEmpty()) linesA.removeLast();
    if (!linesB.isEmpty() && linesB.last().isEmpty()) linesB.removeLast();

    if (linesA.size() > options.maxLines || linesB.size() > options.maxLines) {
        // Never compare a silently cut input and call it a result: report and stop.
        result.isTruncated = true;
        result.linesA = linesA.size();
        result.linesB = linesB.size();
        result.statusMessage = QString("Input exceeds the line limit of %1 lines (%2 / %3); no comparison was performed")
                                   .arg(options.maxLines).arg(linesA.size()).arg(linesB.size());
        return done();
    }

    const int nA = linesA.size();
    const int nB = linesB.size();
    result.linesA = nA;
    result.linesB = nB;

    // Working set estimate: tokens, V arrays (2 x (N+M) ints), flags, entries.
    const qint64 est = (qint64)(nA + nB) * (4 + 4 * 2 + 2) + (qint64)(nA + nB) * (qint64)sizeof(DiffEntry);
    result.estimatedPeakBytes = est;
    if (est > options.maxMemoryBytes) {
        result.isTruncated = true;
        result.statusMessage = QString("Estimated memory use (%1 MB) exceeds the budget (%2 MB); no comparison was performed")
                                   .arg(est >> 20).arg(options.maxMemoryBytes >> 20);
        return done();
    }

    // Normalize + intern. Label renaming is per document, in order of first appearance.
    LineInterner interner;
    auto tokenize = [&](const QStringList& lines) {
        std::vector<int> ids;
        ids.reserve(lines.size());
        QHash<QString, int> labelMap;
        const bool rename = options.normalizeLabels && isGasDialect(options.dialect);
        for (int i = 0; i < lines.size(); ++i) {
            QString norm = normalizeAssemblyLine(lines[i], options);
            if (rename) {
                QString out;
                int last = 0;
                auto it = labelRegex().globalMatch(norm);
                while (it.hasNext()) {
                    auto m = it.next();
                    const QString lbl = m.captured(1);
                    int ord = labelMap.value(lbl, -1);
                    if (ord < 0) { ord = labelMap.size(); labelMap.insert(lbl, ord); }
                    out += norm.mid(last, m.capturedStart(1) - last);
                    out += ".Lc" + QString::number(ord);
                    last = m.capturedEnd(1);
                }
                out += norm.mid(last);
                norm = out;
            }
            ids.push_back(interner.intern(norm));
            if ((i & 1023) == 0 && cancelledNow()) { ids.clear(); break; }
        }
        return ids;
    };
    std::vector<int> idsA = tokenize(linesA);
    std::vector<int> idsB = tokenize(linesB);
    if ((int)idsA.size() != nA || (int)idsB.size() != nB || cancelledNow()) {
        result.isCancelled = true;
        result.statusMessage = "Diff computation cancelled";
        return done();
    }

    LinearMyers myers(idsA, idsB, options.maxDiffSteps, isCancelled);
    const auto status = myers.run();

    if (status == LinearMyers::Cancelled) {
        result.isCancelled = true;
        result.statusMessage = "Diff computation cancelled";
        return done();
    }

    std::vector<Op> ops;
    ops.reserve(nA + nB);
    if (status == LinearMyers::BudgetExceeded) {
        // Clearly flagged coarse substitute: common prefix/suffix kept, middle replaced as a block.
        result.isTruncated = true;
        result.isApproximate = true;
        result.statusMessage = "Diff work limit reached: showing a coarse block-level difference, NOT a minimal or exact diff";
        int pre = 0;
        while (pre < nA && pre < nB && idsA[pre] == idsB[pre]) ++pre;
        int suf = 0;
        while (suf < nA - pre && suf < nB - pre && idsA[nA - 1 - suf] == idsB[nB - 1 - suf]) ++suf;
        for (int i = 0; i < pre; ++i) ops.push_back({Op::Equal, i, i});
        for (int i = pre; i < nA - suf; ++i) ops.push_back({Op::Delete, i, -1});
        for (int j = pre; j < nB - suf; ++j) ops.push_back({Op::Insert, -1, j});
        for (int s = suf; s > 0; --s) ops.push_back({Op::Equal, nA - s, nB - s});
    } else {
        const auto& del = myers.deleted();
        const auto& ins = myers.inserted();
        int i = 0, j = 0;
        while (i < nA || j < nB) {
            if (i < nA && del[i]) { ops.push_back({Op::Delete, i, -1}); ++i; }
            else if (j < nB && ins[j]) { ops.push_back({Op::Insert, -1, j}); ++j; }
            else { ops.push_back({Op::Equal, i, j}); ++i; ++j; }
        }
    }

    result.entries.reserve((int)ops.size());
    for (const Op& op : ops) {
        DiffEntry e;
        switch (op.kind) {
            case Op::Equal:
                e.type = DiffLineType::Unchanged;
                e.textA = linesA[op.a]; e.textB = linesB[op.b];
                e.lineIndexA = op.a; e.lineIndexB = op.b;
                ++result.unchanged;
                break;
            case Op::Delete:
                e.type = DiffLineType::Removed;
                e.textA = linesA[op.a]; e.lineIndexA = op.a;
                ++result.deletions;
                break;
            case Op::Insert:
                e.type = DiffLineType::Added;
                e.textB = linesB[op.b]; e.lineIndexB = op.b;
                ++result.additions;
                break;
        }
        result.entries.append(e);
    }
    return done();
}

QString AssemblyDiffProcessor::toUnifiedDiff(const DiffResult& result, const QString& labelA,
                                             const QString& labelB, QString* error,
                                             bool allowIncomplete, int contextLines) {
    QString banner;
    if (!result.isComplete()) {
        const QString why = result.isCancelled ? QString("the computation was cancelled")
                          : result.statusMessage.isEmpty() ? QString("the diff is incomplete") : result.statusMessage;
        if (!allowIncomplete) {
            if (error) *error = "No patch exported: " + why;
            return QString();
        }
        banner = "# OCTOS INCOMPLETE DIFF - NOT A VALID PATCH: " + why + "\n";
    }
    if (error) error->clear();

    // Patch operations. Lines that are equal only after normalization differ textually between
    // A and B, so a faithful patch must replace them (-A/+B).
    struct P { char c; QString text; int a; int b; };
    std::vector<P> ops;
    ops.reserve(result.entries.size());
    for (const DiffEntry& e : result.entries) {
        switch (e.type) {
            case DiffLineType::Unchanged:
                if (e.textA == e.textB) ops.push_back({' ', e.textA, e.lineIndexA, e.lineIndexB});
                else {
                    ops.push_back({'-', e.textA, e.lineIndexA, -1});
                    ops.push_back({'+', e.textB, -1, e.lineIndexB});
                }
                break;
            case DiffLineType::Added: ops.push_back({'+', e.textB, -1, e.lineIndexB}); break;
            case DiffLineType::Removed: ops.push_back({'-', e.textA, e.lineIndexA, -1}); break;
            case DiffLineType::Modified:
                ops.push_back({'-', e.textA, e.lineIndexA, -1});
                ops.push_back({'+', e.textB, -1, e.lineIndexB});
                break;
        }
    }

    // A differing final-newline state must show up as a change of the last line.
    if (result.linesA > 0 && result.linesB > 0 && result.endsWithNewlineA != result.endsWithNewlineB) {
        for (size_t i = 0; i < ops.size(); ++i) {
            if (ops[i].c == ' ' && ops[i].a == result.linesA - 1 && ops[i].b == result.linesB - 1) {
                P del{'-', ops[i].text, ops[i].a, -1};
                P add{'+', ops[i].text, -1, ops[i].b};
                ops[i] = del;
                ops.insert(ops.begin() + i + 1, add);
                break;
            }
        }
    }

    QString out = banner;
    out += "--- " + labelA + "\n";
    out += "+++ " + labelB + "\n";

    const int total = (int)ops.size();
    int i = 0;
    while (i < total) {
        while (i < total && ops[i].c == ' ') ++i;
        if (i >= total) break;
        int hunkStart = std::max(0, i - contextLines);
        int lastChange = i;
        int j = i;
        while (j < total) {
            if (ops[j].c != ' ') { lastChange = j; ++j; continue; }
            // Extend over context gaps that are small enough to merge hunks.
            int k = j;
            while (k < total && ops[k].c == ' ') ++k;
            if (k < total && (k - j) <= 2 * contextLines) { j = k; continue; }
            break;
        }
        int hunkEnd = std::min(total, lastChange + 1 + contextLines);

        int aStart = -1, bStart = -1, aLen = 0, bLen = 0;
        for (int p = hunkStart; p < hunkEnd; ++p) {
            if (ops[p].c != '+') { if (aStart < 0) aStart = ops[p].a; ++aLen; }
            if (ops[p].c != '-') { if (bStart < 0) bStart = ops[p].b; ++bLen; }
        }
        // For an empty side the start refers to the line *before* the hunk (unified diff rule).
        auto before = [&](bool sideA) {
            int cnt = 0;
            for (int p = 0; p < hunkStart; ++p) if (sideA ? ops[p].c != '+' : ops[p].c != '-') ++cnt;
            return cnt;  // number of lines of that side preceding the hunk
        };
        const int aHeader = aLen == 0 ? before(true) : aStart + 1;
        const int bHeader = bLen == 0 ? before(false) : bStart + 1;
        out += QString("@@ -%1,%2 +%3,%4 @@\n").arg(aHeader).arg(aLen).arg(bHeader).arg(bLen);

        for (int p = hunkStart; p < hunkEnd; ++p) {
            out += QChar(ops[p].c);
            out += ops[p].text;
            out += '\n';
            const bool lastA = ops[p].c != '+' && ops[p].a == result.linesA - 1 && !result.endsWithNewlineA;
            const bool lastB = ops[p].c != '-' && ops[p].b == result.linesB - 1 && !result.endsWithNewlineB;
            if (lastA || lastB) out += "\\ No newline at end of file\n";
        }
        i = hunkEnd;
    }
    return out;
}

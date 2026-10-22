#include "StrategyUtils.h"
#include "../core/WorkspaceModel.h"
#include <QRegularExpression>

namespace StrategyUtils {

QStringList splitCompilerArguments(const QString& flags) {
    QStringList result;
    QString currentArg;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool escaped = false;

    for (int i = 0; i < flags.size(); ++i) {
        QChar c = flags[i];

        if (escaped) {
            currentArg += c;
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            currentArg += c;
            continue;
        }

        if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            continue;
        }

        if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }

        if ((c == ' ' || c == '\t' || c == '\n' || c == '\r') &&
            !inSingleQuote && !inDoubleQuote) {
            if (!currentArg.isEmpty()) {
                result.append(currentArg);
                currentArg.clear();
            }
            continue;
        }

        currentArg += c;
    }

    if (!currentArg.isEmpty()) {
        result.append(currentArg);
    }

    return result;
}

QStringList sanitizeUserFlags(const QStringList& rawFlags,
                              const QStringList& customForbidden) {
    QStringList sanitized;
    bool skipNext = false;

    for (int i = 0; i < rawFlags.size(); ++i) {
        const QString& flag = rawFlags[i];

        if (skipNext) {
            skipNext = false;
            continue;
        }

        // Output overriding flags
        if (flag == "-o" || flag == "--output" || flag == "-out" || flag == "-c") {
            skipNext = true;
            continue;
        }

        if (flag.startsWith("-o=") || flag.startsWith("--output=") || flag.startsWith("-out:")) {
            continue;
        }

        // Check dangerous characters for shell / argument injection
        if (flag.contains(';') || flag.contains('|') || flag.contains('&') ||
            flag.contains('$') || flag.contains('`') || flag.contains('>') ||
            flag.contains('<') || flag.contains('\n') || flag.contains('\r') ||
            flag.contains('(') || flag.contains(')') || flag.contains('{') ||
            flag.contains('}')) {
            continue;
        }

        bool isForbidden = false;
        for (const QString& forb : customForbidden) {
            if (flag == forb || flag.startsWith(forb)) {
                isForbidden = true;
                break;
            }
        }
        if (isForbidden) {
            continue;
        }

        sanitized.append(flag);
    }

    return sanitized;
}

QVector<Diagnostic> parseClangGccDiagnostics(const QString& text) {
    QVector<Diagnostic> list;
    static const QRegularExpression diagRegex(
        R"(^([^:\n]+):(\d+):(\d+):\s*(error|warning|note|fatal error):\s*(.*)$)",
        QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator it = diagRegex.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        Diagnostic diag;
        diag.fileName = m.captured(1).trimmed();
        diag.line = m.captured(2).toInt();
        diag.column = m.captured(3).toInt();

        QString sev = m.captured(4).toLower();
        if (sev.contains("error")) {
            diag.severity = DiagnosticSeverity::Error;
        } else if (sev == "warning") {
            diag.severity = DiagnosticSeverity::Warning;
        } else if (sev == "note") {
            diag.severity = DiagnosticSeverity::Note;
        } else {
            diag.severity = DiagnosticSeverity::Info;
        }

        diag.message = m.captured(5).trimmed();
        list.append(diag);
    }

    return list;
}

bool isSourceSuffixFor(Language language, const QString& fileName) {
    const int dot = fileName.lastIndexOf('.');
    if (dot <= 0) return false;
    const QString suffix = fileName.mid(dot + 1);
    if (language == Language::C) return suffix == "c";
    if (language == Language::Cpp) {
        static const QStringList cxx{"cpp", "cc", "cxx", "c++", "cp", "C", "CPP"};
        return cxx.contains(suffix);
    }
    return false;
}

QString containerProjectRoot(const QString& inputFilePath, const QString& mainFilePath) {
    QString root = inputFilePath;
    const int components = mainFilePath.isEmpty() ? 1 : int(mainFilePath.count('/')) + 1;
    for (int i = 0; i < components; ++i) {
        const int slash = root.lastIndexOf('/');
        if (slash <= 0) return root.startsWith('/') ? QStringLiteral("/") : QString();
        root.truncate(slash);
    }
    return root;
}

QStringList projectIncludeFlags(const CompilationRequest& req, const QString& inputFilePath) {
    if (req.mainFilePath.isEmpty() && req.extraFiles.isEmpty()) return {};
    const QString root = containerProjectRoot(inputFilePath, req.mainFilePath);
    QStringList dirs{root};
    const int slash = inputFilePath.lastIndexOf('/');
    if (slash > 0) dirs << inputFilePath.left(slash);
    for (const QString& inc : req.includePaths) {
        bool ok = false;
        const QString clean = Workspace::normalizeRelativePath(inc, &ok);
        if (ok && !clean.isEmpty()) dirs << root + "/" + clean;
    }
    QStringList flags;
    QStringList seen;
    for (const QString& d : dirs) {
        if (seen.contains(d)) continue;
        seen << d;
        flags << ("-I" + d);
    }
    return flags;
}

} // namespace StrategyUtils

#include "JavaStrategy.h"
#include "StrategyUtils.h"
#include <QRegularExpression>

QString JavaStrategy::detectClassName(const QString& sourceCode) {
    // First try to find a public class/interface/enum/record
    static const QRegularExpression publicClassRegex(
        R"(\bpublic\s+(?:final\s+|abstract\s+|sealed\s+|non-sealed\s+)?(?:class|interface|enum|record)\s+([A-Za-z_][A-Za-z0-9_]*))"
    );
    QRegularExpressionMatch match = publicClassRegex.match(sourceCode);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    // Otherwise find the first class/interface/enum/record declared
    static const QRegularExpression anyClassRegex(
        R"(\b(?:class|interface|enum|record)\s+([A-Za-z_][A-Za-z0-9_]*))"
    );
    match = anyClassRegex.match(sourceCode);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    return "Main";
}

QString JavaStrategy::sourceRoot(const QString& sourceCode, const QString& inputFilePath) {
    // The main file's directory minus its package path: src/main/java/com/acme/App.java declaring
    // `package com.acme;` has the source root src/main/java. A file outside its package directory
    // (single-file mode) uses its own directory.
    const int slash = inputFilePath.lastIndexOf('/');
    QString root = slash > 0 ? inputFilePath.left(slash) : QStringLiteral("/");
    static const QRegularExpression packageRe(R"(^\s*package\s+([A-Za-z_][A-Za-z0-9_]*(?:\s*\.\s*[A-Za-z_][A-Za-z0-9_]*)*)\s*;)",
                                              QRegularExpression::MultilineOption);
    const QRegularExpressionMatch m = packageRe.match(sourceCode);
    if (m.hasMatch()) {
        QString packagePath = m.captured(1);
        packagePath.remove(QRegularExpression(R"(\s)"));
        packagePath.replace('.', '/');
        if (root.endsWith("/" + packagePath) && root.size() > packagePath.size() + 1)
            root.chop(packagePath.size() + 1);
    }
    return root;
}

QString JavaStrategy::sourceFileName(const QString& sourceCode) const {
    return detectClassName(sourceCode) + ".java";
}

QStringList JavaStrategy::buildCompilerArgs(const CompilationRequest& req,
                                            const QString& inputFilePath,
                                            const QString& outputDirPath) const {
    QString className = detectClassName(req.sourceCode);

    QStringList args;
    args << "sh";
    args << "-c";
    // We run javac, then javap on all generated .class files in /output
    // Note: Parameterized via $1 (outputDir), $2 (inputFilePath), $3 (className)
    // to strictly prevent command injection
    // javac may place classes in package directories and emit inner/secondary classes
    // (Outer$Inner.class, other top-level classes), so disassemble every generated class file.
    // Paths are passed as positional parameters ($1 outputDir, $2 inputFile, $3 className) and
    // never interpolated, which prevents command injection.
    // $4 is the source root: other classes the main file uses (project mode) are compiled on demand from it.
    const char* script =
        "javac -d \"$1\" -g -implicit:class -sourcepath \"$4\" \"$2\" || exit $?; "
        "find \"$1\" -name '*.class' | sort | while IFS= read -r f; do "
        "  javap -c -v -p \"$f\" || exit 1; "
        "done";

    args << script;
    args << "octos_java";
    args << outputDirPath;
    args << inputFilePath;
    args << className;
    args << sourceRoot(req.sourceCode, inputFilePath);

    return args;
}

QStringList JavaStrategy::sanitizeUserFlags(const QStringList& rawFlags) const {
    return StrategyUtils::sanitizeUserFlags(rawFlags);
}

CompilationResult JavaStrategy::processOutput(const CompilationRequest& req,
                                              int exitCode,
                                              const QString& stdoutStr,
                                              const QString& stderrStr,
                                              const QString& /*outputFileContent*/) const {
    CompilationResult result;
    result.jobId = req.jobId;
    result.paneId = req.paneId;
    result.revision = req.revision;
    result.exitCode = exitCode;
    result.rawOutput = stdoutStr + (stderrStr.isEmpty() ? "" : ("\n" + stderrStr));

    if (exitCode == 0) {
        result.state = JobState::Completed;
        result.assemblyOutput = stdoutStr;
        result.errorOutput = stderrStr;
    } else {
        result.state = JobState::Failed;
        result.errorOutput = stderrStr.isEmpty() ? stdoutStr : stderrStr;
        result.diagnostics = parseDiagnostics(result.errorOutput);
    }

    return result;
}

QVector<Diagnostic> JavaStrategy::parseDiagnostics(const QString& compilerOutput) const {
    QVector<Diagnostic> list;
    // Example: Main.java:3: error: cannot find symbol
    static const QRegularExpression javaDiagRegex(
        R"(^([^:\n]+):(\d+):\s*(error|warning|note):\s*(.*)$)",
        QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator it = javaDiagRegex.globalMatch(compilerOutput);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        Diagnostic diag;
        diag.fileName = m.captured(1).trimmed();
        diag.line = m.captured(2).toInt();
        diag.column = 0;

        QString sev = m.captured(3).toLower();
        if (sev == "error") diag.severity = DiagnosticSeverity::Error;
        else if (sev == "warning") diag.severity = DiagnosticSeverity::Warning;
        else if (sev == "note") diag.severity = DiagnosticSeverity::Note;
        else diag.severity = DiagnosticSeverity::Info;

        diag.message = m.captured(4).trimmed();
        list.append(diag);
    }

    return list;
}

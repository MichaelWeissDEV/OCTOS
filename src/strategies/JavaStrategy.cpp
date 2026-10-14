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
    const char* script =
        "javac -d \"$1\" -g \"$2\" && "
        "if [ -f \"$1/$3.class\" ]; then "
        "  javap -c -v -p \"$1/$3.class\"; "
        "else "
        "  for f in \"$1\"/*.class; do "
        "    if [ -f \"$f\" ]; then javap -c -v -p \"$f\"; fi; "
        "  done; "
        "fi";

    args << script;
    args << "octos_java";
    args << outputDirPath;
    args << inputFilePath;
    args << className;

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

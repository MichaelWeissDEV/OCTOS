#include "CSharpStrategy.h"
#include "StrategyUtils.h"
#include <QRegularExpression>

QStringList CSharpStrategy::buildCompilerArgs(const CompilationRequest& /*req*/,
                                              const QString& inputFilePath,
                                              const QString& outputDirPath) const {
    QStringList args;
    args << "sh";
    args << "-c";

    // Build IL via mono mcs + monodis
    const char* script =
        "if which mcs >/dev/null 2>&1; then "
        "  mcs -out:\"$1/Program.exe\" \"$2\" && monodis \"$1/Program.exe\"; "
        "elif which csc >/dev/null 2>&1; then "
        "  csc -out:\"$1/Program.exe\" \"$2\" && monodis \"$1/Program.exe\"; "
        "else "
        "  echo 'Error: Neither mcs nor csc compiler found in Docker container.' >&2; "
        "  echo 'C# support requires a Mono container with mcs and monodis.' >&2; "
        "  exit 127; "
        "fi";

    args << script;
    args << "octos_csharp";
    args << outputDirPath;
    args << inputFilePath;

    return args;
}

QStringList CSharpStrategy::sanitizeUserFlags(const QStringList& rawFlags) const {
    return StrategyUtils::sanitizeUserFlags(rawFlags);
}

CompilationResult CSharpStrategy::processOutput(const CompilationRequest& req,
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

QVector<Diagnostic> CSharpStrategy::parseDiagnostics(const QString& compilerOutput) const {
    QVector<Diagnostic> list;
    // Example: Program.cs(5,13): error CS0103: The name 'foo' does not exist in the current context
    static const QRegularExpression csDiagRegex(
        R"(^([^(\n]+)\((\d+),(\d+)\):\s*(error|warning)\s*([A-Za-z0-9]+):\s*(.*)$)",
        QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator it = csDiagRegex.globalMatch(compilerOutput);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        Diagnostic diag;
        diag.fileName = m.captured(1).trimmed();
        diag.line = m.captured(2).toInt();
        diag.column = m.captured(3).toInt();

        QString sev = m.captured(4).toLower();
        diag.severity = (sev == "warning") ? DiagnosticSeverity::Warning : DiagnosticSeverity::Error;
        diag.message = QString("[%1] %2").arg(m.captured(5), m.captured(6));
        list.append(diag);
    }

    return list;
}

#include "RustStrategy.h"
#include "StrategyUtils.h"
#include <QRegularExpression>

QStringList RustStrategy::buildCompilerArgs(const CompilationRequest& req,
                                            const QString& inputFilePath,
                                            const QString& outputDirPath) const {
    QStringList args;
    args << defaultCompilerBinary(req.compilerImage);

    // Assembly output for rustc
    args << "--emit=asm";

    // Syntax flavor via LLVM args
    if (req.syntaxFlavor == SyntaxFlavor::Intel) {
        args << "-C" << "llvm-args=--x86-asm-syntax=intel";
    } else {
        args << "-C" << "llvm-args=--x86-asm-syntax=att";
    }

    // Parse and sanitize user flags (removes GCC-specific flags!)
    QStringList rawFlags = StrategyUtils::splitCompilerArguments(req.userArgs);
    QStringList sanitized = sanitizeUserFlags(rawFlags);

    // Check if debuginfo is already specified
    bool hasDebug = false;
    for (const QString& flag : sanitized) {
        if (flag == "-g" || flag.startsWith("-Cdebuginfo") || flag.startsWith("-C debuginfo")) {
            hasDebug = true;
            break;
        }
    }
    if (!hasDebug) {
        args << "-g";
    }

    args.append(sanitized);

    // Input and output
    args << inputFilePath;
    args << "-o";
    args << (outputDirPath + "/output.s");

    return args;
}

bool RustStrategy::supportsSyntaxFlavor(SyntaxFlavor /*flavor*/) const {
    return true;
}

QStringList RustStrategy::sanitizeUserFlags(const QStringList& rawFlags) const {
    // Rust must NOT receive GCC/Clang specific flags!
    QStringList forbidden = {
        "-masm=",
        "-fno-asynchronous-unwind-tables",
        "-S",
        "-fno-exceptions",
        "-fPIC",
        "-fPIE",
        "-march=",
        "-mtune="
    };

    return StrategyUtils::sanitizeUserFlags(rawFlags, forbidden);
}

CompilationResult RustStrategy::processOutput(const CompilationRequest& req,
                                              int exitCode,
                                              const QString& stdoutStr,
                                              const QString& stderrStr,
                                              const QString& outputFileContent) const {
    CompilationResult result;
    result.jobId = req.jobId;
    result.paneId = req.paneId;
    result.revision = req.revision;
    result.exitCode = exitCode;
    result.rawOutput = stdoutStr + (stderrStr.isEmpty() ? "" : ("\n" + stderrStr));

    if (exitCode == 0) {
        result.state = JobState::Completed;
        result.assemblyOutput = outputFileContent;
        result.errorOutput = stderrStr;
    } else {
        result.state = JobState::Failed;
        result.errorOutput = stderrStr.isEmpty() ? stdoutStr : stderrStr;
        result.diagnostics = parseDiagnostics(result.errorOutput);
    }

    return result;
}

QVector<Diagnostic> RustStrategy::parseDiagnostics(const QString& compilerOutput) const {
    QVector<Diagnostic> list;
    static const QRegularExpression rustDiagRegex(
        R"((error|warning|note)(?:\[[A-Za-z0-9]+\])?:\s*([^\n]+)\n\s*-->\s*([^:\n]+):(\d+):(\d+))",
        QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator it = rustDiagRegex.globalMatch(compilerOutput);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        Diagnostic diag;
        QString sev = m.captured(1).toLower();
        if (sev == "error") diag.severity = DiagnosticSeverity::Error;
        else if (sev == "warning") diag.severity = DiagnosticSeverity::Warning;
        else if (sev == "note") diag.severity = DiagnosticSeverity::Note;
        else diag.severity = DiagnosticSeverity::Info;

        diag.message = m.captured(2).trimmed();
        diag.fileName = m.captured(3).trimmed();
        diag.line = m.captured(4).toInt();
        diag.column = m.captured(5).toInt();
        list.append(diag);
    }

    return list;
}

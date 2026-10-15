#include "PythonStrategy.h"
#include "StrategyUtils.h"
#include <QRegularExpression>

QStringList PythonStrategy::buildCompilerArgs(const CompilationRequest& req,
                                              const QString& inputFilePath,
                                              const QString& outputDirPath) const {
    QStringList args;
    args << defaultCompilerBinary(req.compilerImage);

    // We execute Python directly to compile bytecode into /output (safe against read-only /input)
    // and disassemble with the dis module for bytecode inspection.
    args << "-c";
    const char* pythonCode =
        "import py_compile, dis, sys\n"
        "src = sys.argv[1]\n"
        "dst = sys.argv[2]\n"
        "try:\n"
        "    code = compile(open(src, 'r', encoding='utf-8', errors='replace').read(), src, 'exec')\n"
        "    py_compile.compile(src, cfile=dst, doraise=True)\n"
        "    print('; CPython Bytecode Disassembly (Not native machine assembly)')\n"
        "    print('; Python ' + sys.version.split()[0] + ' | Source: ' + src + '\\n')\n"
        "    dis.dis(code)\n"
        "except SyntaxError as e:\n"
        "    sys.stderr.write(f'  File \"{e.filename}\", line {e.lineno}, col {e.offset}\\n    {e.text or \"\"}\\nSyntaxError: {e.msg}\\n')\n"
        "    sys.exit(1)\n"
        "except Exception as e:\n"
        "    sys.stderr.write(f'  File \"{src}\", line 0\\n    \\n{type(e).__name__}: {e}\\n')\n"
        "    sys.exit(1)\n";

    args << pythonCode;
    args << inputFilePath;
    args << (outputDirPath + "/source.pyc");

    return args;
}

QStringList PythonStrategy::sanitizeUserFlags(const QStringList& rawFlags) const {
    return StrategyUtils::sanitizeUserFlags(rawFlags);
}

CompilationResult PythonStrategy::processOutput(const CompilationRequest& req,
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

QVector<Diagnostic> PythonStrategy::parseDiagnostics(const QString& compilerOutput) const {
    QVector<Diagnostic> list;
    // Example: File "source.py", line 2, col 5 ... SyntaxError: invalid syntax
    static const QRegularExpression pyDiagRegex(
        QStringLiteral("File \"([^\"]+)\", line (\\d+)(?:, col (\\d+))?[^\\n]*\\n(?:[^\\n]*\\n)*?([A-Za-z]+Error): ([^\\n]*)"),
        QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator it = pyDiagRegex.globalMatch(compilerOutput);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        Diagnostic diag;
        diag.fileName = m.captured(1).trimmed();
        diag.line = m.captured(2).toInt();
        diag.column = m.captured(3).isEmpty() ? 0 : m.captured(3).toInt();
        diag.severity = DiagnosticSeverity::Error;
        diag.message = QString("%1: %2").arg(m.captured(4), m.captured(5));
        list.append(diag);
    }

    return list;
}

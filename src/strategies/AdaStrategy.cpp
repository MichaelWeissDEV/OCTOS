#include "AdaStrategy.h"
#include "StrategyUtils.h"
#include <QRegularExpression>

QStringList AdaStrategy::buildCompilerArgs(const CompilationRequest& req,
                                          const QString& inputFilePath,
                                          const QString& outputDirPath) const {
    QStringList args;
    args << "gcc";
    args << "-c";
    args << "-S";

    // -masm= exists only on x86 targets (GCC rejects it on aarch64).
    if (req.targetsX86()) {
        if (req.syntaxFlavor == SyntaxFlavor::Intel) {
            args << "-masm=intel";
        } else {
            args << "-masm=att";
        }
    }

    QStringList userFlags = StrategyUtils::splitCompilerArguments(req.userArgs);
    args.append(sanitizeUserFlags(userFlags));

    args << inputFilePath;
    args << "-o";
    args << (outputDirPath + "/output.s");

    return args;
}

bool AdaStrategy::supportsSyntaxFlavor(SyntaxFlavor /*flavor*/) const {
    return true;
}

QStringList AdaStrategy::sanitizeUserFlags(const QStringList& rawFlags) const {
    return StrategyUtils::sanitizeUserFlags(rawFlags);
}

CompilationResult AdaStrategy::processOutput(const CompilationRequest& req,
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
        QString err = stderrStr.isEmpty() ? stdoutStr : stderrStr;
        if (err.contains("gnat1") || err.contains("Ada is not supported")) {
            err = "Error: Ada compiler (GNAT) not found in the selected container.\n"
                  "Ada support is currently planned and requires a container with GNAT toolchain installed.\n" + err;
        }
        result.errorOutput = err;
        result.diagnostics = parseDiagnostics(result.errorOutput);
    }

    return result;
}

QVector<Diagnostic> AdaStrategy::parseDiagnostics(const QString& compilerOutput) const {
    return StrategyUtils::parseClangGccDiagnostics(compilerOutput);
}

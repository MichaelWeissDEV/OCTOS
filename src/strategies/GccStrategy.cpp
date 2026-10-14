#include "GccStrategy.h"
#include "StrategyUtils.h"

GccStrategy::GccStrategy(Language lang) : m_language(lang) {
}

Language GccStrategy::language() const {
    return m_language;
}

QString GccStrategy::languageId() const {
    return (m_language == Language::C) ? "c" : "cpp";
}

QString GccStrategy::defaultCompilerBinary(const QString& /*dockerImage*/) const {
    if (m_language == Language::C) {
        return "gcc";
    }
    return "g++";
}

QString GccStrategy::sourceFileName(const QString& /*sourceCode*/) const {
    return (m_language == Language::C) ? "source.c" : "source.cpp";
}

QString GccStrategy::outputFileName() const {
    return "output.s";
}

QStringList GccStrategy::buildCompilerArgs(const CompilationRequest& req,
                                          const QString& inputFilePath,
                                          const QString& outputDirPath) const {
    QStringList args;
    args << defaultCompilerBinary(req.compilerImage);

    // Compilation to assembly
    args << "-S";
    args << "-fno-asynchronous-unwind-tables";

    // Syntax flavor
    if (req.syntaxFlavor == SyntaxFlavor::Intel) {
        args << "-masm=intel";
    } else {
        args << "-masm=att";
    }

    // Parse user flags
    QStringList userFlags = StrategyUtils::splitCompilerArguments(req.userArgs);
    userFlags = sanitizeUserFlags(userFlags);

    // Add -g if not explicitly supplied
    bool hasDebug = false;
    for (const QString& flag : userFlags) {
        if (flag.startsWith("-g")) {
            hasDebug = true;
            break;
        }
    }
    if (!hasDebug) {
        args << "-g";
    }

    args.append(userFlags);

    // Input and output
    args << inputFilePath;
    args << "-o";
    args << (outputDirPath + "/output.s");

    return args;
}

bool GccStrategy::supportsSyntaxFlavor(SyntaxFlavor /*flavor*/) const {
    return true;
}

QStringList GccStrategy::sanitizeUserFlags(const QStringList& rawFlags) const {
    return StrategyUtils::sanitizeUserFlags(rawFlags);
}

CompilationResult GccStrategy::processOutput(const CompilationRequest& req,
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

QVector<Diagnostic> GccStrategy::parseDiagnostics(const QString& compilerOutput) const {
    return StrategyUtils::parseClangGccDiagnostics(compilerOutput);
}

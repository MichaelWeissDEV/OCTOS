#include "ClangStrategy.h"
#include "StrategyUtils.h"

ClangStrategy::ClangStrategy(Language lang) : m_language(lang) {
}

Language ClangStrategy::language() const {
    return m_language;
}

QString ClangStrategy::languageId() const {
    return (m_language == Language::C) ? "c" : "cpp";
}

QString ClangStrategy::defaultCompilerBinary(const QString& /*dockerImage*/) const {
    if (m_language == Language::C) {
        return "clang";
    }
    return "clang++";
}

QString ClangStrategy::sourceFileName(const QString& /*sourceCode*/) const {
    return (m_language == Language::C) ? "source.c" : "source.cpp";
}

QString ClangStrategy::outputFileName() const {
    return "output.s";
}

QStringList ClangStrategy::buildCompilerArgs(const CompilationRequest& req,
                                            const QString& inputFilePath,
                                            const QString& outputDirPath) const {
    QStringList args;
    args << defaultCompilerBinary(req.compilerImage);

    // Compilation to assembly
    args << "-S";
    args << "-fno-asynchronous-unwind-tables";

    // Syntax flavor: Clang supports -masm=intel or -masm=att
    // -masm= exists only on x86 targets (GCC rejects it on aarch64).
    if (req.targetsX86()) {
        if (req.syntaxFlavor == SyntaxFlavor::Intel) {
            args << "-masm=intel";
        } else {
            args << "-masm=att";
        }
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

    // Project mode: headers sit next to / below the main file inside the sandbox.
    if (!req.extraFiles.isEmpty()) {
        args << "-I/input";
        const int slash = req.mainFilePath.lastIndexOf('/');
        if (slash > 0) args << ("-I/input/" + req.mainFilePath.left(slash));
    }

    // Input and output
    args << inputFilePath;
    args << "-o";
    args << (outputDirPath + "/output.s");

    return args;
}

bool ClangStrategy::supportsSyntaxFlavor(SyntaxFlavor /*flavor*/) const {
    return true;
}

QStringList ClangStrategy::sanitizeUserFlags(const QStringList& rawFlags) const {
    return StrategyUtils::sanitizeUserFlags(rawFlags);
}

CompilationResult ClangStrategy::processOutput(const CompilationRequest& req,
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

QVector<Diagnostic> ClangStrategy::parseDiagnostics(const QString& compilerOutput) const {
    return StrategyUtils::parseClangGccDiagnostics(compilerOutput);
}

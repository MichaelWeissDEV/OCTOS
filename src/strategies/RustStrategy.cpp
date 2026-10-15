#include "RustStrategy.h"
#include "StrategyUtils.h"
#include <QRegularExpression>

RustCrateType RustStrategy::resolveCrateType(RustCrateType requested, const QString& sourceCode) {
    if (requested != RustCrateType::Auto) return requested;
    // `fn main(` not preceded by an identifier character (so `fn domain(` does not count).
    static const QRegularExpression mainRe(R"((?<![A-Za-z0-9_])fn\s+main\s*\()");
    return mainRe.match(sourceCode).hasMatch() ? RustCrateType::Binary : RustCrateType::Library;
}

QStringList RustStrategy::buildCompilerArgs(const CompilationRequest& req,
                                            const QString& inputFilePath,
                                            const QString& outputDirPath) const {
    QStringList args;
    args << defaultCompilerBinary(req.compilerImage);

    const QStringList sanitized = sanitizeUserFlags(StrategyUtils::splitCompilerArguments(req.userArgs));

    // Fold the user's flags into one value per setting so nothing is passed twice or contradictorily.
    QString userCrateType;
    QString userEdition;
    QString optLevel;          // from -O or -C opt-level=; the last one wins
    bool hasDebug = false;
    QStringList passthrough;

    for (int i = 0; i < sanitized.size(); ++i) {
        const QString& f = sanitized[i];
        // A flag's separate value must not itself look like a flag ("--crate-type -O" is a dangling
        // flag followed by -O, not a crate type called "-O").
        auto nextValue = [&](QString* out) {
            if (i + 1 < sanitized.size() && !sanitized[i + 1].startsWith('-')) { *out = sanitized[++i]; return true; }
            return false;
        };
        if (f == "--crate-type") { nextValue(&userCrateType); continue; }
        if (f.startsWith("--crate-type=")) { userCrateType = f.mid(13); continue; }
        if (f == "--edition") { nextValue(&userEdition); continue; }
        if (f.startsWith("--edition=")) { userEdition = f.mid(10); continue; }
        if (f == "--emit" || f.startsWith("--emit=")) {   // the output artifact is always assembly
            if (f == "--emit") { QString ignored; nextValue(&ignored); }
            continue;
        }
        if (f == "-O") { optLevel = "2"; continue; }
        if (f == "-C") {
            QString v;
            if (nextValue(&v)) {
                if (v.startsWith("opt-level=")) { optLevel = v.mid(10); continue; }
                if (v.startsWith("debuginfo")) { hasDebug = true; }
                passthrough << "-C" << v;
            }
            continue;
        }
        if (f.startsWith("-Copt-level=")) { optLevel = f.mid(12); continue; }
        if (f.startsWith("-Cdebuginfo")) { hasDebug = true; passthrough << f; continue; }
        if (f == "-g") { hasDebug = true; continue; }
        passthrough << f;
    }

    // Crate type: an explicit user choice wins over the request, which wins over auto-detection.
    QString crateType = userCrateType;
    if (crateType.isEmpty()) {
        crateType = resolveCrateType(req.rustCrateType, req.sourceCode) == RustCrateType::Library ? "lib" : "bin";
    }
    args << ("--crate-type=" + crateType);
    args << "--emit=asm";
    args << "--crate-name" << "octos_snippet";   // fixed name: independent of the file name, reproducible
    args << ("--edition=" + (userEdition.isEmpty() ? QString("2021") : userEdition));
    args << "-C" << "codegen-units=1";            // deterministic single assembly unit

    if (!optLevel.isEmpty()) args << "-C" << ("opt-level=" + optLevel);

    // Syntax flavor via LLVM args (x86 targets only; other targets ignore it)
    if (req.targetsX86()) {
        args << "-C" << (req.syntaxFlavor == SyntaxFlavor::Intel ? "llvm-args=--x86-asm-syntax=intel"
                                                                 : "llvm-args=--x86-asm-syntax=att");
    }

    // Debug info drives the source mapping (.loc); `-g` is added unless the user chose a level.
    if (!hasDebug) args << "-g";
    args.append(passthrough);

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

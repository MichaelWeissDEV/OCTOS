#include "RustStrategy.h"
#include "StrategyUtils.h"
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>

namespace {

bool isRustIdentifier(const QString& s) {
    static const QRegularExpression re(R"(^[A-Za-z_][A-Za-z0-9_]*$)");
    return re.match(s).hasMatch() && s != "_";
}

// Removes a TOML comment (`#` outside a string) from one line.
QString stripTomlComment(const QString& line) {
    QChar quote;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (!quote.isNull()) {
            if (c == '\\' && quote == '"') { ++i; continue; }
            if (c == quote) quote = QChar();
        } else if (c == '"' || c == '\'') {
            quote = c;
        } else if (c == '#') {
            return line.left(i);
        }
    }
    return line;
}

} // namespace

RustCrateType RustStrategy::resolveCrateType(RustCrateType requested, const QString& sourceCode,
                                             const QString& mainFilePath) {
    if (requested != RustCrateType::Auto) return requested;
    if (!mainFilePath.isEmpty()) {
        const QStringList segs = mainFilePath.split('/');
        if (segs.last() == "lib.rs") return RustCrateType::Library;
        if (segs.last() == "main.rs") return RustCrateType::Binary;
        if (segs.size() >= 2 && (segs[segs.size() - 2] == "bin" || segs[segs.size() - 2] == "examples"))
            return RustCrateType::Binary;
    }
    // `fn main(` not preceded by an identifier character (so `fn domain(` does not count).
    static const QRegularExpression mainRe(R"((?<![A-Za-z0-9_])fn\s+main\s*\()");
    return mainRe.match(sourceCode).hasMatch() ? RustCrateType::Binary : RustCrateType::Library;
}

RustStrategy::CrateInfo RustStrategy::crateInfo(const CompilationRequest& req) {
    CrateInfo info;
    if (req.mainFilePath.isEmpty()) return info;

    // Nearest manifest: the main file's directory first, then each parent up to the project root.
    QHash<QString, QString> files;
    for (const auto& f : req.extraFiles) files.insert(f.first, f.second);
    QStringList dir = req.mainFilePath.split('/');
    dir.removeLast();
    QString manifest;
    for (;;) {
        const QString candidate = dir.isEmpty() ? QString("Cargo.toml") : dir.join('/') + "/Cargo.toml";
        if (files.contains(candidate)) { info.manifestPath = candidate; manifest = files.value(candidate); break; }
        if (dir.isEmpty()) return info;
        dir.removeLast();
    }
    const QString manifestDir = dir.isEmpty() ? QString() : dir.join('/') + "/";

    static const QRegularExpression headerRe(R"(^\s*\[\[?\s*([^\]]+?)\s*\]\]?\s*$)");
    static const QRegularExpression keyValueRe(R"re(^\s*([A-Za-z0-9_.-]+)\s*=\s*(?:"([^"]*)"|'([^']*)'|(.*\S)))re");
    QString section;
    QString packageName, libName, edition, buildScript;
    bool hasDependencies = false;
    for (const QString& rawLine : manifest.split('\n')) {
        const QString line = stripTomlComment(rawLine).trimmed();
        if (line.isEmpty()) continue;
        const QRegularExpressionMatch h = headerRe.match(line);
        if (h.hasMatch()) {
            section = h.captured(1);
            // [dependencies.serde] / [target.'cfg(unix)'.dependencies] declare a dependency by themselves.
            if (section.startsWith("dependencies.") || section.contains(".dependencies")) hasDependencies = true;
            continue;
        }
        const QRegularExpressionMatch kv = keyValueRe.match(line);
        if (!kv.hasMatch()) continue;
        const QString key = kv.captured(1);
        const QString value = !kv.captured(2).isNull() ? kv.captured(2) : !kv.captured(3).isNull() ? kv.captured(3) : kv.captured(4);
        if (section == "dependencies" || section.endsWith(".dependencies")) hasDependencies = true;
        if (section == "package") {
            if (key == "name") packageName = value;
            else if (key == "edition") edition = value;
            else if (key == "build") buildScript = value;
        } else if (section == "lib" && key == "name") {
            libName = value;
        }
    }

    // Cargo's crate name: [lib] name for the library, the file stem for src/bin/<name>.rs, else the package name.
    const QStringList mainSegs = req.mainFilePath.split('/');
    const RustCrateType type = resolveCrateType(req.rustCrateType, req.sourceCode, req.mainFilePath);
    QString name = packageName;
    if (type == RustCrateType::Library && !libName.isEmpty()) name = libName;
    if (type == RustCrateType::Binary && mainSegs.size() >= 2 && mainSegs[mainSegs.size() - 2] == "bin")
        name = QFileInfo(mainSegs.last()).completeBaseName();
    name.replace('-', '_');
    if (!name.isEmpty()) {
        if (isRustIdentifier(name)) info.crateName = name;
        else info.warnings << QString("Crate name '%1' from %2 is not a valid identifier; using 'octos_snippet'.")
                                  .arg(name, info.manifestPath);
    }

    static const QRegularExpression editionRe(R"(^\d{4}$)");
    if (!edition.isEmpty()) {
        if (editionRe.match(edition).hasMatch()) info.edition = edition;
        else info.warnings << QString("Edition '%1' in %2 is not understood; using the default edition.")
                                  .arg(edition, info.manifestPath);
    }

    if (hasDependencies) {
        info.warnings << QString("%1 declares dependencies. OCTOS runs rustc on this crate alone, without Cargo, so "
                                 "external crates are not available and `use` of them will not resolve.")
                             .arg(info.manifestPath);
    }
    if (!buildScript.isEmpty() || files.contains(manifestDir + "build.rs")) {
        info.warnings << "The crate has a build script; OCTOS does not run it (generated code and cfg flags are missing).";
    }
    return info;
}

QStringList RustStrategy::requestWarnings(const CompilationRequest& req) const {
    return crateInfo(req).warnings;
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
        crateType = resolveCrateType(req.rustCrateType, req.sourceCode, req.mainFilePath) == RustCrateType::Library
                        ? "lib" : "bin";
    }
    // Single-file mode uses a fixed crate name (independent of the file name, reproducible); project mode takes
    // name and edition from the nearest Cargo.toml so symbols are mangled as in the real build.
    const CrateInfo crate = crateInfo(req);
    QString edition = userEdition;
    if (edition.isEmpty()) edition = crate.edition;
    if (edition.isEmpty()) edition = "2021";
    args << ("--crate-type=" + crateType);
    args << (req.outputKind == OutputKind::LlvmIr ? "--emit=llvm-ir" : "--emit=asm");
    args << "--crate-name" << crate.crateName;
    args << ("--edition=" + edition);
    args << "-C" << "codegen-units=1";            // deterministic single assembly unit

    if (!optLevel.isEmpty()) args << "-C" << ("opt-level=" + optLevel);

    // Syntax flavor via LLVM args (x86 targets only; other targets ignore it)
    if (req.outputKind == OutputKind::Assembly && req.targetsX86()) {
        args << "-C" << (req.syntaxFlavor == SyntaxFlavor::Intel ? "llvm-args=--x86-asm-syntax=intel"
                                                                 : "llvm-args=--x86-asm-syntax=att");
    }

    // Debug info drives the source mapping (.loc); `-g` is added unless the user chose a level.
    if (!hasDebug) args << "-g";
    args.append(passthrough);

    args << inputFilePath;
    args << "-o";
    args << (outputDirPath + "/" + outputFileFor(req));
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

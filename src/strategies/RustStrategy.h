#ifndef RUSTSTRATEGY_H
#define RUSTSTRATEGY_H

#include "ILanguageStrategy.h"

class RustStrategy : public ILanguageStrategy {
public:
    RustStrategy() = default;
    ~RustStrategy() override = default;

    Language language() const override { return Language::Rust; }
    QString languageId() const override { return "rust"; }
    QString defaultCompilerBinary(const QString& /*dockerImage*/) const override { return "rustc"; }
    QString sourceFileName(const QString& /*sourceCode*/) const override { return "source.rs"; }
    QString outputFileName() const override { return "output.s"; }
    QString outputFileFor(const CompilationRequest& req) const override {
        return req.outputKind == OutputKind::LlvmIr ? "output.ll" : "output.s";
    }
    bool supportsOutputKind(OutputKind, const CompilationRequest&) const override { return true; }

    QStringList buildCompilerArgs(const CompilationRequest& req,
                                  const QString& inputFilePath,
                                  const QString& outputDirPath) const override;

    bool supportsSyntaxFlavor(SyntaxFlavor flavor) const override;
    QStringList sanitizeUserFlags(const QStringList& rawFlags) const override;

    CompilationResult processOutput(const CompilationRequest& req,
                                    int exitCode,
                                    const QString& stdoutStr,
                                    const QString& stderrStr,
                                    const QString& outputFileContent) const override;

    QVector<Diagnostic> parseDiagnostics(const QString& compilerOutput) const override;

    QStringList requestWarnings(const CompilationRequest& req) const override;

    /// Resolves Auto. In project mode the crate root's path decides as Cargo does (`lib.rs` is a library,
    /// `main.rs`, `src/bin/*.rs` and `examples/*.rs` are binaries); otherwise Binary if the source declares
    /// `fn main`, else Library.
    static RustCrateType resolveCrateType(RustCrateType requested, const QString& sourceCode,
                                          const QString& mainFilePath = QString());

    /// What OCTOS takes from a project's Cargo manifest. OCTOS calls rustc directly, without Cargo: only the
    /// crate name and edition are used; dependencies and build scripts are not available (see warnings).
    struct CrateInfo {
        QString crateName{"octos_snippet"};
        QString edition;          // empty: OCTOS default (2021)
        QString manifestPath;     // project-relative Cargo.toml that was used; empty if none
        QStringList warnings;
    };
    /// Reads the Cargo.toml nearest to req.mainFilePath among req.extraFiles (project mode only).
    static CrateInfo crateInfo(const CompilationRequest& req);
};

#endif // RUSTSTRATEGY_H

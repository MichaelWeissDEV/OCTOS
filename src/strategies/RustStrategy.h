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

    /// Resolves Auto: Binary if the source declares a top-level `fn main`, otherwise Library.
    static RustCrateType resolveCrateType(RustCrateType requested, const QString& sourceCode);
};

#endif // RUSTSTRATEGY_H

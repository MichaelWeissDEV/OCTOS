#ifndef CLANGSTRATEGY_H
#define CLANGSTRATEGY_H

#include "ILanguageStrategy.h"
#include "StrategyUtils.h"

class ClangStrategy : public ILanguageStrategy {
public:
    explicit ClangStrategy(Language lang = Language::Cpp);
    ~ClangStrategy() override = default;

    Language language() const override;
    QString languageId() const override;
    QString defaultCompilerBinary(const QString& dockerImage) const override;
    QString sourceFileName(const QString& sourceCode) const override;
    bool acceptsSourceName(const QString& fileName) const override {
        return StrategyUtils::isSourceSuffixFor(language(), fileName);
    }
    QString outputFileName() const override;
    QString outputFileFor(const CompilationRequest& req) const override;
    bool supportsOutputKind(OutputKind kind, const CompilationRequest& req) const override;

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

private:
    Language m_language;
};

#endif // CLANGSTRATEGY_H

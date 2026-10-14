#ifndef ADASTRATEGY_H
#define ADASTRATEGY_H

#include "ILanguageStrategy.h"

class AdaStrategy : public ILanguageStrategy {
public:
    AdaStrategy() = default;
    ~AdaStrategy() override = default;

    Language language() const override { return Language::Ada; }
    QString languageId() const override { return "ada"; }
    QString defaultCompilerBinary(const QString& /*dockerImage*/) const override { return "gcc"; }
    QString sourceFileName(const QString& /*sourceCode*/) const override { return "source.adb"; }
    QString outputFileName() const override { return "output.s"; }

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
};

#endif // ADASTRATEGY_H

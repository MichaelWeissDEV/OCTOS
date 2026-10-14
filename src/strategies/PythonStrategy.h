#ifndef PYTHONSTRATEGY_H
#define PYTHONSTRATEGY_H

#include "ILanguageStrategy.h"

class PythonStrategy : public ILanguageStrategy {
public:
    PythonStrategy() = default;
    ~PythonStrategy() override = default;

    Language language() const override { return Language::Python; }
    QString languageId() const override { return "python"; }
    QString defaultCompilerBinary(const QString& /*dockerImage*/) const override { return "python3"; }
    QString sourceFileName(const QString& /*sourceCode*/) const override { return "source.py"; }
    QString outputFileName() const override { return "source.pyc"; }

    QStringList buildCompilerArgs(const CompilationRequest& req,
                                  const QString& inputFilePath,
                                  const QString& outputDirPath) const override;

    bool supportsSyntaxFlavor(SyntaxFlavor /*flavor*/) const override { return false; }
    QStringList sanitizeUserFlags(const QStringList& rawFlags) const override;

    CompilationResult processOutput(const CompilationRequest& req,
                                    int exitCode,
                                    const QString& stdoutStr,
                                    const QString& stderrStr,
                                    const QString& outputFileContent) const override;

    QVector<Diagnostic> parseDiagnostics(const QString& compilerOutput) const override;
};

#endif // PYTHONSTRATEGY_H

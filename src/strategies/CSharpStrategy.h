#ifndef CSHARPSTRATEGY_H
#define CSHARPSTRATEGY_H

#include "ILanguageStrategy.h"

class CSharpStrategy : public ILanguageStrategy {
public:
    CSharpStrategy() = default;
    ~CSharpStrategy() override = default;

    Language language() const override { return Language::CSharp; }
    QString languageId() const override { return "csharp"; }
    QString defaultCompilerBinary(const QString& /*dockerImage*/) const override { return "sh"; }
    QString sourceFileName(const QString& /*sourceCode*/) const override { return "Program.cs"; }
    QString outputFileName() const override { return "Program.il"; }

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

#endif // CSHARPSTRATEGY_H

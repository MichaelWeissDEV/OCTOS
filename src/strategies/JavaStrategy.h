#ifndef JAVASTRATEGY_H
#define JAVASTRATEGY_H

#include "ILanguageStrategy.h"

class JavaStrategy : public ILanguageStrategy {
public:
    JavaStrategy() = default;
    ~JavaStrategy() override = default;

    Language language() const override { return Language::Java; }
    QString languageId() const override { return "java"; }
    QString defaultCompilerBinary(const QString& /*dockerImage*/) const override { return "sh"; }
    QString sourceFileName(const QString& sourceCode) const override;
    QString outputFileName() const override { return "output.bytecode"; }

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

    static QString detectClassName(const QString& sourceCode);
};

#endif // JAVASTRATEGY_H

#ifndef ILANGUAGESTRATEGY_H
#define ILANGUAGESTRATEGY_H

#include <QString>
#include <QStringList>
#include <QVector>
#include "../core/CompilerEnums.h"
#include "../core/CompilationRequest.h"
#include "../core/CompilationResult.h"
#include "../core/Diagnostic.h"

class ILanguageStrategy {
public:
    virtual ~ILanguageStrategy() = default;

    virtual Language language() const = 0;
    virtual QString languageId() const = 0;
    virtual QString defaultCompilerBinary(const QString& dockerImage) const = 0;
    virtual QString sourceFileName(const QString& sourceCode) const = 0;
    virtual QString outputFileName() const = 0;

    virtual QStringList buildCompilerArgs(const CompilationRequest& req,
                                          const QString& inputFilePath,
                                          const QString& outputDirPath) const = 0;

    virtual bool supportsSyntaxFlavor(SyntaxFlavor flavor) const = 0;
    virtual QStringList sanitizeUserFlags(const QStringList& rawFlags) const = 0;

    virtual CompilationResult processOutput(const CompilationRequest& req,
                                            int exitCode,
                                            const QString& stdoutStr,
                                            const QString& stderrStr,
                                            const QString& outputFileContent) const = 0;

    virtual QVector<Diagnostic> parseDiagnostics(const QString& compilerOutput) const = 0;
};

#endif // ILANGUAGESTRATEGY_H

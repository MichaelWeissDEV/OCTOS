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
    // Project mode keeps the main file's own name inside the sandbox when the compiler recognises it as this
    // language (otherwise the canonical sourceFileName() is used in the same directory).
    virtual bool acceptsSourceName(const QString& fileName) const {
        const QString canonical = sourceFileName(QString());
        const int a = fileName.lastIndexOf('.');
        const int b = canonical.lastIndexOf('.');
        return a > 0 && b >= 0 && fileName.mid(a) == canonical.mid(b);
    }
    // File the compiler writes for this request (differs from outputFileName() for non-default output kinds).
    virtual QString outputFileFor(const CompilationRequest& /*req*/) const { return outputFileName(); }
    // Which output kinds this strategy can produce for the given request (image/language specifics).
    virtual bool supportsOutputKind(OutputKind kind, const CompilationRequest& /*req*/) const {
        return kind == OutputKind::Assembly;
    }

    // Things the user should know about how this request is compiled (e.g. ignored Cargo dependencies).
    // The backend appends them to the result's warnings.
    virtual QStringList requestWarnings(const CompilationRequest& /*req*/) const { return {}; }

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

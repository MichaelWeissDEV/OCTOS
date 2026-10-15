#ifndef COMPILATIONREQUEST_H
#define COMPILATIONREQUEST_H

#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>
#include "CompilerEnums.h"

struct CompilationRequest {
    quint64 jobId{0};
    quint64 paneId{0};
    quint64 revision{0};
    QString sourceCode;
    QString compilerImage;
    Language language{Language::Cpp};
    SyntaxFlavor syntaxFlavor{SyntaxFlavor::Intel};
    QString userArgs;
    // Additional project files (headers, modules) placed next to the main source inside the sandbox.
    // Paths must be relative and are validated by the backend; they count towards the source size limit.
    QVector<QPair<QString, QString>> extraFiles;
    // Architecture the compiler inside the image targets ("x86_64", "aarch64", ...). Empty = unknown, treated
    // as x86-64. x86-only options (-masm=, x86 asm syntax) are only passed for x86 targets.
    QString targetArchitecture;
    bool targetsX86() const {
        const QString a = targetArchitecture.toLower();
        return a.isEmpty() || a.contains("x86") || a.contains("amd64") || a.contains("i386") || a.contains("i686");
    }
    QString mainFilePath;   // project-relative path of the main source (used to derive include directories)
    OutputKind outputKind{OutputKind::Assembly};   // honoured only if the strategy supports it
    RustCrateType rustCrateType{RustCrateType::Auto}; // only used for Language::Rust
    int timeoutMs{30000};
};

#endif // COMPILATIONREQUEST_H

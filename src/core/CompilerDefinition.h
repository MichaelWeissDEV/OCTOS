#ifndef COMPILERDEFINITION_H
#define COMPILERDEFINITION_H

#include <QString>
#include <QStringList>
#include <QVector>
#include "CompilerEnums.h"
#include "CompilationArtifact.h"

enum class CompilerFamily {
    GCC,
    Clang,
    Rustc,
    OpenJDK,
    Python,
    Mono,
    GNAT,
    Go,
    Zig,
    Generic
};

inline QString compilerFamilyToString(CompilerFamily family) {
    switch (family) {
        case CompilerFamily::GCC: return "GCC";
        case CompilerFamily::Clang: return "Clang";
        case CompilerFamily::Rustc: return "rustc";
        case CompilerFamily::OpenJDK: return "OpenJDK";
        case CompilerFamily::Python: return "CPython";
        case CompilerFamily::Mono: return "Mono";
        case CompilerFamily::GNAT: return "GNAT";
        case CompilerFamily::Go: return "Go";
        case CompilerFamily::Zig: return "Zig";
        case CompilerFamily::Generic: return "Generic";
    }
    return "Unknown";
}

struct CompilerDefinition {
    QString id;
    QString displayName;
    CompilerFamily family{CompilerFamily::Generic};
    Language language{Language::Cpp};
    QString version;
    QString containerImage;
    QString imageDigest;
    QString architecture{"x86_64"};
    QVector<ArtifactType> supportedArtifactTypes;
    QVector<SyntaxFlavor> supportedSyntaxFlavors;
    QStringList availableOptimizationOptions;
    QStringList requiredBinaries;
    bool isExperimental{false};
    QString statusMessage;

    bool supportsSyntax(SyntaxFlavor flavor) const {
        return supportedSyntaxFlavors.contains(flavor);
    }

    bool supportsArtifact(ArtifactType type) const {
        return supportedArtifactTypes.contains(type);
    }
};

#endif // COMPILERDEFINITION_H

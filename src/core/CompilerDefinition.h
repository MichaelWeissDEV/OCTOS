#ifndef COMPILERDEFINITION_H
#define COMPILERDEFINITION_H

#include <QString>
#include <QStringList>
#include <QVector>
#include "CompilerEnums.h"
#include "CompilationArtifact.h"
#include "ToolchainProbe.h"

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

// Three separate facts, never conflated:
//   ImageRecognized    - the image name was matched/inferred (says nothing about its contents)
//   ToolchainPlausible - metadata says a complete toolchain is expected (still unproven)
//   Verified           - a probe inside the image ran and found the required tools + a version
//   VerificationFailed - a probe ran and failed (tools missing, no version, cannot run)
enum class VerificationState {
    ImageRecognized,
    ToolchainPlausible,
    Verified,
    VerificationFailed
};

inline QString verificationStateToString(VerificationState s) {
    switch (s) {
        case VerificationState::ImageRecognized: return "Image recognized";
        case VerificationState::ToolchainPlausible: return "Plausible (not verified)";
        case VerificationState::Verified: return "Verified";
        case VerificationState::VerificationFailed: return "Verification failed";
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

    // Verification
    QVector<Language> supportedLanguages;
    ToolchainProbeResult probe;        // last probe run on this image, if any
    QString currentImageId;            // ID of the image as installed right now (empty if unknown)

    // A stored probe only counts while the image it ran against is still the installed one.
    bool probeIsCurrent() const {
        return probe.performed && (currentImageId.isEmpty() || probe.imageDigest.isEmpty() ||
                                   probe.imageDigest == currentImageId);
    }

    VerificationState verificationState() const {
        if (probeIsCurrent()) {
            return (probe.success && probe.missingBinaries.isEmpty() && !probe.version.isEmpty())
                       ? VerificationState::Verified
                       : VerificationState::VerificationFailed;
        }
        if (family != CompilerFamily::Generic && !isExperimental && !requiredBinaries.isEmpty())
            return VerificationState::ToolchainPlausible;
        return VerificationState::ImageRecognized;
    }

    // Version as reported by the compiler itself; empty unless verified.
    QString actualVersion() const {
        return verificationState() == VerificationState::Verified ? probe.version : QString();
    }

    bool supportsSyntax(SyntaxFlavor flavor) const {
        return supportedSyntaxFlavors.contains(flavor);
    }

    bool supportsArtifact(ArtifactType type) const {
        return supportedArtifactTypes.contains(type);
    }
};

#endif // COMPILERDEFINITION_H

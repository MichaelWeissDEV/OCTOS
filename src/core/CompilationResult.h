#ifndef COMPILATIONRESULT_H
#define COMPILATIONRESULT_H

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>
#include "CompilerEnums.h"
#include "Diagnostic.h"
#include "CompilationArtifact.h"

// Who is responsible for a failed job. Lets the UI distinguish a compiler error
// from a Docker problem or an internal OCTOS fault.
enum class FailureOrigin {
    None,
    Compiler,   // the toolchain ran and reported errors
    Docker,     // docker client/daemon/image problem
    Limits,     // size/output/timeout limits enforced by OCTOS
    Internal    // OCTOS itself (temp dirs, strategy missing, backend exception)
};

inline QString failureOriginToString(FailureOrigin o) {
    switch (o) {
        case FailureOrigin::None: return "None";
        case FailureOrigin::Compiler: return "Compiler";
        case FailureOrigin::Docker: return "Docker";
        case FailureOrigin::Limits: return "Limits";
        case FailureOrigin::Internal: return "Internal";
    }
    return "Unknown";
}

struct CompilationResult {
    quint64 jobId{0};
    quint64 paneId{0};
    quint64 revision{0};
    JobState state{JobState::Queued};
    int exitCode{-1};
    QString rawOutput;
    QString assemblyOutput;
    QString errorOutput;
    QVector<Diagnostic> diagnostics;
    qint64 durationMs{0};

    // Reproducibility & Environment Metadata
    QString compilerImage;
    QString imageDigest;
    QString architecture;
    QString compilerVersion;      // as reported by the compiler inside the image (only if the image was verified)
    QString compilerCommand;      // exact compiler invocation inside the container (reproducibility)

    // Truncation Tracking
    bool stdoutTruncated{false};
    bool stderrTruncated{false};
    bool artifactTruncated{false};

    FailureOrigin failureOrigin{FailureOrigin::None};
    // User-visible, non-fatal notes (truncation, container cleanup problems, ...).
    QStringList warnings;

    // Structured Artifacts
    QVector<CompilationArtifact> artifacts;

    bool success() const {
        return state == JobState::Completed && exitCode == 0;
    }

    // True only if the compilation succeeded and nothing was cut off along the way.
    bool isComplete() const {
        return success() && !stdoutTruncated && !stderrTruncated && !artifactTruncated;
    }

    bool hasArtifact(ArtifactType type) const {
        for (const auto& art : artifacts) {
            if (art.type == type) return true;
        }
        return false;
    }

    CompilationArtifact artifact(ArtifactType type) const {
        for (const auto& art : artifacts) {
            if (art.type == type) return art;
        }
        return CompilationArtifact();
    }

    void addArtifact(const CompilationArtifact& art) {
        artifacts.append(art);
    }
};

Q_DECLARE_METATYPE(CompilationResult);

#endif // COMPILATIONRESULT_H

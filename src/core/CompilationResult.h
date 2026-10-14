#ifndef COMPILATIONRESULT_H
#define COMPILATIONRESULT_H

#include <QMetaType>
#include <QString>
#include <QVector>
#include "CompilerEnums.h"
#include "Diagnostic.h"
#include "CompilationArtifact.h"

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
    QString compilerVersion;

    // Truncation Tracking
    bool stdoutTruncated{false};
    bool stderrTruncated{false};
    bool artifactTruncated{false};

    // Structured Artifacts
    QVector<CompilationArtifact> artifacts;

    bool success() const {
        return state == JobState::Completed && exitCode == 0;
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

#ifndef COMPILATIONRESULT_H
#define COMPILATIONRESULT_H

#include <QMetaType>
#include <QString>
#include <QVector>
#include "CompilerEnums.h"
#include "Diagnostic.h"

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

    bool success() const {
        return state == JobState::Completed && exitCode == 0;
    }
};

Q_DECLARE_METATYPE(CompilationResult);

#endif // COMPILATIONRESULT_H

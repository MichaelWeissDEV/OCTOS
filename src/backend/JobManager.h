#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <QMutex>
#include <QObject>
#include <QMap>
#include <QSet>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include "../core/CompilationRequest.h"
#include "../core/CompilationResult.h"
#include "IExecutionBackend.h"

class JobTask;

class JobManager : public QObject {
    Q_OBJECT

public:
    explicit JobManager(std::shared_ptr<IExecutionBackend> backend = nullptr,
                        QObject* parent = nullptr);
    ~JobManager() override;

    quint64 submitJob(CompilationRequest req);
    void cancelJob(quint64 jobId);
    void cancelPaneJobs(quint64 paneId);
    void invalidatePane(quint64 paneId);

    void setMaxParallelJobs(int maxJobs);
    int maxParallelJobs() const;

    int activeJobCount() const;
    quint64 getPaneCurrentRevision(quint64 paneId) const;

    void shutdown();

signals:
    void jobStateChanged(quint64 jobId, quint64 paneId, JobState state);
    void jobFinished(const CompilationResult& result);

private slots:
    void handleTaskFinished(const CompilationResult& result);

private:
    friend class JobTask;

    std::shared_ptr<IExecutionBackend> m_backend;
    QThreadPool m_threadPool;
    mutable QMutex m_mutex;
    std::atomic<quint64> m_nextJobId{1};

    QMap<quint64, quint64> m_paneCurrentRevision;
    QMap<quint64, QSet<quint64>> m_paneActiveJobs;
    QMap<quint64, std::shared_ptr<std::atomic_bool>> m_jobCancelledFlags;
    std::atomic_bool m_isShuttingDown{false};
};

#endif // JOBMANAGER_H

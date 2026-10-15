#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <QHash>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include <utility>
#include "../core/CompilationRequest.h"
#include "../core/CompilationResult.h"
#include "IExecutionBackend.h"

class JobTask;

/**
 * Bounded job scheduler in front of a QThreadPool.
 *
 * Jobs wait in an internal queue (at most one waiting job per pane, at most
 * maxPendingJobs() in total) and are only handed to the thread pool while fewer
 * than maxParallelJobs() are in flight. Consequently the pool never holds an
 * unbounded backlog of unstarted QRunnables.
 *
 * Job state transitions:
 *   Queued -> Cancelled                                  (removed while waiting)
 *   Queued -> Starting -> Running -> Completed|Failed|TimedOut|Cancelled
 *
 * Only the newest job of a pane (and only if its revision is not superseded)
 * ever produces jobFinished().
 */
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
    void setMaxPendingJobs(int maxJobs);
    int maxPendingJobs() const;

    /// Pending + in-flight jobs that have not reported back yet.
    int activeJobCount() const;
    int pendingJobCount() const;
    int runningJobCount() const;
    quint64 getPaneCurrentRevision(quint64 paneId) const;
    quint64 getPaneCurrentJobId(quint64 paneId) const;

    /**
     * Stops accepting jobs, drops queued jobs, cancels running jobs and waits
     * (up to timeoutMs, re-sending cancellation) until every worker has
     * actually returned. Returns false if some workers did not terminate; in
     * that case shared resources stay alive for them (see ~JobManager).
     */
    bool shutdown(int timeoutMs = 10000);
    /// Upper bound the destructor waits for workers (default 10 s).
    void setShutdownTimeoutMs(int ms) { m_shutdownTimeoutMs = ms; }
    bool isShutdown() const { return m_isShuttingDown.load(std::memory_order_acquire); }

signals:
    void jobStateChanged(quint64 jobId, quint64 paneId, JobState state);
    void jobFinished(const CompilationResult& result);

private slots:
    void handleTaskFinished(const CompilationResult& result);
    // Processes the messages workers left in the inbox (see JobManager.cpp).
    void drainInbox();

private:
    friend class JobTask;

    struct Shared;  // worker bookkeeping that outlives the manager if needed

    struct JobRecord {
        CompilationRequest req;
        std::shared_ptr<std::atomic_bool> cancelFlag;
        bool dispatched{false};
    };

    struct Dispatch {
        quint64 jobId;
        quint64 paneId;
        JobTask* task;
    };

    // Called with m_mutex held; collects work to be performed after unlocking.
    void removePendingLocked(quint64 jobId);
    void pumpLocked(QList<Dispatch>& out);
    void finishDispatch(const QList<Dispatch>& dispatches);
    using DroppedJob = std::pair<CompilationRequest, bool>; // request, deliver result to UI
    void notifyCancelledPending(const QList<DroppedJob>& dropped);
    void cancelDispatched(const QList<quint64>& ids);

    std::shared_ptr<IExecutionBackend> m_backend;
    std::unique_ptr<QThreadPool> m_threadPool;
    std::shared_ptr<Shared> m_shared;
    mutable QMutex m_mutex;
    std::atomic<quint64> m_nextJobId{1};

    int m_maxRunning{1};
    int m_maxPending{32};
    int m_running{0};
    int m_shutdownTimeoutMs{10000};

    QHash<quint64, JobRecord> m_records;
    QList<quint64> m_pending;                 // FIFO of job ids waiting for a slot
    QHash<quint64, quint64> m_panePendingJob; // pane -> waiting job
    QHash<quint64, quint64> m_paneCurrentRevision;
    QHash<quint64, quint64> m_paneCurrentJobId;
    std::atomic_bool m_isShuttingDown{false};
};

#endif // JOBMANAGER_H

#include "JobManager.h"
#include "DockerExecutionBackend.h"
#include "../core/AppSettings.h"
#include <QMetaObject>
#include <QMutexLocker>
#include <QPointer>
#include <QRunnable>

class JobTask : public QRunnable {
public:
    JobTask(CompilationRequest req,
            std::shared_ptr<IExecutionBackend> backend,
            std::shared_ptr<std::atomic_bool> cancelledFlag,
            QPointer<JobManager> owner)
        : m_req(std::move(req)),
          m_backend(std::move(backend)),
          m_cancelledFlag(std::move(cancelledFlag)),
          m_owner(owner) {
        setAutoDelete(true);
    }

    void run() override {
        if (!m_backend || m_cancelledFlag->load(std::memory_order_acquire)) {
            CompilationResult res;
            res.jobId = m_req.jobId;
            res.paneId = m_req.paneId;
            res.revision = m_req.revision;
            res.state = JobState::Cancelled;
            res.errorOutput = "Compilation cancelled";
            dispatchResult(res);
            return;
        }

        if (m_owner) {
            quint64 jId = m_req.jobId;
            quint64 pId = m_req.paneId;
            QPointer<JobManager> owner = m_owner;
            QMetaObject::invokeMethod(owner, [owner, jId, pId]() {
                if (owner) emit owner->jobStateChanged(jId, pId, JobState::Running);
            }, Qt::QueuedConnection);
        }

        auto isCancelled = [flag = m_cancelledFlag]() {
            return flag->load(std::memory_order_acquire);
        };

        CompilationResult result = m_backend->execute(m_req, isCancelled);

        if (m_cancelledFlag->load(std::memory_order_acquire)) {
            result.state = JobState::Cancelled;
            result.errorOutput = "Compilation cancelled";
        }

        dispatchResult(result);
    }

private:
    void dispatchResult(const CompilationResult& result) {
        if (!m_owner) return;
        QPointer<JobManager> owner = m_owner;
        QMetaObject::invokeMethod(owner, [owner, result]() {
            if (owner) owner->handleTaskFinished(result);
        }, Qt::QueuedConnection);
    }

    CompilationRequest m_req;
    std::shared_ptr<IExecutionBackend> m_backend;
    std::shared_ptr<std::atomic_bool> m_cancelledFlag;
    QPointer<JobManager> m_owner;
};

JobManager::JobManager(std::shared_ptr<IExecutionBackend> backend, QObject* parent)
    : QObject(parent),
      m_backend(std::move(backend)) {
    if (!m_backend) {
        m_backend = std::make_shared<DockerExecutionBackend>();
    }

    int threadCount = AppSettings::instance().maxParallelJobs();
    m_threadPool.setMaxThreadCount(threadCount);
    m_threadPool.setExpiryTimeout(10000);
}

JobManager::~JobManager() {
    shutdown();
}

void JobManager::shutdown() {
    m_isShuttingDown.store(true, std::memory_order_release);

    QList<quint64> jobIdsToCancel;
    {
        QMutexLocker locker(&m_mutex);
        jobIdsToCancel = m_jobCancelledFlags.keys();
        for (const auto& flag : m_jobCancelledFlags) {
            if (flag) {
                flag->store(true, std::memory_order_release);
            }
        }
    }

    if (m_backend) {
        for (quint64 jobId : jobIdsToCancel) {
            m_backend->cancel(jobId);
        }
    }

    m_threadPool.clear();
    m_threadPool.waitForDone(5000);

    QMutexLocker locker(&m_mutex);
    m_jobCancelledFlags.clear();
    m_paneActiveJobs.clear();
}

quint64 JobManager::submitJob(CompilationRequest req) {
    if (m_isShuttingDown.load(std::memory_order_acquire)) {
        return 0;
    }

    quint64 jobId = m_nextJobId.fetch_add(1);
    req.jobId = jobId;

    auto cancelFlag = std::make_shared<std::atomic_bool>(false);

    QList<quint64> oldJobsToCancel;
    {
        QMutexLocker locker(&m_mutex);

        // Track latest revision for this pane
        m_paneCurrentRevision[req.paneId] = req.revision;

        // Cancel previous active jobs for the same pane
        if (m_paneActiveJobs.contains(req.paneId)) {
            for (quint64 oldJobId : m_paneActiveJobs[req.paneId]) {
                if (m_jobCancelledFlags.contains(oldJobId)) {
                    m_jobCancelledFlags[oldJobId]->store(true, std::memory_order_release);
                    oldJobsToCancel.append(oldJobId);
                }
            }
        }

        m_paneActiveJobs[req.paneId].insert(jobId);
        m_jobCancelledFlags[jobId] = cancelFlag;
    }

    // Cancel old jobs in the backend
    if (m_backend) {
        for (quint64 oldId : oldJobsToCancel) {
            m_backend->cancel(oldId);
        }
    }

    emit jobStateChanged(jobId, req.paneId, JobState::Queued);

    auto* task = new JobTask(req, m_backend, cancelFlag, this);
    m_threadPool.start(task);

    return jobId;
}

void JobManager::cancelJob(quint64 jobId) {
    {
        QMutexLocker locker(&m_mutex);
        if (m_jobCancelledFlags.contains(jobId)) {
            m_jobCancelledFlags[jobId]->store(true, std::memory_order_release);
        }
    }
    if (m_backend) {
        m_backend->cancel(jobId);
    }
}

void JobManager::cancelPaneJobs(quint64 paneId) {
    QList<quint64> jobsToCancel;
    {
        QMutexLocker locker(&m_mutex);
        if (m_paneActiveJobs.contains(paneId)) {
            for (quint64 jId : m_paneActiveJobs[paneId]) {
                if (m_jobCancelledFlags.contains(jId)) {
                    m_jobCancelledFlags[jId]->store(true, std::memory_order_release);
                    jobsToCancel.append(jId);
                }
            }
            m_paneActiveJobs.remove(paneId);
        }
    }
    if (m_backend) {
        for (quint64 jId : jobsToCancel) {
            m_backend->cancel(jId);
        }
    }
}

void JobManager::invalidatePane(quint64 paneId) {
    cancelPaneJobs(paneId);
    QMutexLocker locker(&m_mutex);
    m_paneCurrentRevision.remove(paneId);
}

void JobManager::setMaxParallelJobs(int maxJobs) {
    if (maxJobs < 1) maxJobs = 1;
    m_threadPool.setMaxThreadCount(maxJobs);
}

int JobManager::maxParallelJobs() const {
    return m_threadPool.maxThreadCount();
}

int JobManager::activeJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_jobCancelledFlags.size();
}

quint64 JobManager::getPaneCurrentRevision(quint64 paneId) const {
    QMutexLocker locker(&m_mutex);
    return m_paneCurrentRevision.value(paneId, 0);
}

void JobManager::handleTaskFinished(const CompilationResult& result) {
    bool isObsolete = false;
    {
        QMutexLocker locker(&m_mutex);
        m_jobCancelledFlags.remove(result.jobId);
        if (m_paneActiveJobs.contains(result.paneId)) {
            m_paneActiveJobs[result.paneId].remove(result.jobId);
            if (m_paneActiveJobs[result.paneId].isEmpty()) {
                m_paneActiveJobs.remove(result.paneId);
            }
        }

        quint64 currentRev = m_paneCurrentRevision.value(result.paneId, 0);
        if (result.revision < currentRev) {
            // A newer revision exists for this pane! Discard obsolete result!
            isObsolete = true;
        }
    }

    emit jobStateChanged(result.jobId, result.paneId, result.state);

    if (!isObsolete) {
        emit jobFinished(result);
    }
}

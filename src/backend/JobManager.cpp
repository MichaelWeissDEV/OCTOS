#include "JobManager.h"
#include "DockerExecutionBackend.h"
#include "../core/AppSettings.h"
#include "../utils/TsanAnnotations.h"
#include <deque>
#include <QMetaObject>
#include <QMutexLocker>
#include <QPointer>
#include <QRunnable>
#include <QElapsedTimer>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>

// Bookkeeping shared between the manager and its workers. Workers hold a
// shared_ptr, so this stays valid even if a worker outlives the manager.
struct JobManager::Shared {
    std::mutex mutex;
    std::condition_variable cv;
    int liveWorkers{0};  // dispatched tasks that have not finished run() yet
    bool closed{false};  // manager is gone / going: never touch it again

    // Worker -> GUI hand-off. Messages travel through this mutex-protected queue (a synchronisation
    // ThreadSanitizer sees); only a data-free "drain" call is posted through Qt's event queue.
    struct Message {
        bool isResult{false};
        quint64 jobId{0};
        quint64 paneId{0};
        JobState state{JobState::Queued};
        CompilationResult result;
    };
    std::mutex inboxMutex;
    std::deque<Message> inbox;
    std::atomic_bool drainPending{false};
};

namespace {
CompilationResult makeCancelledResult(const CompilationRequest& req, const QString& msg) {
    CompilationResult res;
    res.jobId = req.jobId;
    res.paneId = req.paneId;
    res.revision = req.revision;
    res.state = JobState::Cancelled;
    res.errorOutput = msg;
    return res;
}
}

class JobTask : public QRunnable {
public:
    JobTask(CompilationRequest req,
            std::shared_ptr<IExecutionBackend> backend,
            std::shared_ptr<std::atomic_bool> cancelledFlag,
            std::shared_ptr<JobManager::Shared> shared,
            QPointer<JobManager> owner)
        : m_req(std::move(req)),
          m_backend(std::move(backend)),
          m_cancelledFlag(std::move(cancelledFlag)),
          m_shared(std::move(shared)),
          m_owner(owner) {
        setAutoDelete(true);
    }

    void run() override {
        OCTOS_TSAN_ACQUIRE(this);   // pairs with the release in JobManager::finishDispatch()
        // RAII: whatever happens, the worker is accounted for exactly once,
        // and only AFTER the result has been posted.
        struct WorkerGuard {
            std::shared_ptr<JobManager::Shared> s;
            ~WorkerGuard() {
                std::lock_guard<std::mutex> l(s->mutex);
                --s->liveWorkers;
                s->cv.notify_all();
            }
        } guard{m_shared};

        CompilationResult result;
        if (!m_backend || m_cancelledFlag->load(std::memory_order_acquire)) {
            result = makeCancelledResult(m_req, "Compilation cancelled");
            dispatchResult(result);
            return;
        }

        postState(JobState::Running);

        auto isCancelled = [flag = m_cancelledFlag]() {
            return flag->load(std::memory_order_acquire);
        };

        try {
            result = m_backend->execute(m_req, isCancelled);
        } catch (const std::exception& e) {
            result = CompilationResult();
            result.jobId = m_req.jobId;
            result.paneId = m_req.paneId;
            result.revision = m_req.revision;
            result.state = JobState::Failed;
            result.exitCode = -1;
            result.errorOutput = QString("Internal error in execution backend: %1").arg(e.what());
        } catch (...) {
            result = CompilationResult();
            result.jobId = m_req.jobId;
            result.paneId = m_req.paneId;
            result.revision = m_req.revision;
            result.state = JobState::Failed;
            result.exitCode = -1;
            result.errorOutput = "Internal error in execution backend";
        }

        // Normalise identity: a backend must never be able to misattribute a result.
        result.jobId = m_req.jobId;
        result.paneId = m_req.paneId;
        result.revision = m_req.revision;

        if (m_cancelledFlag->load(std::memory_order_acquire)) {
            result.state = JobState::Cancelled;
            result.errorOutput = "Compilation cancelled";
        }

        dispatchResult(result);
    }

private:
    // Messages go into the inbox; the owner is only poked (without data) when no drain is pending.
    // Posting happens under the shared mutex so it cannot race with the manager being destroyed
    // (the destructor sets `closed` under that mutex).
    void post(JobManager::Shared::Message msg) {
        {
            std::lock_guard<std::mutex> l(m_shared->inboxMutex);
            m_shared->inbox.push_back(std::move(msg));
        }
        std::lock_guard<std::mutex> l(m_shared->mutex);
        if (m_shared->closed || !m_owner) return;
        if (m_shared->drainPending.exchange(true)) return;   // a drain call is already on its way
        JobManager* owner = m_owner.data();                   // alive: `closed` is set before destruction
        QMetaObject::invokeMethod(owner, "drainInbox", Qt::QueuedConnection);
    }

    void postState(JobState state) {
        JobManager::Shared::Message m;
        m.isResult = false;
        m.jobId = m_req.jobId;
        m.paneId = m_req.paneId;
        m.state = state;
        post(std::move(m));
    }

    void dispatchResult(const CompilationResult& result) {
        JobManager::Shared::Message m;
        m.isResult = true;
        m.jobId = result.jobId;
        m.paneId = result.paneId;
        m.state = result.state;
        m.result = result;
        post(std::move(m));
    }

    CompilationRequest m_req;
    std::shared_ptr<IExecutionBackend> m_backend;
    std::shared_ptr<std::atomic_bool> m_cancelledFlag;
    std::shared_ptr<JobManager::Shared> m_shared;
    QPointer<JobManager> m_owner;
};

JobManager::JobManager(std::shared_ptr<IExecutionBackend> backend, QObject* parent)
    : QObject(parent),
      m_backend(std::move(backend)),
      m_threadPool(new QThreadPool),
      m_shared(std::make_shared<Shared>()) {
    if (!m_backend) {
        DockerExecutionConfig cfg = DockerExecutionConfig::fromSettings(AppSettings::instance());
        m_backend = std::make_shared<DockerExecutionBackend>(cfg);
    }

    m_maxRunning = qMax(1, AppSettings::instance().maxParallelJobs());
    m_threadPool->setMaxThreadCount(m_maxRunning);
    m_threadPool->setExpiryTimeout(10000);
}

JobManager::~JobManager() {
    const bool clean = shutdown(m_shutdownTimeoutMs);
    {
        std::lock_guard<std::mutex> l(m_shared->mutex);
        m_shared->closed = true;
    }
    if (!clean) {
        // Workers are still running and may touch the thread pool. Destroying
        // it would block indefinitely (waitForDone), so it is intentionally
        // leaked; workers only hold shared_ptrs (backend, Shared) and see
        // `closed`, so nothing they use is destroyed under them.
        m_threadPool.release();
    }
}

bool JobManager::shutdown(int timeoutMs) {
    // 1. Refuse any new jobs
    m_isShuttingDown.store(true, std::memory_order_release);

    // 2. Drop waiting jobs, 3. invalidate + cancel dispatched ones
    QList<DroppedJob> dropped;
    QList<quint64> toCancel;
    {
        QMutexLocker locker(&m_mutex);
        for (quint64 id : std::as_const(m_pending)) {
            auto it = m_records.find(id);
            if (it != m_records.end()) {
                dropped.append({it->req, false});
                m_records.erase(it);
            }
        }
        m_pending.clear();
        m_panePendingJob.clear();
        for (auto it = m_records.begin(); it != m_records.end(); ++it) {
            it->cancelFlag->store(true, std::memory_order_release);
            toCancel.append(it.key());
        }
        // Nothing may be delivered to the UI any more.
        m_paneCurrentJobId.clear();
        m_paneCurrentRevision.clear();
    }
    notifyCancelledPending(dropped);

    // 4./5./6. Cancel in the backend (kills docker clients / containers) outside the mutex.
    cancelDispatched(toCancel);

    // 7. Wait for the workers to actually return - observed, not assumed.
    QElapsedTimer timer;
    timer.start();
    bool clean = false;
    for (;;) {
        {
            std::unique_lock<std::mutex> l(m_shared->mutex);
            if (m_shared->liveWorkers == 0) { clean = true; break; }
            if (timer.elapsed() >= timeoutMs) break;
            m_shared->cv.wait_for(l, std::chrono::milliseconds(250), [&] { return m_shared->liveWorkers == 0; });
            if (m_shared->liveWorkers == 0) { clean = true; break; }
        }
        // Re-signal cancellation: a job may have registered with the backend after the first cancel.
        cancelDispatched(toCancel);
    }

    if (clean) {
        QMutexLocker locker(&m_mutex);
        m_records.clear();
        m_running = 0;
    }
    return clean;
}

void JobManager::cancelDispatched(const QList<quint64>& ids) {
    if (!m_backend) return;
    for (quint64 id : ids) m_backend->cancel(id);
}

void JobManager::removePendingLocked(quint64 jobId) {
    m_pending.removeOne(jobId);
    auto it = m_records.find(jobId);
    if (it != m_records.end()) {
        if (m_panePendingJob.value(it->req.paneId) == jobId) m_panePendingJob.remove(it->req.paneId);
        m_records.erase(it);
    }
}

void JobManager::notifyCancelledPending(const QList<DroppedJob>& dropped) {
    for (const auto& d : dropped) {
        emit jobStateChanged(d.first.jobId, d.first.paneId, JobState::Cancelled);
        if (d.second) {
            emit jobFinished(makeCancelledResult(d.first, "Compilation cancelled"));
        }
    }
}

void JobManager::pumpLocked(QList<Dispatch>& out) {
    if (m_isShuttingDown.load(std::memory_order_acquire)) return;
    while (m_running < m_maxRunning && !m_pending.isEmpty()) {
        const quint64 id = m_pending.takeFirst();
        auto it = m_records.find(id);
        if (it == m_records.end()) continue;
        if (m_panePendingJob.value(it->req.paneId) == id) m_panePendingJob.remove(it->req.paneId);
        it->dispatched = true;
        ++m_running;
        {
            std::lock_guard<std::mutex> l(m_shared->mutex);
            ++m_shared->liveWorkers;
        }
        auto* task = new JobTask(it->req, m_backend, it->cancelFlag, m_shared, this);
        out.append({id, it->req.paneId, task});
    }
}

void JobManager::finishDispatch(const QList<Dispatch>& dispatches) {
    for (const auto& d : dispatches) {
        emit jobStateChanged(d.jobId, d.paneId, JobState::Starting);
        OCTOS_TSAN_RELEASE(d.task);   // the task is fully built; the worker acquires it in run()
        m_threadPool->start(d.task);
    }
}

void JobManager::drainInbox() {
    m_shared->drainPending.store(false);
    for (;;) {
        Shared::Message msg;
        {
            std::lock_guard<std::mutex> l(m_shared->inboxMutex);
            if (m_shared->inbox.empty()) return;
            msg = std::move(m_shared->inbox.front());
            m_shared->inbox.pop_front();
        }
        if (msg.isResult) handleTaskFinished(msg.result);
        else emit jobStateChanged(msg.jobId, msg.paneId, msg.state);
    }
}

quint64 JobManager::submitJob(CompilationRequest req) {
    if (m_isShuttingDown.load(std::memory_order_acquire)) {
        return 0;
    }

    const quint64 jobId = m_nextJobId.fetch_add(1);
    req.jobId = jobId;

    QList<DroppedJob> dropped;
    QList<quint64> toCancel;
    QList<Dispatch> dispatches;
    {
        QMutexLocker locker(&m_mutex);
        if (m_isShuttingDown.load(std::memory_order_acquire)) return 0;

        m_paneCurrentRevision[req.paneId] = req.revision;
        m_paneCurrentJobId[req.paneId] = jobId;

        // Supersede everything older that belongs to this pane.
        int replaceIndex = -1;
        const quint64 oldPending = m_panePendingJob.value(req.paneId, 0);
        if (oldPending != 0) {
            replaceIndex = m_pending.indexOf(oldPending);
            auto it = m_records.find(oldPending);
            if (it != m_records.end()) {
                dropped.append({it->req, false});
                m_records.erase(it);
            }
            m_pending.removeOne(oldPending);
            m_panePendingJob.remove(req.paneId);
        }
        for (auto it = m_records.begin(); it != m_records.end(); ++it) {
            if (it->dispatched && it->req.paneId == req.paneId &&
                !it->cancelFlag->load(std::memory_order_acquire)) {
                it->cancelFlag->store(true, std::memory_order_release);
                toCancel.append(it.key());
            }
        }

        JobRecord rec;
        rec.req = req;
        rec.cancelFlag = std::make_shared<std::atomic_bool>(false);
        m_records.insert(jobId, rec);
        // A replacing job keeps the queue position of the one it replaces
        // (a busy pane neither starves nor jumps ahead of other panes).
        if (replaceIndex >= 0 && replaceIndex <= m_pending.size()) m_pending.insert(replaceIndex, jobId);
        else m_pending.append(jobId);
        m_panePendingJob[req.paneId] = jobId;

        // Bounded queue: the oldest waiting job is dropped.
        while (m_pending.size() > m_maxPending) {
            const quint64 victim = m_pending.takeFirst();
            auto it = m_records.find(victim);
            if (it != m_records.end()) {
                if (m_panePendingJob.value(it->req.paneId) == victim) m_panePendingJob.remove(it->req.paneId);
                dropped.append({it->req, victim == m_paneCurrentJobId.value(it->req.paneId)});
                m_records.erase(it);
            }
        }

        pumpLocked(dispatches);
    }

    cancelDispatched(toCancel);
    emit jobStateChanged(jobId, req.paneId, JobState::Queued);
    notifyCancelledPending(dropped);
    finishDispatch(dispatches);
    return jobId;
}

void JobManager::cancelJob(quint64 jobId) {
    QList<DroppedJob> dropped;
    QList<quint64> toCancel;
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_records.find(jobId);
        if (it == m_records.end()) return; // unknown or already finished: nothing to cancel
        if (it->dispatched) {
            it->cancelFlag->store(true, std::memory_order_release);
            toCancel.append(jobId);
        } else {
            const bool isCurrent = m_paneCurrentJobId.value(it->req.paneId) == jobId;
            dropped.append({it->req, isCurrent});
            removePendingLocked(jobId);
        }
    }
    cancelDispatched(toCancel);
    notifyCancelledPending(dropped);
}

void JobManager::cancelPaneJobs(quint64 paneId) {
    QList<DroppedJob> dropped;
    QList<quint64> toCancel;
    {
        QMutexLocker locker(&m_mutex);
        QList<quint64> pendingIds;
        for (auto it = m_records.begin(); it != m_records.end(); ++it) {
            if (it->req.paneId != paneId) continue;
            if (it->dispatched) {
                it->cancelFlag->store(true, std::memory_order_release);
                toCancel.append(it.key());
            } else {
                pendingIds.append(it.key());
            }
        }
        for (quint64 id : pendingIds) {
            const auto& req = m_records.value(id).req;
            dropped.append({req, id == m_paneCurrentJobId.value(paneId)});
            removePendingLocked(id);
        }
    }
    cancelDispatched(toCancel);
    notifyCancelledPending(dropped);
}

void JobManager::invalidatePane(quint64 paneId) {
    {
        // Forget the pane first so nothing cancelled below is delivered to a closed panel.
        QMutexLocker locker(&m_mutex);
        m_paneCurrentRevision.remove(paneId);
        m_paneCurrentJobId.remove(paneId);
    }
    cancelPaneJobs(paneId);
}

void JobManager::setMaxParallelJobs(int maxJobs) {
    if (maxJobs < 1) maxJobs = 1;
    QList<Dispatch> dispatches;
    {
        QMutexLocker locker(&m_mutex);
        m_maxRunning = maxJobs;
        m_threadPool->setMaxThreadCount(maxJobs);
        pumpLocked(dispatches);
    }
    finishDispatch(dispatches);
}

int JobManager::maxParallelJobs() const {
    QMutexLocker locker(&m_mutex);
    return m_maxRunning;
}

void JobManager::setMaxPendingJobs(int maxJobs) {
    QMutexLocker locker(&m_mutex);
    m_maxPending = qMax(1, maxJobs);
}

int JobManager::maxPendingJobs() const {
    QMutexLocker locker(&m_mutex);
    return m_maxPending;
}

int JobManager::activeJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_records.size();
}

int JobManager::pendingJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_pending.size();
}

int JobManager::runningJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_running;
}

quint64 JobManager::getPaneCurrentRevision(quint64 paneId) const {
    QMutexLocker locker(&m_mutex);
    return m_paneCurrentRevision.value(paneId, 0);
}

quint64 JobManager::getPaneCurrentJobId(quint64 paneId) const {
    QMutexLocker locker(&m_mutex);
    return m_paneCurrentJobId.value(paneId, 0);
}

void JobManager::handleTaskFinished(const CompilationResult& result) {
    bool isObsolete = false;
    QList<Dispatch> dispatches;
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_records.find(result.jobId);
        if (it != m_records.end() && it->dispatched) {
            m_records.erase(it);
            if (m_running > 0) --m_running;
        }

        const quint64 latest = m_paneCurrentJobId.value(result.paneId, 0);
        if (latest == 0 || result.jobId != latest) {
            isObsolete = true;
        } else if (result.revision < m_paneCurrentRevision.value(result.paneId, 0)) {
            isObsolete = true;
        }
        if (m_isShuttingDown.load(std::memory_order_acquire)) isObsolete = true;

        pumpLocked(dispatches);
    }

    emit jobStateChanged(result.jobId, result.paneId, result.state);
    if (!isObsolete) {
        emit jobFinished(result);
    }
    finishDispatch(dispatches);
}

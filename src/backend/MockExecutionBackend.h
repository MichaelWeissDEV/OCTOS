#ifndef MOCKEXECUTIONBACKEND_H
#define MOCKEXECUTIONBACKEND_H

#include "IExecutionBackend.h"
#include <QMutex>
#include <QSet>
#include <atomic>

class MockExecutionBackend : public IExecutionBackend {
public:
    MockExecutionBackend();
    ~MockExecutionBackend() override = default;

    CompilationResult execute(const CompilationRequest& req,
                              const std::function<bool()>& isCancelled = nullptr) override;

    void cancel(quint64 jobId) override;

    void setSimulatedDelayMs(int delayMs) { m_delayMs = delayMs; }
    void setForceError(bool error) { m_forceError = error; }
    void setForceTimeout(bool timeout) { m_forceTimeout = timeout; }

    int executedJobCount() const;
    bool wasJobCancelled(quint64 jobId) const;

private:
    int m_delayMs{0};
    bool m_forceError{false};
    bool m_forceTimeout{false};
    mutable QMutex m_mutex;
    QSet<quint64> m_cancelledJobs;
    int m_executedCount{0};
};

#endif // MOCKEXECUTIONBACKEND_H

#ifndef DOCKEREXECUTIONBACKEND_H
#define DOCKEREXECUTIONBACKEND_H

#include "IExecutionBackend.h"
#include "DockerExecutionConfig.h"
#include <QHash>
#include <QMutex>
#include <QSet>
#include <QStringList>

/**
 * Runs compilations in short-lived Docker containers.
 *
 * Concurrency model: every execute() call runs entirely on the caller's worker
 * thread and owns its QProcess. The shared registry (m_jobs) is only touched under
 * m_mutex for short bookkeeping; no docker process is ever started or awaited
 * while the mutex is held.
 *
 * A job's container name stays registered until its cleanup has finished. If the
 * cleanup fails the name moves to the "leaked" set (see leakedContainers()) and is
 * retried on retryLeakedCleanup() / destruction, so it is never silently forgotten.
 */
class DockerExecutionBackend : public IExecutionBackend {
public:
    explicit DockerExecutionBackend(const DockerExecutionConfig& config = DockerExecutionConfig());
    ~DockerExecutionBackend() override;

    CompilationResult execute(const CompilationRequest& req,
                              const std::function<bool()>& isCancelled = nullptr) override;

    void cancel(quint64 jobId) override;

    DockerExecutionConfig config() const;
    void setConfig(const DockerExecutionConfig& config);

    static QStringList buildDockerRunArgs(const CompilationRequest& req,
                                         const DockerExecutionConfig& config,
                                         const QString& containerName,
                                         const QString& inputHostPath,
                                         const QString& outputHostPath,
                                         const QStringList& compilerArgs);

    /// Removes an OCTOS-owned container (name must start with "octos_"). Idempotent:
    /// returns true if the container is gone afterwards (including "did not exist"),
    /// false if removal could not be confirmed.
    static bool cleanupContainer(const QString& containerName,
                                 const QString& dockerProgram = QStringLiteral("docker"),
                                 int timeoutMs = 8000);

    /// Appends `chunk` to `buffer` without exceeding `limit` bytes. Returns the number
    /// of bytes that were dropped. Sets `truncated` once anything was dropped.
    static qint64 appendBounded(QByteArray& buffer, const QByteArray& chunk,
                                qint64 limit, bool& truncated);
    /// Removes an incomplete trailing UTF-8 sequence (a cut in the middle of a code point).
    static void trimToUtf8Boundary(QByteArray& data);
    /// Classifies a non-zero docker exit (125/126/127 or daemon/image messages) as Docker-side.
    static bool isDockerSideFailure(int exitCode, const QString& stderrText);

    // Introspection (diagnostics and tests)
    QStringList activeContainerNames() const;
    QStringList leakedContainers() const;
    /// Re-attempts removal of containers whose cleanup failed. Returns how many are still leaked.
    int retryLeakedCleanup();

private:
    struct JobCtx {
        QString container;
        bool cancelled{false};
    };

    bool isJobCancelled(quint64 jobId) const;
    bool finishCleanup(const QString& containerName);

    DockerExecutionConfig m_config;
    mutable QMutex m_mutex;
    QHash<quint64, JobCtx> m_jobs;
    QSet<QString> m_leaked;
};

#endif // DOCKEREXECUTIONBACKEND_H

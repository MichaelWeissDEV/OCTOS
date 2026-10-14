#ifndef DOCKEREXECUTIONBACKEND_H
#define DOCKEREXECUTIONBACKEND_H

#include "IExecutionBackend.h"
#include "DockerExecutionConfig.h"
#include <QMutex>
#include <QMap>
#include <QSet>
#include <QStringList>

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

    static void cleanupContainer(const QString& containerName);

private:
    bool isJobCancelled(quint64 jobId) const;

    DockerExecutionConfig m_config;
    mutable QMutex m_mutex;
    QSet<quint64> m_cancelledJobs;
    QMap<quint64, QString> m_activeContainers;
};

#endif // DOCKEREXECUTIONBACKEND_H

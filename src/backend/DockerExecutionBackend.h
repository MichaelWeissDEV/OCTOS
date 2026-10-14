#ifndef DOCKEREXECUTIONBACKEND_H
#define DOCKEREXECUTIONBACKEND_H

#include "IExecutionBackend.h"
#include "DockerExecutionConfig.h"
#include <QMutex>
#include <QMap>
#include <QProcess>
#include <QStringList>

class DockerExecutionBackend : public IExecutionBackend {
public:
    explicit DockerExecutionBackend(const DockerExecutionConfig& config = DockerExecutionConfig());
    ~DockerExecutionBackend() override;

    CompilationResult execute(const CompilationRequest& req,
                              const std::function<bool()>& isCancelled = nullptr) override;

    void cancel(quint64 jobId) override;

    const DockerExecutionConfig& config() const { return m_config; }
    void setConfig(const DockerExecutionConfig& config) { m_config = config; }

    static QStringList buildDockerRunArgs(const CompilationRequest& req,
                                         const DockerExecutionConfig& config,
                                         const QString& containerName,
                                         const QString& inputHostPath,
                                         const QString& outputHostPath,
                                         const QStringList& compilerArgs);

    static void cleanupContainer(const QString& containerName);

private:
    DockerExecutionConfig m_config;
    QMutex m_processMutex;
    QMap<quint64, QProcess*> m_runningProcesses;
    QMap<quint64, QString> m_runningContainers;
};

#endif // DOCKEREXECUTIONBACKEND_H

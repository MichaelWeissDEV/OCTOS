#include "DockerExecutionBackend.h"
#include "../strategies/LanguageStrategyRegistry.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QMutexLocker>
#include <QTemporaryDir>
#include <QTextStream>

DockerExecutionBackend::DockerExecutionBackend(const DockerExecutionConfig& config)
    : m_config(config) {
}

DockerExecutionBackend::~DockerExecutionBackend() {
    QMutexLocker locker(&m_processMutex);
    for (auto it = m_runningProcesses.begin(); it != m_runningProcesses.end(); ++it) {
        if (it.value()) {
            it.value()->kill();
        }
    }
    for (const QString& container : m_runningContainers.values()) {
        cleanupContainer(container);
    }
    m_runningProcesses.clear();
    m_runningContainers.clear();
}

void DockerExecutionBackend::cleanupContainer(const QString& containerName) {
    if (containerName.isEmpty()) return;
    QProcess::execute("docker", QStringList() << "kill" << containerName);
    QProcess::execute("docker", QStringList() << "rm" << "-f" << containerName);
}

QStringList DockerExecutionBackend::buildDockerRunArgs(const CompilationRequest& req,
                                                      const DockerExecutionConfig& config,
                                                      const QString& containerName,
                                                      const QString& inputHostPath,
                                                      const QString& outputHostPath,
                                                      const QStringList& compilerArgs) {
    QStringList dockerArgs;
    dockerArgs << "run" << "--rm";
    dockerArgs << "--name" << containerName;

    // Security & Isolation
    if (config.networkNone) {
        dockerArgs << "--network" << "none";
    }

    if (!config.memoryLimit.isEmpty()) {
        dockerArgs << "--memory" << config.memoryLimit;
    }

    if (!config.cpuLimit.isEmpty()) {
        dockerArgs << "--cpus" << config.cpuLimit;
    }

    if (config.pidsLimit > 0) {
        dockerArgs << "--pids-limit" << QString::number(config.pidsLimit);
    }

    if (config.noNewPrivileges) {
        dockerArgs << "--security-opt" << "no-new-privileges";
    }

    if (config.dropCapabilities) {
        dockerArgs << "--cap-drop=ALL";
    }

    if (config.readOnlyRootFs) {
        dockerArgs << "--read-only";
        dockerArgs << "--tmpfs" << "/tmp:rw,noexec,nosuid,size=64m";
    }

    // Mount input read-only
    dockerArgs << "-v" << (inputHostPath + ":/input:ro");

    // Mount output read-write
    dockerArgs << "-v" << (outputHostPath + ":/output:rw");

    // Working directory
    dockerArgs << "-w" << "/output";

    // Docker image
    dockerArgs << req.compilerImage;

    // Compiler and arguments
    dockerArgs.append(compilerArgs);

    return dockerArgs;
}

CompilationResult DockerExecutionBackend::execute(const CompilationRequest& req,
                                                 const std::function<bool()>& isCancelled) {
    QElapsedTimer timer;
    timer.start();

    CompilationResult result;
    result.jobId = req.jobId;
    result.paneId = req.paneId;
    result.revision = req.revision;
    result.state = JobState::Starting;

    if (isCancelled && isCancelled()) {
        result.state = JobState::Cancelled;
        result.errorOutput = "Compilation cancelled before starting";
        result.durationMs = timer.elapsed();
        return result;
    }

    const ILanguageStrategy* strategy =
        LanguageStrategyRegistry::instance().getStrategy(req.language, req.compilerImage);

    if (!strategy) {
        result.state = JobState::Failed;
        result.exitCode = -1;
        result.errorOutput = "No compiler strategy available for this language";
        result.durationMs = timer.elapsed();
        return result;
    }

    // Set up host temporary workspaces
    QTemporaryDir inputDir;
    QTemporaryDir outputDir;

    if (!inputDir.isValid() || !outputDir.isValid()) {
        result.state = JobState::Failed;
        result.exitCode = -1;
        result.errorOutput = "Failed to create temporary directories for compilation";
        result.durationMs = timer.elapsed();
        return result;
    }

    // Ensure output directory is world-writable so non-root container users can write
    QFile::setPermissions(outputDir.path(),
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                          QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
                          QFile::ReadOther | QFile::WriteOther | QFile::ExeOther);

    QString sourceFile = strategy->sourceFileName(req.sourceCode);
    QString hostSourcePath = inputDir.path() + "/" + sourceFile;
    QFile srcOut(hostSourcePath);
    if (!srcOut.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.state = JobState::Failed;
        result.exitCode = -1;
        result.errorOutput = "Failed to write source file to workspace";
        result.durationMs = timer.elapsed();
        return result;
    }
    {
        QTextStream stream(&srcOut);
        stream << req.sourceCode;
    }
    srcOut.close();

    QString containerInputPath = "/input/" + sourceFile;
    QString containerOutputDir = "/output";
    QStringList compilerArgs = strategy->buildCompilerArgs(req, containerInputPath, containerOutputDir);

    qint64 pid = QCoreApplication::applicationPid();
    QString containerName = QString("octos_%1_%2").arg(pid).arg(req.jobId);

    QStringList dockerArgs = buildDockerRunArgs(req, m_config, containerName,
                                                inputDir.path(), outputDir.path(), compilerArgs);

    if (isCancelled && isCancelled()) {
        result.state = JobState::Cancelled;
        result.errorOutput = "Compilation cancelled";
        result.durationMs = timer.elapsed();
        return result;
    }

    QProcess process;
    {
        QMutexLocker locker(&m_processMutex);
        m_runningProcesses[req.jobId] = &process;
        m_runningContainers[req.jobId] = containerName;
    }

    result.state = JobState::Running;
    process.start("docker", dockerArgs);

    if (!process.waitForStarted(5000)) {
        {
            QMutexLocker locker(&m_processMutex);
            m_runningProcesses.remove(req.jobId);
            m_runningContainers.remove(req.jobId);
        }
        cleanupContainer(containerName);

        result.state = JobState::Failed;
        result.exitCode = -1;
        result.errorOutput = "Docker process could not be started. Check if Docker daemon is running.";
        result.durationMs = timer.elapsed();
        return result;
    }

    int timeoutMs = req.timeoutMs > 0 ? req.timeoutMs : m_config.defaultTimeoutMs;
    bool finished = process.waitForFinished(timeoutMs);

    {
        QMutexLocker locker(&m_processMutex);
        m_runningProcesses.remove(req.jobId);
        m_runningContainers.remove(req.jobId);
    }

    if (!finished) {
        // Timeout
        process.kill();
        cleanupContainer(containerName);

        result.state = JobState::TimedOut;
        result.exitCode = -2;
        result.errorOutput = QString("Compilation timed out after %1 seconds").arg(timeoutMs / 1000);
        result.durationMs = timer.elapsed();
        return result;
    }

    if (isCancelled && isCancelled()) {
        cleanupContainer(containerName);
        result.state = JobState::Cancelled;
        result.exitCode = -1;
        result.errorOutput = "Compilation cancelled";
        result.durationMs = timer.elapsed();
        return result;
    }

    cleanupContainer(containerName);

    QByteArray rawStdOut = process.readAllStandardOutput();
    QByteArray rawStdErr = process.readAllStandardError();

    // Check size limits
    if (rawStdOut.size() > m_config.maxOutputBytes) {
        rawStdOut.truncate(m_config.maxOutputBytes);
        rawStdOut.append("\n[OCTOS: stdout truncated because it exceeded output limit]");
    }
    if (rawStdErr.size() > m_config.maxOutputBytes) {
        rawStdErr.truncate(m_config.maxOutputBytes);
        rawStdErr.append("\n[OCTOS: stderr truncated because it exceeded output limit]");
    }

    QString stdoutStr = QString::fromUtf8(rawStdOut);
    QString stderrStr = QString::fromUtf8(rawStdErr);
    int exitCode = process.exitCode();

    QString outputFileContent;
    QString hostOutputFile = outputDir.path() + "/" + strategy->outputFileName();
    QFile outF(hostOutputFile);
    if (outF.exists() && outF.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray fileBytes = outF.read(m_config.maxOutputBytes);
        outputFileContent = QString::fromUtf8(fileBytes);
        outF.close();
    }

    result = strategy->processOutput(req, exitCode, stdoutStr, stderrStr, outputFileContent);
    result.durationMs = timer.elapsed();

    return result;
}

void DockerExecutionBackend::cancel(quint64 jobId) {
    QMutexLocker locker(&m_processMutex);
    if (m_runningProcesses.contains(jobId)) {
        QProcess* proc = m_runningProcesses[jobId];
        if (proc) {
            proc->kill();
        }
    }
    if (m_runningContainers.contains(jobId)) {
        QString container = m_runningContainers[jobId];
        cleanupContainer(container);
    }
}

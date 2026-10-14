#include "DockerExecutionBackend.h"
#include "../strategies/LanguageStrategyRegistry.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>

#if !defined(Q_OS_WIN)
#include <unistd.h>
#include <sys/types.h>
#endif

DockerExecutionBackend::DockerExecutionBackend(const DockerExecutionConfig& config)
    : m_config(config) {
}

DockerExecutionBackend::~DockerExecutionBackend() {
    QList<QString> containersToClean;
    {
        QMutexLocker locker(&m_mutex);
        containersToClean = m_activeContainers.values();
        m_activeContainers.clear();
        m_cancelledJobs.clear();
    }
    // Cleanup containers outside of the mutex lock to avoid deadlocks
    for (const QString& container : containersToClean) {
        cleanupContainer(container);
    }
}

DockerExecutionConfig DockerExecutionBackend::config() const {
    QMutexLocker locker(&m_mutex);
    return m_config;
}

void DockerExecutionBackend::setConfig(const DockerExecutionConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_config = config;
}

bool DockerExecutionBackend::isJobCancelled(quint64 jobId) const {
    QMutexLocker locker(&m_mutex);
    return m_cancelledJobs.contains(jobId);
}

void DockerExecutionBackend::cleanupContainer(const QString& containerName) {
    if (containerName.isEmpty()) return;
    // Execute docker kill and rm with short bounded timeouts
    QProcess killProc;
    killProc.start("docker", QStringList() << "kill" << containerName);
    killProc.waitForFinished(3000);

    QProcess rmProc;
    rmProc.start("docker", QStringList() << "rm" << "-f" << containerName);
    rmProc.waitForFinished(3000);
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

    // Optional user isolation
    if (!config.runAsUser.isEmpty()) {
        dockerArgs << "--user" << config.runAsUser;
    }
#if defined(Q_OS_LINUX)
    else if (config.runAsCurrentUser) {
        dockerArgs << "--user" << QString("%1:%2").arg(::getuid()).arg(::getgid());
    }
#endif

    // Mount input strictly read-only
    dockerArgs << "-v" << (inputHostPath + ":/input:ro");

    // Mount output read-write
    dockerArgs << "-v" << (outputHostPath + ":/output:rw");

    // Working directory inside container
    dockerArgs << "-w" << "/output";

    // Docker image
    dockerArgs << req.compilerImage;

    // Compiler executable and arguments
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
    result.compilerImage = req.compilerImage;
    result.state = JobState::Starting;

    DockerExecutionConfig activeConfig = config();

    if ((isCancelled && isCancelled()) || isJobCancelled(req.jobId)) {
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

    // Set strict secure permissions: owner read/write/exec, group read/exec, NEVER world-writable!
    QFile::setPermissions(outputDir.path(),
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                          QFile::ReadGroup | QFile::ExeGroup);

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

    QStringList dockerArgs = buildDockerRunArgs(req, activeConfig, containerName,
                                                inputDir.path(), outputDir.path(), compilerArgs);

    if ((isCancelled && isCancelled()) || isJobCancelled(req.jobId)) {
        result.state = JobState::Cancelled;
        result.errorOutput = "Compilation cancelled";
        result.durationMs = timer.elapsed();
        return result;
    }

    // Register active container before launching process
    {
        QMutexLocker locker(&m_mutex);
        m_activeContainers[req.jobId] = containerName;
    }

    // QProcess is allocated and operated strictly within this worker thread!
    QProcess process;
    result.state = JobState::Running;
    process.start("docker", dockerArgs);

    if (!process.waitForStarted(5000)) {
        {
            QMutexLocker locker(&m_mutex);
            m_activeContainers.remove(req.jobId);
        }
        cleanupContainer(containerName);

        result.state = JobState::Failed;
        result.exitCode = -1;
        result.errorOutput = "Docker process could not be started. Check if Docker daemon is running.";
        result.durationMs = timer.elapsed();
        return result;
    }

    int timeoutMs = req.timeoutMs > 0 ? req.timeoutMs : activeConfig.defaultTimeoutMs;

    QByteArray rawStdOut;
    QByteArray rawStdErr;
    bool stdoutTruncated = false;
    bool stderrTruncated = false;
    bool wasCancelled = false;
    bool wasTimedOut = false;

    // Incremental polling loop to avoid blocking and buffer explosions
    while (process.state() == QProcess::Running) {
        if ((isCancelled && isCancelled()) || isJobCancelled(req.jobId)) {
            wasCancelled = true;
            break;
        }

        if (timer.elapsed() > timeoutMs) {
            wasTimedOut = true;
            break;
        }

        // Bounded wait of 100ms
        process.waitForReadyRead(100);

        // Read incrementally into bounded buffers
        QByteArray chunkOut = process.readAllStandardOutput();
        if (!chunkOut.isEmpty()) {
            if (rawStdOut.size() + chunkOut.size() <= activeConfig.maxOutputBytes) {
                rawStdOut.append(chunkOut);
            } else if (!stdoutTruncated) {
                int remaining = static_cast<int>(activeConfig.maxOutputBytes - rawStdOut.size());
                if (remaining > 0) {
                    rawStdOut.append(chunkOut.left(remaining));
                }
                stdoutTruncated = true;
            }
        }

        QByteArray chunkErr = process.readAllStandardError();
        if (!chunkErr.isEmpty()) {
            if (rawStdErr.size() + chunkErr.size() <= activeConfig.maxOutputBytes) {
                rawStdErr.append(chunkErr);
            } else if (!stderrTruncated) {
                int remaining = static_cast<int>(activeConfig.maxOutputBytes - rawStdErr.size());
                if (remaining > 0) {
                    rawStdErr.append(chunkErr.left(remaining));
                }
                stderrTruncated = true;
            }
        }

        // Prevent runaway processes from eating infinite memory if output severely exceeds limits
        if (stdoutTruncated && stderrTruncated &&
            (rawStdOut.size() + rawStdErr.size() >= 2 * activeConfig.maxOutputBytes)) {
            process.kill();
            break;
        }
    }

    // Clean up container mapping
    {
        QMutexLocker locker(&m_mutex);
        m_activeContainers.remove(req.jobId);
        m_cancelledJobs.remove(req.jobId);
    }

    if (wasCancelled) {
        process.kill();
        process.waitForFinished(1000);
        cleanupContainer(containerName);

        result.state = JobState::Cancelled;
        result.exitCode = -1;
        result.errorOutput = "Compilation cancelled";
        result.durationMs = timer.elapsed();
        return result;
    }

    if (wasTimedOut) {
        process.kill();
        process.waitForFinished(1000);
        cleanupContainer(containerName);

        result.state = JobState::TimedOut;
        result.exitCode = -2;
        result.errorOutput = QString("Compilation timed out after %1 seconds").arg(timeoutMs / 1000);
        result.durationMs = timer.elapsed();
        return result;
    }

    // Wait for process to fully terminate if still in Finishing state
    if (process.state() != QProcess::NotRunning) {
        process.waitForFinished(2000);
    }

    // Read any remaining output bytes
    QByteArray finalOut = process.readAllStandardOutput();
    if (!finalOut.isEmpty() && !stdoutTruncated) {
        if (rawStdOut.size() + finalOut.size() <= activeConfig.maxOutputBytes) {
            rawStdOut.append(finalOut);
        } else {
            int remaining = static_cast<int>(activeConfig.maxOutputBytes - rawStdOut.size());
            if (remaining > 0) rawStdOut.append(finalOut.left(remaining));
            stdoutTruncated = true;
        }
    }

    QByteArray finalErr = process.readAllStandardError();
    if (!finalErr.isEmpty() && !stderrTruncated) {
        if (rawStdErr.size() + finalErr.size() <= activeConfig.maxOutputBytes) {
            rawStdErr.append(finalErr);
        } else {
            int remaining = static_cast<int>(activeConfig.maxOutputBytes - rawStdErr.size());
            if (remaining > 0) rawStdErr.append(finalErr.left(remaining));
            stderrTruncated = true;
        }
    }

    cleanupContainer(containerName);

    if (stdoutTruncated) {
        rawStdOut.append("\n[OCTOS: stdout truncated because it exceeded memory limit]");
    }
    if (stderrTruncated) {
        rawStdErr.append("\n[OCTOS: stderr truncated because it exceeded memory limit]");
    }

    result.stdoutTruncated = stdoutTruncated;
    result.stderrTruncated = stderrTruncated;

    QString stdoutStr = QString::fromUtf8(rawStdOut);
    QString stderrStr = QString::fromUtf8(rawStdErr);
    int exitCode = process.exitCode();

    QString outputFileContent;
    QString hostOutputFile = outputDir.path() + "/" + strategy->outputFileName();
    QFileInfo outInfo(hostOutputFile);

    if (outInfo.exists()) {
        QFile outF(hostOutputFile);
        if (outF.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (outInfo.size() > activeConfig.maxFileSizeBytes) {
                QByteArray fileBytes = outF.read(activeConfig.maxFileSizeBytes);
                outputFileContent = QString::fromUtf8(fileBytes);
                outputFileContent.append("\n[OCTOS: Output artifact truncated because file size exceeded maximum limit]");
                result.artifactTruncated = true;
            } else {
                QByteArray fileBytes = outF.readAll();
                outputFileContent = QString::fromUtf8(fileBytes);
            }
            outF.close();
        }
    }

    result = strategy->processOutput(req, exitCode, stdoutStr, stderrStr, outputFileContent);
    result.compilerImage = req.compilerImage;
    result.stdoutTruncated = stdoutTruncated;
    result.stderrTruncated = stderrTruncated;
    result.durationMs = timer.elapsed();

    // Populate structured artifacts
    if (!result.assemblyOutput.isEmpty()) {
        CompilationArtifact asmArt;
        asmArt.type = (req.language == Language::Java) ? ArtifactType::JVM_Bytecode :
                      (req.language == Language::Python) ? ArtifactType::CPython_Bytecode :
                      (req.language == Language::CSharp) ? ArtifactType::CIL : ArtifactType::Assembly;
        asmArt.name = strategy->outputFileName();
        asmArt.content = result.assemblyOutput;
        asmArt.sizeBytes = result.assemblyOutput.toUtf8().size();
        asmArt.originPath = hostOutputFile;
        result.addArtifact(asmArt);
    }

    if (!result.rawOutput.isEmpty()) {
        CompilationArtifact rawArt;
        rawArt.type = ArtifactType::RawOutput;
        rawArt.name = "stdout";
        rawArt.content = result.rawOutput;
        rawArt.sizeBytes = result.rawOutput.toUtf8().size();
        result.addArtifact(rawArt);
    }

    return result;
}

void DockerExecutionBackend::cancel(quint64 jobId) {
    QString containerToClean;
    {
        QMutexLocker locker(&m_mutex);
        m_cancelledJobs.insert(jobId);
        containerToClean = m_activeContainers.value(jobId);
    }

    // Clean up container outside the mutex lock
    if (!containerToClean.isEmpty()) {
        cleanupContainer(containerToClean);
    }
}

#include "DockerExecutionBackend.h"
#include "../strategies/LanguageStrategyRegistry.h"
#include "../core/WorkspaceModel.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QProcess>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#if !defined(Q_OS_WIN)
#include <unistd.h>
#include <sys/types.h>
#endif

namespace {
const QString kContainerPrefix = QStringLiteral("octos_");
}

DockerExecutionBackend::DockerExecutionBackend(const DockerExecutionConfig& config)
    : m_config(config.sanitized()) {
}

DockerExecutionBackend::~DockerExecutionBackend() {
    QStringList toClean;
    DockerExecutionConfig cfg;
    {
        QMutexLocker locker(&m_mutex);
        cfg = m_config;
        for (const JobCtx& ctx : std::as_const(m_jobs)) toClean << ctx.container;
        toClean << m_leaked.values();
        m_jobs.clear();
        m_leaked.clear();
    }
    // No docker process is run under the mutex.
    for (const QString& container : std::as_const(toClean)) {
        cleanupContainer(container, cfg.dockerProgram, cfg.cleanupTimeoutMs);
    }
}

DockerExecutionConfig DockerExecutionBackend::config() const {
    QMutexLocker locker(&m_mutex);
    return m_config;
}

void DockerExecutionBackend::setConfig(const DockerExecutionConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_config = config.sanitized();
}

bool DockerExecutionBackend::isJobCancelled(quint64 jobId) const {
    QMutexLocker locker(&m_mutex);
    auto it = m_jobs.constFind(jobId);
    return it != m_jobs.constEnd() && it->cancelled;
}

QStringList DockerExecutionBackend::activeContainerNames() const {
    QMutexLocker locker(&m_mutex);
    QStringList names;
    for (const JobCtx& ctx : m_jobs) names << ctx.container;
    names.sort();
    return names;
}

QStringList DockerExecutionBackend::leakedContainers() const {
    QMutexLocker locker(&m_mutex);
    QStringList names = m_leaked.values();
    names.sort();
    return names;
}

int DockerExecutionBackend::retryLeakedCleanup() {
    QStringList names;
    DockerExecutionConfig cfg;
    {
        QMutexLocker locker(&m_mutex);
        names = m_leaked.values();
        cfg = m_config;
    }
    for (const QString& n : std::as_const(names)) {
        if (cleanupContainer(n, cfg.dockerProgram, cfg.cleanupTimeoutMs)) {
            QMutexLocker locker(&m_mutex);
            m_leaked.remove(n);
        }
    }
    QMutexLocker locker(&m_mutex);
    return m_leaked.size();
}

bool DockerExecutionBackend::cleanupContainer(const QString& containerName,
                                              const QString& dockerProgram,
                                              int timeoutMs) {
    // Only ever touch containers this application created.
    if (containerName.isEmpty() || !containerName.startsWith(kContainerPrefix)) return false;

    // `rm -f` kills and removes in one step and is safe to repeat.
    QProcess rm;
    rm.start(dockerProgram, QStringList() << "rm" << "-f" << containerName);
    if (!rm.waitForStarted(timeoutMs)) return false;
    if (!rm.waitForFinished(timeoutMs)) {
        rm.kill();
        rm.waitForFinished(1000);
        return false;
    }
    if (rm.exitStatus() == QProcess::NormalExit && rm.exitCode() == 0) return true;

    const QString err = QString::fromUtf8(rm.readAllStandardError());
    // Already gone is the desired end state, not a failure.
    return err.contains("No such container", Qt::CaseInsensitive);
}

bool DockerExecutionBackend::finishCleanup(const QString& containerName) {
    const DockerExecutionConfig cfg = config();
    const bool ok = cleanupContainer(containerName, cfg.dockerProgram, cfg.cleanupTimeoutMs);
    if (!ok) {
        QMutexLocker locker(&m_mutex);
        m_leaked.insert(containerName);
    }
    return ok;
}

qint64 DockerExecutionBackend::appendBounded(QByteArray& buffer, const QByteArray& chunk,
                                             qint64 limit, bool& truncated) {
    if (chunk.isEmpty()) return 0;
    if (truncated) return chunk.size();  // keep a consistent prefix: nothing after the cut
    const qint64 room = qMax<qint64>(0, limit - buffer.size());
    if (chunk.size() <= room) {
        buffer.append(chunk);
        return 0;
    }
    buffer.append(chunk.constData(), static_cast<qsizetype>(room));
    truncated = true;
    return chunk.size() - room;
}

void DockerExecutionBackend::trimToUtf8Boundary(QByteArray& data) {
    const int n = data.size();
    if (n == 0) return;
    int i = n - 1;
    int back = 0;
    while (i >= 0 && back < 4 && (static_cast<uchar>(data[i]) & 0xC0) == 0x80) { --i; ++back; }
    if (i < 0) { data.clear(); return; }
    const uchar lead = static_cast<uchar>(data[i]);
    int need = 1;
    if (lead >= 0xF0) need = 4;
    else if (lead >= 0xE0) need = 3;
    else if (lead >= 0xC0) need = 2;
    if (n - i < need) data.truncate(i);  // incomplete trailing sequence
}

bool DockerExecutionBackend::isDockerSideFailure(int exitCode, const QString& stderrText) {
    if (exitCode == 125 || exitCode == 126 || exitCode == 127) return true;
    static const QRegularExpression re(
        R"(Cannot connect to the Docker daemon|error during connect|Unable to find image|docker: Error response from daemon|permission denied while trying to connect to the Docker)",
        QRegularExpression::CaseInsensitiveOption);
    return re.match(stderrText).hasMatch();
}

QStringList DockerExecutionBackend::buildDockerRunArgs(const CompilationRequest& req,
                                                      const DockerExecutionConfig& config,
                                                      const QString& containerName,
                                                      const QString& inputHostPath,
                                                      const QString& outputHostPath,
                                                      const QStringList& compilerArgs) {
    QStringList dockerArgs;
    // Never download anything implicitly: a missing image must fail fast, and pulling stays an
    // explicit user action in the compiler manager.
    dockerArgs << "run" << "--rm" << "--pull=never";
    dockerArgs << "--name" << containerName;

    // Security & Isolation
    if (config.networkNone) {
        dockerArgs << "--network" << "none";
    }

    if (!config.memoryLimit.isEmpty()) {
        dockerArgs << "--memory" << config.memoryLimit;
        if (config.disableSwap) dockerArgs << "--memory-swap" << config.memoryLimit;
    }
    if (config.ipcNone) {
        dockerArgs << "--ipc" << "none";
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
    }
    // /tmp is always a bounded tmpfs so scratch files cannot grow the container layer.
    dockerArgs << "--tmpfs" << QString("/tmp:rw,noexec,nosuid,size=%1m").arg(config.tmpfsSizeMb);
    // Bound the size of any single file written (the output dir is a host bind mount). The limit
    // sits well above the read limit so artifact truncation stays reachable; a process that
    // exceeds it gets SIGXFSZ (exit code 153), which is reported as a limit violation.
    dockerArgs << "--ulimit" << QString("fsize=%1:%1").arg(config.maxTotalArtifactBytes);

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
    QStringList warnings;

    const DockerExecutionConfig activeConfig = config();
    const QString containerName =
        QString("%1%2_%3").arg(kContainerPrefix).arg(QCoreApplication::applicationPid()).arg(req.jobId);

    // Register BEFORE anything can fail or be cancelled; the registration is dropped by the
    // scope guard at the very end, i.e. after the cleanup below has run.
    {
        QMutexLocker locker(&m_mutex);
        JobCtx ctx;
        ctx.container = containerName;
        m_jobs.insert(req.jobId, ctx);
    }
    struct JobScope {
        DockerExecutionBackend* self;
        quint64 id;
        ~JobScope() {
            QMutexLocker locker(&self->m_mutex);
            self->m_jobs.remove(id);
        }
    } scope{this, req.jobId};

    auto cancelled = [&]() { return (isCancelled && isCancelled()) || isJobCancelled(req.jobId); };
    auto finish = [&](JobState state, int exitCode, FailureOrigin origin, const QString& msg) {
        result.state = state;
        result.exitCode = exitCode;
        result.failureOrigin = origin;
        result.errorOutput = msg;
        result.warnings = warnings;
        result.durationMs = timer.elapsed();
        return result;
    };

    if (cancelled()) {
        return finish(JobState::Cancelled, -1, FailureOrigin::None, "Compilation cancelled before starting");
    }

    const ILanguageStrategy* strategy =
        LanguageStrategyRegistry::instance().getStrategy(req.language, req.compilerImage);
    if (!strategy) {
        return finish(JobState::Failed, -1, FailureOrigin::Internal,
                      "No compiler strategy available for this language");
    }

    // IR is only produced when the strategy supports it for this request; otherwise assembly is the output.
    CompilationRequest effectiveReq = req;
    if (!strategy->supportsOutputKind(req.outputKind, req)) effectiveReq.outputKind = OutputKind::Assembly;

    const QByteArray sourceBytes = req.sourceCode.toUtf8();
    qint64 totalSourceBytes = sourceBytes.size();
    for (const auto& extra : req.extraFiles) totalSourceBytes += extra.second.toUtf8().size();
    if (totalSourceBytes > activeConfig.maxSourceBytes) {
        return finish(JobState::Failed, -1, FailureOrigin::Limits,
                      QString("Source is too large (%1 bytes, limit %2 bytes)")
                          .arg(totalSourceBytes).arg(activeConfig.maxSourceBytes));
    }

    QTemporaryDir inputDir;
    QTemporaryDir outputDir;
    if (!inputDir.isValid() || !outputDir.isValid()) {
        return finish(JobState::Failed, -1, FailureOrigin::Internal,
                      "Failed to create temporary directories for compilation");
    }

    // Owner rwx, group rx, never world-accessible.
    QFile::setPermissions(outputDir.path(),
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                          QFile::ReadGroup | QFile::ExeGroup);

    const QString sourceFile = strategy->sourceFileName(req.sourceCode);
    {
        QFile srcOut(inputDir.path() + "/" + sourceFile);
        if (!srcOut.open(QIODevice::WriteOnly) ||
            srcOut.write(sourceBytes) != sourceBytes.size()) {
            return finish(JobState::Failed, -1, FailureOrigin::Internal,
                          "Failed to write source file to workspace");
        }
    }

    // Additional project files (headers, modules). Paths come from a project file and are untrusted:
    // each one must resolve to a plain location below the input directory.
    for (const auto& extra : req.extraFiles) {
        QString pathErr;
        const QString target = Workspace::resolveInside(inputDir.path(), extra.first, &pathErr);
        if (target.isEmpty()) {
            return finish(JobState::Failed, -1, FailureOrigin::Internal, "Rejected project file: " + pathErr);
        }
        if (QFileInfo(target).fileName() == sourceFile && QFileInfo(target).absolutePath() == QFileInfo(inputDir.path()).canonicalFilePath()) {
            return finish(JobState::Failed, -1, FailureOrigin::Internal,
                          "Project file '" + extra.first + "' collides with the main source file name");
        }
        if (!QDir().mkpath(QFileInfo(target).absolutePath())) {
            return finish(JobState::Failed, -1, FailureOrigin::Internal, "Failed to create directory for " + extra.first);
        }
        // Re-check now that parent directories exist.
        if (Workspace::resolveInside(inputDir.path(), extra.first, &pathErr).isEmpty()) {
            return finish(JobState::Failed, -1, FailureOrigin::Internal, "Rejected project file: " + pathErr);
        }
        QFile ef(target);
        const QByteArray bytes = extra.second.toUtf8();
        if (!ef.open(QIODevice::WriteOnly) || ef.write(bytes) != bytes.size()) {
            return finish(JobState::Failed, -1, FailureOrigin::Internal, "Failed to write " + extra.first);
        }
    }

    const QString containerInputPath = "/input/" + sourceFile;
    const QStringList compilerArgs = strategy->buildCompilerArgs(effectiveReq, containerInputPath, "/output");
    const QStringList dockerArgs = buildDockerRunArgs(req, activeConfig, containerName,
                                                      inputDir.path(), outputDir.path(), compilerArgs);

    if (cancelled()) {
        return finish(JobState::Cancelled, -1, FailureOrigin::None, "Compilation cancelled");
    }

    // QProcess lives and dies on this worker thread; without an event loop its pipes only drain in the wait calls below (natural back-pressure).
    QProcess process;
    result.state = JobState::Running;
    process.start(activeConfig.dockerProgram, dockerArgs);

    if (!process.waitForStarted(activeConfig.startTimeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        finishCleanup(containerName);
        return finish(JobState::Failed, -1, FailureOrigin::Docker,
                      "Docker process could not be started. Check if Docker is installed and the daemon is running.");
    }

    const int timeoutMs = req.timeoutMs > 0 ? req.timeoutMs : activeConfig.defaultTimeoutMs;

    QByteArray rawStdOut;
    QByteArray rawStdErr;
    bool stdoutTruncated = false;
    bool stderrTruncated = false;
    qint64 discarded = 0;
    const qint64 maxDiscard = 4 * activeConfig.maxOutputBytes;
    bool wasCancelled = false;
    bool wasTimedOut = false;
    bool outputFlood = false;

    auto drain = [&]() {
        discarded += appendBounded(rawStdOut, process.readAllStandardOutput(),
                                   activeConfig.maxOutputBytes, stdoutTruncated);
        discarded += appendBounded(rawStdErr, process.readAllStandardError(),
                                   activeConfig.maxOutputBytes, stderrTruncated);
    };

    while (process.state() != QProcess::NotRunning) {
        if (cancelled()) { wasCancelled = true; break; }
        if (timer.elapsed() > timeoutMs) { wasTimedOut = true; break; }
        process.waitForReadyRead(100);
        drain();
        if (discarded > maxDiscard) { outputFlood = true; break; }
    }

    if (wasCancelled || wasTimedOut || outputFlood) {
        process.kill();
        process.waitForFinished(1000);
        // The client is dead but the container may live on: remove it, record failure if not possible.
        if (!finishCleanup(containerName)) {
            warnings << QString("Container %1 could not be removed; it will be retried.").arg(containerName);
        }
        result.stdoutTruncated = stdoutTruncated;
        result.stderrTruncated = stderrTruncated;
        if (wasCancelled) {
            return finish(JobState::Cancelled, -1, FailureOrigin::None, "Compilation cancelled");
        }
        if (wasTimedOut) {
            return finish(JobState::TimedOut, -2, FailureOrigin::Limits,
                          QString("Compilation timed out after %1 seconds").arg(timeoutMs / 1000));
        }
        return finish(JobState::Failed, -3, FailureOrigin::Limits,
                      "Compiler output exceeded the configured limit and the job was stopped");
    }

    // Normal end: collect what is left in the pipes.
    process.waitForFinished(2000);
    for (int i = 0; i < 64 && (process.bytesAvailable() > 0 || process.waitForReadyRead(0)); ++i) drain();
    drain();

    int exitCode = process.exitCode();
    if (process.exitStatus() == QProcess::CrashExit) exitCode = -1;

    const QString rawErrForClassify = QString::fromUtf8(rawStdErr);
    const bool dockerSide = exitCode != 0 && isDockerSideFailure(exitCode, rawErrForClassify);

    // The client exiting does not prove the container is gone: always remove it.
    // With an unreachable daemon no container can exist.
    const bool daemonDown = rawErrForClassify.contains("Cannot connect to the Docker daemon", Qt::CaseInsensitive);
    if (!daemonDown && !finishCleanup(containerName)) {
        warnings << QString("Container %1 could not be removed; it will be retried.").arg(containerName);
    }

    trimToUtf8Boundary(rawStdOut);
    trimToUtf8Boundary(rawStdErr);
    if (stdoutTruncated) {
        rawStdOut.append("\n[OCTOS: stdout truncated after " + QByteArray::number(activeConfig.maxOutputBytes) + " bytes]");
        warnings << "Compiler stdout was truncated.";
    }
    if (stderrTruncated) {
        rawStdErr.append("\n[OCTOS: stderr truncated after " + QByteArray::number(activeConfig.maxOutputBytes) + " bytes]");
        warnings << "Compiler stderr was truncated.";
    }
    const QString stdoutStr = QString::fromUtf8(rawStdOut);
    const QString stderrStr = QString::fromUtf8(rawStdErr);

    // Read the artifact with a hard size bound and never follow symlinks out of the sandbox.
    bool artifactTruncated = false;
    QString outputFileContent;
    const QString outputFileNameUsed = strategy->outputFileFor(effectiveReq);
    const QString hostOutputFile = outputDir.path() + "/" + outputFileNameUsed;
    QFileInfo outInfo(hostOutputFile);
    if (outInfo.exists() && !dockerSide) {
        const QString canonicalRoot = QFileInfo(outputDir.path()).canonicalFilePath() + "/";
        if (outInfo.isSymLink() || !outInfo.isFile() ||
            !outInfo.canonicalFilePath().startsWith(canonicalRoot)) {
            warnings << "Output artifact ignored: it is not a regular file inside the sandbox.";
        } else {
            QFile outF(hostOutputFile);
            if (outF.open(QIODevice::ReadOnly)) {
                const qint64 limit = activeConfig.maxFileSizeBytes;
                QByteArray fileBytes = outF.read(limit);
                if (outInfo.size() > limit || !outF.atEnd()) {
                    artifactTruncated = true;
                    trimToUtf8Boundary(fileBytes);
                    warnings << QString("Output artifact was truncated at %1 bytes; it is incomplete.").arg(limit);
                }
                outputFileContent = QString::fromUtf8(fileBytes);
            }
        }
    }

    // processOutput() builds a fresh result object: everything measured above is re-applied after it.
    result = strategy->processOutput(req, exitCode, stdoutStr, stderrStr, outputFileContent);
    result.compilerImage = req.compilerImage;
    result.compilerCommand = compilerArgs.join(' ');
    result.stdoutTruncated = stdoutTruncated;
    result.stderrTruncated = stderrTruncated;
    result.artifactTruncated = artifactTruncated;
    result.warnings = warnings;
    result.durationMs = timer.elapsed();
    if (result.state == JobState::Failed) {
        result.failureOrigin = dockerSide ? FailureOrigin::Docker : FailureOrigin::Compiler;
        if (exitCode == 153) {  // 128 + SIGXFSZ: wrote a file larger than the fsize ulimit
            result.failureOrigin = FailureOrigin::Limits;
            result.errorOutput = "The compiler wrote a file larger than the configured size limit and was stopped.\n" + result.errorOutput;
        }
        if (dockerSide && exitCode >= 126 && exitCode <= 127 && result.errorOutput.isEmpty()) {
            result.errorOutput = "The compiler could not be executed inside the image (command not found or not executable).";
        }
    }

    const QString profile = req.compilerImage + (req.userArgs.isEmpty() ? QString() : " " + req.userArgs);
    qint64 totalArtifactBytes = 0;
    int index = 0;
    auto finalize = [&](CompilationArtifact& art, const QString& source) {
        art.id = QString("%1:%2:%3").arg(req.jobId).arg(index++).arg(artifactTypeToString(art.type));
        art.compilationId = req.jobId;
        art.compilerProfile = profile;
        art.source = source;
        art.createdAt = QDateTime::currentDateTimeUtc();
        art.sizeBytes = art.content.toUtf8().size();
        art.computeChecksum();
        totalArtifactBytes += art.sizeBytes;
    };

    if (!result.assemblyOutput.isEmpty()) {
        CompilationArtifact asmArt;
        asmArt.type = (req.language == Language::Java) ? ArtifactType::JVM_Bytecode :
                      (req.language == Language::Python) ? ArtifactType::CPython_Bytecode :
                      (req.language == Language::CSharp) ? ArtifactType::CIL : ArtifactType::Assembly;
        asmArt.name = outputFileNameUsed;
        if (effectiveReq.outputKind == OutputKind::LlvmIr) asmArt.type = ArtifactType::LLVM_IR;
        asmArt.content = result.assemblyOutput;
        asmArt.containerPath = "/output/" + outputFileNameUsed;
        asmArt.truncated = artifactTruncated;
        // Deliberately no host path: the sandbox directory is deleted when this function returns.
        finalize(asmArt, "compiler output file");
        result.addArtifact(asmArt);
    }

    if (!result.rawOutput.isEmpty()) {
        CompilationArtifact rawArt;
        rawArt.type = ArtifactType::RawOutput;
        rawArt.name = "compiler output";
        rawArt.content = result.rawOutput;
        rawArt.truncated = stdoutTruncated || stderrTruncated;
        finalize(rawArt, "compiler stdout/stderr");
        if (totalArtifactBytes + rawArt.sizeBytes <= activeConfig.maxTotalArtifactBytes) {
            result.addArtifact(rawArt);
        } else {
            warnings << "Raw output artifact dropped: total artifact size limit reached.";
            result.warnings = warnings;
        }
    }

    if (!result.diagnostics.isEmpty()) {
        CompilationArtifact diag;
        diag.type = ArtifactType::Diagnostics;
        diag.name = "diagnostics";
        QStringList lines;
        for (const Diagnostic& d : std::as_const(result.diagnostics))
            lines << QString("%1:%2:%3: %4").arg(d.fileName).arg(d.line).arg(d.column).arg(d.message);
        diag.content = lines.join('\n');
        finalize(diag, "parsed compiler diagnostics");
        result.addArtifact(diag);
    }

    {   // Reproducibility record: what ran, how, with which limits and outcome.
        CompilationArtifact meta;
        meta.type = ArtifactType::CompilerMetadata;
        meta.name = "compilation.json";
        meta.mimeType = "application/json";
        QJsonObject o;
        o["image"] = req.compilerImage;
        o["command"] = result.compilerCommand;
        o["exit_code"] = result.exitCode;
        o["state"] = jobStateToString(result.state);
        o["duration_ms"] = static_cast<double>(result.durationMs);
        o["stdout_truncated"] = stdoutTruncated;
        o["stderr_truncated"] = stderrTruncated;
        o["artifact_truncated"] = artifactTruncated;
        o["memory_limit"] = activeConfig.memoryLimit;
        o["cpu_limit"] = activeConfig.cpuLimit;
        o["network"] = activeConfig.networkNone ? "none" : "enabled";
        meta.content = QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented));
        finalize(meta, "OCTOS");
        result.addArtifact(meta);
    }

    return result;
}

void DockerExecutionBackend::cancel(quint64 jobId) {
    // Must never block: it is called from the GUI thread (superseded jobs, closed panes) and from
    // the shutdown loop. It only raises the flag; the worker notices within ~100 ms, kills its
    // docker client and performs the authoritative container cleanup (and records failures).
    QMutexLocker locker(&m_mutex);
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) return;  // unknown or finished: nothing to cancel, nothing to leak
    it->cancelled = true;
}

#include "DockerExecutionConfig.h"
#include "../core/AppSettings.h"
#include <QRegularExpression>

bool DockerExecutionConfig::validate(QString* errorMsg) const {
    // Memory limit format: e.g. "256m", "512m", "1g", "1024m" or plain bytes
    static const QRegularExpression memRegex(R"(^[1-9]\d*[kKmMgG]?$)");
    if (!memRegex.match(memoryLimit).hasMatch()) {
        if (errorMsg) *errorMsg = QString("Invalid Docker memory limit format: '%1'. Expected format like '512m' or '1g'.").arg(memoryLimit);
        return false;
    }

    // CPU limit: float > 0.0 and <= 64.0
    bool ok = false;
    double cpus = cpuLimit.toDouble(&ok);
    if (!ok || cpus <= 0.0 || cpus > 64.0) {
        if (errorMsg) *errorMsg = QString("Invalid Docker CPU limit: '%1'. Expected a positive number up to 64.0.").arg(cpuLimit);
        return false;
    }

    // PIDs limit: between 16 and 4096
    if (pidsLimit < 16 || pidsLimit > 4096) {
        if (errorMsg) *errorMsg = QString("Invalid Docker PIDs limit: %1. Expected value between 16 and 4096.").arg(pidsLimit);
        return false;
    }

    // Timeout: between 1000ms and 300000ms
    if (defaultTimeoutMs < 1000 || defaultTimeoutMs > 300000) {
        if (errorMsg) *errorMsg = QString("Invalid Docker timeout: %1ms. Expected between 1000ms and 300000ms.").arg(defaultTimeoutMs);
        return false;
    }

    // Max output bytes: between 1KB and 100MB
    if (maxOutputBytes < 1024 || maxOutputBytes > 100 * 1024 * 1024) {
        if (errorMsg) *errorMsg = QString("Invalid max output bytes: %1. Expected between 1KB and 100MB.").arg(maxOutputBytes);
        return false;
    }

    if (maxFileSizeBytes < 1024 || maxFileSizeBytes > 100 * 1024 * 1024) {
        if (errorMsg) *errorMsg = QString("Invalid max file size: %1. Expected between 1KB and 100MB.").arg(maxFileSizeBytes);
        return false;
    }

    if (maxSourceBytes < 1024 || maxSourceBytes > 64 * 1024 * 1024) {
        if (errorMsg) *errorMsg = QString("Invalid max source size: %1. Expected between 1KB and 64MB.").arg(maxSourceBytes);
        return false;
    }

    if (maxTotalArtifactBytes < maxFileSizeBytes || maxTotalArtifactBytes > 1024LL * 1024 * 1024) {
        if (errorMsg) *errorMsg = QString("Invalid total artifact size: %1.").arg(maxTotalArtifactBytes);
        return false;
    }

    if (tmpfsSizeMb < 8 || tmpfsSizeMb > 4096) {
        if (errorMsg) *errorMsg = QString("Invalid tmpfs size: %1 MB. Expected between 8 and 4096.").arg(tmpfsSizeMb);
        return false;
    }

    if (dockerProgram.trimmed().isEmpty()) {
        if (errorMsg) *errorMsg = "Docker program must not be empty.";
        return false;
    }

    if (startTimeoutMs < 500 || startTimeoutMs > 60000 || cleanupTimeoutMs < 500 || cleanupTimeoutMs > 120000) {
        if (errorMsg) *errorMsg = "Invalid start/cleanup timeout.";
        return false;
    }

    return true;
}

DockerExecutionConfig DockerExecutionConfig::sanitized() const {
    const DockerExecutionConfig def;
    DockerExecutionConfig c = *this;
    auto invalid = [](const DockerExecutionConfig& probe) { return !probe.validate(); };

    // Probe each field on an otherwise-default config so one bad value cannot reset the others.
    auto probe = [&](auto assign) {
        DockerExecutionConfig p;
        assign(p);
        return !invalid(p);
    };
    if (!probe([&](DockerExecutionConfig& p) { p.memoryLimit = c.memoryLimit; })) c.memoryLimit = def.memoryLimit;
    if (!probe([&](DockerExecutionConfig& p) { p.cpuLimit = c.cpuLimit; })) c.cpuLimit = def.cpuLimit;
    if (!probe([&](DockerExecutionConfig& p) { p.pidsLimit = c.pidsLimit; })) c.pidsLimit = def.pidsLimit;
    if (!probe([&](DockerExecutionConfig& p) { p.defaultTimeoutMs = c.defaultTimeoutMs; })) c.defaultTimeoutMs = def.defaultTimeoutMs;
    if (!probe([&](DockerExecutionConfig& p) { p.maxOutputBytes = c.maxOutputBytes; })) c.maxOutputBytes = def.maxOutputBytes;
    if (!probe([&](DockerExecutionConfig& p) { p.maxFileSizeBytes = c.maxFileSizeBytes; })) c.maxFileSizeBytes = def.maxFileSizeBytes;
    if (!probe([&](DockerExecutionConfig& p) { p.maxSourceBytes = c.maxSourceBytes; })) c.maxSourceBytes = def.maxSourceBytes;
    if (!probe([&](DockerExecutionConfig& p) { p.tmpfsSizeMb = c.tmpfsSizeMb; })) c.tmpfsSizeMb = def.tmpfsSizeMb;
    if (!probe([&](DockerExecutionConfig& p) { p.dockerProgram = c.dockerProgram; })) c.dockerProgram = def.dockerProgram;
    if (!probe([&](DockerExecutionConfig& p) { p.startTimeoutMs = c.startTimeoutMs; })) c.startTimeoutMs = def.startTimeoutMs;
    if (!probe([&](DockerExecutionConfig& p) { p.cleanupTimeoutMs = c.cleanupTimeoutMs; })) c.cleanupTimeoutMs = def.cleanupTimeoutMs;
    if (c.maxTotalArtifactBytes < c.maxFileSizeBytes || c.maxTotalArtifactBytes > 1024LL * 1024 * 1024)
        c.maxTotalArtifactBytes = qMax(def.maxTotalArtifactBytes, c.maxFileSizeBytes);
    return c;
}

DockerExecutionConfig DockerExecutionConfig::fromSettings(const AppSettings& settings) {
    DockerExecutionConfig config;

    QString mem = settings.dockerMemoryLimit();
    if (!mem.isEmpty()) {
        config.memoryLimit = mem;
    }

    QString cpu = settings.dockerCpuLimit();
    if (!cpu.isEmpty()) {
        config.cpuLimit = cpu;
    }

    int pids = settings.dockerPidsLimit();
    if (pids >= 16) {
        config.pidsLimit = pids;
    }

    // Every field is validated on its own; a single corrupted setting no longer
    // discards the other (valid) ones. Only invalid fields fall back to defaults.
    return config.sanitized();
}

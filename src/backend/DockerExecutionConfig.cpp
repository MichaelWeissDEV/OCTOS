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

    return true;
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

    // Ensure resulting config is strictly valid; fall back to safe defaults if settings are corrupted
    QString error;
    if (!config.validate(&error)) {
        DockerExecutionConfig fallback;
        fallback.memoryLimit = "512m";
        fallback.cpuLimit = "1.5";
        fallback.pidsLimit = 128;
        return fallback;
    }

    return config;
}

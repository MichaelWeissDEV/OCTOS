#ifndef DOCKEREXECUTIONCONFIG_H
#define DOCKEREXECUTIONCONFIG_H

#include <QString>

struct DockerExecutionConfig {
    QString memoryLimit{"512m"};
    QString cpuLimit{"1.5"};
    int pidsLimit{128};
    bool networkNone{true};
    bool dropCapabilities{true};
    bool noNewPrivileges{true};
    bool readOnlyRootFs{false}; // Disabled by default for broader compiler compatibility
    int defaultTimeoutMs{30000};
    qint64 maxOutputBytes{10 * 1024 * 1024}; // 10MB limit to prevent memory exhaustion
};

#endif // DOCKEREXECUTIONCONFIG_H

#ifndef DOCKEREXECUTIONCONFIG_H
#define DOCKEREXECUTIONCONFIG_H

#include <QString>

class AppSettings;

struct DockerExecutionConfig {
    QString memoryLimit{"512m"};
    QString cpuLimit{"1.5"};
    int pidsLimit{128};
    bool networkNone{true};
    bool dropCapabilities{true};
    bool noNewPrivileges{true};
    bool readOnlyRootFs{false}; // Disabled by default for broader compiler compatibility
    int defaultTimeoutMs{30000};
    qint64 maxOutputBytes{5 * 1024 * 1024}; // 5MB limit for output streams
    qint64 maxFileSizeBytes{10 * 1024 * 1024}; // 10MB limit for artifacts

    // User execution mode: if empty and runAsCurrentUser is true, maps host uid:gid
    QString runAsUser;
    bool runAsCurrentUser{true};

    bool validate(QString* errorMsg = nullptr) const;
    static DockerExecutionConfig fromSettings(const AppSettings& settings);
};

#endif // DOCKEREXECUTIONCONFIG_H

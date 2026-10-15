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

    // Additional resource bounds
    qint64 maxSourceBytes{1024 * 1024};            // 1MB of submitted source
    qint64 maxTotalArtifactBytes{20 * 1024 * 1024}; // sum of all artifacts taken over from a job
    int tmpfsSizeMb{64};                           // size of the bounded /tmp tmpfs
    bool disableSwap{true};                        // --memory-swap == --memory
    bool ipcNone{true};                            // --ipc none

    // Process handling
    QString dockerProgram{"docker"};               // overridable (tests use a fake client)
    int startTimeoutMs{5000};
    int cleanupTimeoutMs{8000};

    bool validate(QString* errorMsg = nullptr) const;

    // Returns a copy in which every individually invalid field was replaced by its default.
    DockerExecutionConfig sanitized() const;
    static DockerExecutionConfig fromSettings(const AppSettings& settings);
};

#endif // DOCKEREXECUTIONCONFIG_H

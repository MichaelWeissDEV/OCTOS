#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QString>

// Storage format for OCTOS settings: native by default, INI inside $OCTOS_SETTINGS_DIR when that
// variable is set (tests, portable setups). Every QSettings of the application must use it.
QSettings::Format octosSettingsFormat();

class AppSettings {
public:
    static AppSettings& instance();

    bool autoCompile() const;
    void setAutoCompile(bool enabled);

    int debounceIntervalMs() const;
    void setDebounceIntervalMs(int ms);

    int maxParallelJobs() const;
    void setMaxParallelJobs(int jobs);

    int themeIndex() const;
    void setThemeIndex(int index);

    QString dockerMemoryLimit() const;
    void setDockerMemoryLimit(const QString& limit);

    QString dockerCpuLimit() const;
    void setDockerCpuLimit(const QString& limit);

    int dockerPidsLimit() const;
    void setDockerPidsLimit(int limit);

    // Images OCTOS itself pulled (and therefore may offer to delete). Images that were already
    // present are never recorded here.
    QStringList octosPulledImages() const;
    void addOctosPulledImage(const QString& image);
    void removeOctosPulledImage(const QString& image);

    QByteArray compilerProfilesJson() const;
    void setCompilerProfilesJson(const QByteArray& json);

    QByteArray toolchainProbesJson() const;
    void setToolchainProbesJson(const QByteArray& json);

    // Window layout (geometry, dock state, splitter sizes), stored as opaque blobs.
    QByteArray layoutBlob(const QString& name) const;
    void setLayoutBlob(const QString& name, const QByteArray& data);
    void clearLayout();

    QString lastDirectory() const;
    void setLastDirectory(const QString& dir);

    void sync();

private:
    AppSettings();
    ~AppSettings() = default;
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;

    QSettings m_settings;
};

#endif // APPSETTINGS_H

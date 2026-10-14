#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QString>

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

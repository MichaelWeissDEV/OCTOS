#include "AppSettings.h"
#include <QThread>

AppSettings& AppSettings::instance() {
    static AppSettings inst;
    return inst;
}

AppSettings::AppSettings() : m_settings("OCTOS", "OCTOS") {
}

bool AppSettings::autoCompile() const {
    return m_settings.value("Editor/AutoCompile", true).toBool();
}

void AppSettings::setAutoCompile(bool enabled) {
    m_settings.setValue("Editor/AutoCompile", enabled);
}

int AppSettings::debounceIntervalMs() const {
    int val = m_settings.value("Editor/DebounceIntervalMs", 300).toInt();
    if (val < 50) val = 50;
    if (val > 5000) val = 5000;
    return val;
}

void AppSettings::setDebounceIntervalMs(int ms) {
    if (ms < 50) ms = 50;
    if (ms > 5000) ms = 5000;
    m_settings.setValue("Editor/DebounceIntervalMs", ms);
}

int AppSettings::maxParallelJobs() const {
    int defaultJobs = qMax(2, QThread::idealThreadCount());
    int val = m_settings.value("Backend/MaxParallelJobs", defaultJobs).toInt();
    if (val < 1) val = 1;
    if (val > 16) val = 16;
    return val;
}

void AppSettings::setMaxParallelJobs(int jobs) {
    if (jobs < 1) jobs = 1;
    if (jobs > 16) jobs = 16;
    m_settings.setValue("Backend/MaxParallelJobs", jobs);
}

int AppSettings::themeIndex() const {
    return m_settings.value("UI/ThemeIndex", 0).toInt();
}

void AppSettings::setThemeIndex(int index) {
    if (index < 0) index = 0;
    m_settings.setValue("UI/ThemeIndex", index);
}

QString AppSettings::dockerMemoryLimit() const {
    QString limit = m_settings.value("Docker/MemoryLimit", "512m").toString().trimmed();
    if (limit.isEmpty()) limit = "512m";
    return limit;
}

void AppSettings::setDockerMemoryLimit(const QString& limit) {
    m_settings.setValue("Docker/MemoryLimit", limit.trimmed());
}

QString AppSettings::dockerCpuLimit() const {
    QString limit = m_settings.value("Docker/CpuLimit", "1.5").toString().trimmed();
    if (limit.isEmpty()) limit = "1.5";
    return limit;
}

void AppSettings::setDockerCpuLimit(const QString& limit) {
    m_settings.setValue("Docker/CpuLimit", limit.trimmed());
}

int AppSettings::dockerPidsLimit() const {
    int val = m_settings.value("Docker/PidsLimit", 128).toInt();
    if (val < 16) val = 16;
    if (val > 1024) val = 1024;
    return val;
}

void AppSettings::setDockerPidsLimit(int limit) {
    if (limit < 16) limit = 16;
    if (limit > 1024) limit = 1024;
    m_settings.setValue("Docker/PidsLimit", limit);
}

QString AppSettings::lastDirectory() const {
    return m_settings.value("General/LastDirectory", "").toString();
}

void AppSettings::setLastDirectory(const QString& dir) {
    m_settings.setValue("General/LastDirectory", dir);
}

void AppSettings::sync() {
    m_settings.sync();
}

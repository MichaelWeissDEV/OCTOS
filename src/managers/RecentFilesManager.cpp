# 1 "./src/managers/RecentFilesManager.cpp"
#include "RecentFilesManager.h"

#include <QFileInfo>

RecentFilesManager::RecentFilesManager() : m_maxRecentFiles(10), m_settings("OCTOS", "OCTOS") {
    loadFromSettings();
}

void RecentFilesManager::addRecentFile(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) return;

    m_recentFiles.removeAll(filePath);

    m_recentFiles.prepend(filePath);

    while (m_recentFiles.size() > m_maxRecentFiles) {
        m_recentFiles.removeLast();
    }

    saveToSettings();
}

void RecentFilesManager::removeRecentFile(const QString &filePath) {
    m_recentFiles.removeAll(filePath);
    saveToSettings();
}

void RecentFilesManager::clearRecentFiles() {
    m_recentFiles.clear();
    saveToSettings();
}

QStringList RecentFilesManager::getRecentFiles() const { return m_recentFiles; }

bool RecentFilesManager::saveToSettings() {
    m_settings.beginGroup("RecentFiles");
    m_settings.remove("");

    for (int i = 0; i < m_recentFiles.size(); ++i) {
        m_settings.setValue(QString("file_%1").arg(i), m_recentFiles.at(i));
    }

    m_settings.setValue("count", m_recentFiles.size());
    m_settings.endGroup();

    return m_settings.status() == QSettings::NoError;
}

bool RecentFilesManager::loadFromSettings() {
    m_recentFiles.clear();

    m_settings.beginGroup("RecentFiles");
    int count = m_settings.value("count", 0).toInt();

    for (int i = 0; i < count; ++i) {
        QString file = m_settings.value(QString("file_%1").arg(i), "").toString();
        if (!file.isEmpty()) {
            m_recentFiles.append(file);
        }
    }

    m_settings.endGroup();

    return !m_recentFiles.isEmpty();
}

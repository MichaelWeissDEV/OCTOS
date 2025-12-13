# 1 "./src/managers/RecentFilesManager.h"
#ifndef RECENTFILESMANAGER_H
#define RECENTFILESMANAGER_H

#include <QSettings>
#include <QString>
#include <QStringList>

class RecentFilesManager {
   public:
    RecentFilesManager();

    void addRecentFile(const QString &filePath);
    void removeRecentFile(const QString &filePath);
    void clearRecentFiles();

    QStringList getRecentFiles() const;
    void setMaxRecentFiles(int max) { m_maxRecentFiles = max; }

    bool saveToSettings();
    bool loadFromSettings();

   private:
    QStringList m_recentFiles;
    int m_maxRecentFiles;
    QSettings m_settings;
};

#endif

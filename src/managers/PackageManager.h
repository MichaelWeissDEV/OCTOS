# 1 "./src/managers/PackageManager.h"
#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include <QMap>
#include <QString>
#include <QStringList>

enum class PackageManagerType { APT, PACMAN, DNF, ZYPPER, UNKNOWN };

struct Package {
    QString name;
    QString version;
    QString description;
    bool installed = false;
};

class PackageManager {
   public:
    PackageManager();

    PackageManagerType detectPackageManager();

    bool installPackages(const QStringList &packages) const;

    QList<Package> searchPackages(const QString &query);

    bool isPackageInstalled(const QString &packageName);

    QString getPackageManagerName() const;

    QStringList getCompilerPackages(const QString &language);

   private:
    QString buildInstallCommand(const QStringList &packages) const;
    QString executeWithPrivilegeEscalation(const QString &command) const;
    PackageManagerType m_detectedManager;
    QString m_managerPath;
};

#endif

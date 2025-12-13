# 1 "./src/managers/PackageManager.cpp"
#include "PackageManager.h"

#include <QFile>
#include <QProcess>
#include <QStandardPaths>

PackageManager::PackageManager() : m_detectedManager(PackageManagerType::UNKNOWN) {
    detectPackageManager();
}

PackageManagerType PackageManager::detectPackageManager() {
    if (QFile::exists("/usr/bin/apt")) {
        m_detectedManager = PackageManagerType::APT;
        m_managerPath = "/usr/bin/apt";
        return m_detectedManager;
    }

    if (QFile::exists("/usr/bin/pacman")) {
        m_detectedManager = PackageManagerType::PACMAN;
        m_managerPath = "/usr/bin/pacman";
        return m_detectedManager;
    }

    if (QFile::exists("/usr/bin/dnf")) {
        m_detectedManager = PackageManagerType::DNF;
        m_managerPath = "/usr/bin/dnf";
        return m_detectedManager;
    }

    if (QFile::exists("/usr/bin/zypper")) {
        m_detectedManager = PackageManagerType::ZYPPER;
        m_managerPath = "/usr/bin/zypper";
        return m_detectedManager;
    }

    m_detectedManager = PackageManagerType::UNKNOWN;
    return m_detectedManager;
}

bool PackageManager::installPackages(const QStringList &packages) const {
    if (m_detectedManager == PackageManagerType::UNKNOWN) {
        return false;
    }

    QString command = buildInstallCommand(packages);
    QString result = executeWithPrivilegeEscalation(command);

    return result.contains("Success") || !result.contains("Error");
}

QString PackageManager::buildInstallCommand(const QStringList &packages) const {
    QString packageList = packages.join(" ");

    switch (m_detectedManager) {
        case PackageManagerType::APT:
            return "apt-get install -y " + packageList;

        case PackageManagerType::PACMAN:
            return "pacman -S --noconfirm " + packageList;

        case PackageManagerType::DNF:
            return "dnf install -y " + packageList;

        case PackageManagerType::ZYPPER:
            return "zypper install -n " + packageList;

        default:
            return "";
    }
}

QString PackageManager::executeWithPrivilegeEscalation(const QString &command) const {
    QProcess process;

    if (!QFile::exists("/usr/bin/pkexec")) {
        return "Error: pkexec not found";
    }

    process.start("/usr/bin/pkexec", QStringList() << "sh" << "-c" << command);

    if (!process.waitForFinished(300000)) {
        return "Error: Installation timeout";
    }

    if (process.exitCode() != 0) {
        QString errorOutput = QString::fromUtf8(process.readAllStandardError());
        return "Error: " + errorOutput;
    }

    return "Success";
}

QList<Package> PackageManager::searchPackages(const QString &query) {
    QList<Package> results;

    if (m_detectedManager == PackageManagerType::UNKNOWN) {
        return results;
    }

    QProcess process;
    QStringList searchCmd;

    switch (m_detectedManager) {
        case PackageManagerType::APT:
            process.start("apt", QStringList() << "search" << query);
            break;

        case PackageManagerType::PACMAN:
            process.start("pacman", QStringList() << "-Ss" << query);
            break;

        case PackageManagerType::DNF:
            process.start("dnf", QStringList() << "search" << query);
            break;

        case PackageManagerType::ZYPPER:
            process.start("zypper", QStringList() << "search" << query);
            break;

        default:
            break;
    }

    if (process.waitForFinished(10000)) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());

        QStringList lines = output.split('\n', Qt::SkipEmptyParts);

        for (const QString &line : lines) {
            if (!line.isEmpty() && !line.contains("---")) {
                Package pkg;
                pkg.name = line.split('/').first().trimmed();
                results.append(pkg);
            }
        }
    }

    return results;
}

bool PackageManager::isPackageInstalled(const QString &packageName) {
    switch (m_detectedManager) {
        case PackageManagerType::APT: {
            QProcess process;
            process.start("dpkg", QStringList() << "-l" << packageName);
            process.waitForFinished(5000);
            return process.exitCode() == 0;
        }

        case PackageManagerType::PACMAN: {
            QProcess process;
            process.start("pacman", QStringList() << "-Q" << packageName);
            process.waitForFinished(5000);
            return process.exitCode() == 0;
        }

        case PackageManagerType::DNF:
        case PackageManagerType::ZYPPER: {
            QProcess process;
            process.start("which", QStringList() << packageName);
            process.waitForFinished(5000);
            return process.exitCode() == 0;
        }

        default:
            return false;
    }
}

QString PackageManager::getPackageManagerName() const {
    switch (m_detectedManager) {
        case PackageManagerType::APT:
            return "APT (Debian/Ubuntu)";
        case PackageManagerType::PACMAN:
            return "Pacman (Arch)";
        case PackageManagerType::DNF:
            return "DNF (Fedora)";
        case PackageManagerType::ZYPPER:
            return "Zypper (openSUSE)";
        default:
            return "Unknown";
    }
}

QStringList PackageManager::getCompilerPackages(const QString &language) {
    QMap<QString, QStringList> compilerMap = {
        {"cpp", {"g++", "clang", "clang++"}},
        {"c", {"gcc", "clang"}},
        {"java", {"openjdk-17-jdk", "openjdk-18-jdk"}},
        {"csharp", {"dotnet", "mono"}},
        {"python", {"python3", "python3-pip", "python3-numba"}},
    };

    return compilerMap.value(language, QStringList());
}

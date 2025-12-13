# 1 "./src/CompilerDetector.cpp"
#include "CompilerDetector.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

CompilerDetectorWorker::CompilerDetectorWorker() {}

void CompilerDetectorWorker::detectAllCompilers() {
    emit detectionProgress("Detecting compilers...");

    QList<CompilerInfo> allCompilers;

    emit detectionProgress("Scanning for GCC...");
    allCompilers.append(detectGCC());

    emit detectionProgress("Scanning for Clang...");
    allCompilers.append(detectClang());

    emit detectionProgress("Scanning for ARM compilers...");
    allCompilers.append(detectARM());

    emit detectionProgress("Scanning for SPARK/Ada...");
    allCompilers.append(detectSpark());

    emit detectionsComplete(allCompilers);
}

QList<CompilerInfo> CompilerDetectorWorker::detectGCC() {
    QList<CompilerInfo> result;
    QStringList patterns = {"g++",
                            "g++-*",
                            "gcc",
                            "gcc-*",
                            "/usr/bin/g++*",
                            "/usr/bin/gcc*",
                            "/usr/local/bin/g++*",
                            "/usr/local/bin/gcc*"};

    for (const QString &pattern : patterns) {
        QStringList found = findCompilersInPath(pattern);
        for (const QString &compiler : found) {
            if (isCompilerAvailable(compiler)) {
                CompilerInfo info = getCompilerInfo(compiler);
                bool isDuplicate = false;
                for (const CompilerInfo &existing : result) {
                    if (existing.path == info.path) {
                        isDuplicate = true;
                        break;
                    }
                }
                if (!isDuplicate) {
                    result.append(info);
                }
            }
        }
    }

    return result;
}

QList<CompilerInfo> CompilerDetectorWorker::detectClang() {
    QList<CompilerInfo> result;
    QStringList patterns = {"clang++",
                            "clang++-*",
                            "clang",
                            "clang-*",
                            "/usr/bin/clang++*",
                            "/usr/bin/clang*",
                            "/usr/local/bin/clang++*",
                            "/usr/local/bin/clang*"};

    for (const QString &pattern : patterns) {
        QStringList found = findCompilersInPath(pattern);
        for (const QString &compiler : found) {
            if (isCompilerAvailable(compiler)) {
                CompilerInfo info = getCompilerInfo(compiler);
                bool isDuplicate = false;
                for (const CompilerInfo &existing : result) {
                    if (existing.path == info.path) {
                        isDuplicate = true;
                        break;
                    }
                }
                if (!isDuplicate) {
                    result.append(info);
                }
            }
        }
    }

    return result;
}

QList<CompilerInfo> CompilerDetectorWorker::detectARM() {
    QList<CompilerInfo> result;
    QStringList patterns = {"arm-linux-gnueabihf-g++*",        "arm-linux-gnueabihf-gcc*",
                            "aarch64-linux-gnu-g++*",          "aarch64-linux-gnu-gcc*",
                            "armv7l-rpi-linux-gnueabihf-g++*", "armv7l-rpi-linux-gnueabihf-gcc*"};

    for (const QString &pattern : patterns) {
        QStringList found = findCompilersInPath(pattern);
        for (const QString &compiler : found) {
            if (isCompilerAvailable(compiler)) {
                CompilerInfo info = getCompilerInfo(compiler);
                bool isDuplicate = false;
                for (const CompilerInfo &existing : result) {
                    if (existing.path == info.path) {
                        isDuplicate = true;
                        break;
                    }
                }
                if (!isDuplicate) {
                    result.append(info);
                }
            }
        }
    }

    return result;
}

QList<CompilerInfo> CompilerDetectorWorker::detectSpark() {
    QList<CompilerInfo> result;

    QStringList patterns = {"gprbuild*", "gnatmake*", "gnat*"};

    for (const QString &pattern : patterns) {
        QStringList found = findCompilersInPath(pattern);
        for (const QString &compiler : found) {
            if (isCompilerAvailable(compiler)) {
                CompilerInfo info = getCompilerInfo(compiler);
                bool isDuplicate = false;
                for (const CompilerInfo &existing : result) {
                    if (existing.path == info.path) {
                        isDuplicate = true;
                        break;
                    }
                }
                if (!isDuplicate) {
                    result.append(info);
                }
            }
        }
    }

    return result;
}

QString CompilerDetector::getInstallCommand(const QString &compilerName, const QString &distro) {
    QMap<QString, QMap<QString, QString>> installCommands = {
        {"gcc",
         {{"ubuntu", "apt-get install -y build-essential"},
          {"debian", "apt-get install -y build-essential"},
          {"fedora", "dnf install -y gcc gcc-c++"},
          {"arch", "pacman -S --noconfirm base-devel"},
          {"opensuse", "zypper install -y gcc gcc-c++"}}},
        {"clang",
         {{"ubuntu", "apt-get install -y clang"},
          {"debian", "apt-get install -y clang"},
          {"fedora", "dnf install -y clang"},
          {"arch", "pacman -S --noconfirm clang"},
          {"opensuse", "zypper install -y clang"}}},
        {"arm-compiler",
         {{"ubuntu", "apt-get install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf"},
          {"debian", "apt-get install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf"},
          {"fedora", "dnf install -y arm-linux-gnu-gcc arm-linux-gnu-g++"},
          {"arch", "pacman -S --noconfirm arm-none-eabi-gcc"},
          {"opensuse", "zypper install -y cross-arm-none-eabi-gcc"}}}};

    if (installCommands.contains(compilerName) && installCommands[compilerName].contains(distro)) {
        return installCommands[compilerName][distro];
    }

    return QString();
}

bool CompilerDetector::isCompilerAvailable(const QString &compilerPath) {
    QProcess process;
    process.start(compilerPath, QStringList() << "--version");
    return process.waitForFinished(2000) && process.exitCode() == 0;
}

QString CompilerDetector::detectLinuxDistro() {
    CompilerDetectorWorker worker;
    return worker.detectLinuxDistro();
}

CompilerInfo CompilerDetector::getCompilerInfo(const QString &compilerPath) {
    CompilerDetectorWorker worker;
    return worker.getCompilerInfo(compilerPath);
}

CompilerDetector::CompilerDetector(QObject *parent)
    : QObject(parent), m_detectorThread(nullptr), m_worker(nullptr) {}

CompilerDetector::~CompilerDetector() {
    if (m_detectorThread) {
        m_detectorThread->quit();
        m_detectorThread->wait();
        delete m_detectorThread;
    }
}

void CompilerDetector::detectAsyncronously() {
    m_detectorThread = new QThread(this);
    m_worker = new CompilerDetectorWorker();
    m_worker->moveToThread(m_detectorThread);

    connect(m_detectorThread, &QThread::started, m_worker,
            &CompilerDetectorWorker::detectAllCompilers);
    connect(m_worker, &CompilerDetectorWorker::detectionsComplete, this,
            &CompilerDetector::onDetectionsComplete);
    connect(m_worker, &CompilerDetectorWorker::detectionProgress, this,
            &CompilerDetector::detectionProgress);
    connect(m_worker, &QObject::destroyed, m_detectorThread, &QThread::quit);

    m_detectorThread->start();
}

void CompilerDetector::onDetectionsComplete(const QList<CompilerInfo> &allCompilers) {
    m_allDetectedCompilers = allCompilers;
    emit detectionsComplete(allCompilers);
}

QList<CompilerInfo> CompilerDetector::detectGCC() {
    CompilerDetectorWorker worker;
    return worker.detectGCC();
}

QList<CompilerInfo> CompilerDetector::detectClang() {
    CompilerDetectorWorker worker;
    return worker.detectClang();
}

QList<CompilerInfo> CompilerDetector::detectARM() {
    CompilerDetectorWorker worker;
    return worker.detectARM();
}

QList<CompilerInfo> CompilerDetector::detectSpark() {
    CompilerDetectorWorker worker;
    return worker.detectSpark();
}

QList<CompilerInfo> CompilerDetector::getAllDetectedCompilers() const {
    return m_allDetectedCompilers;
}

QStringList CompilerDetectorWorker::findCompilersInPath(const QString &pattern) {
    QStringList result;

    QStringList paths = {"/usr/bin", "/usr/local/bin", "/opt/gcc/bin", "/opt/llvm/bin"};

    for (const QString &path : paths) {
        QDir dir(path);
        if (!dir.exists()) continue;

        QFileInfoList entries =
            dir.entryInfoList(QDir::Files | QDir::Executable | QDir::NoDotAndDotDot);

        for (const QFileInfo &entry : entries) {
            QString fileName = entry.fileName();

            if (pattern.contains("*")) {
                QString regexPattern = pattern;
                regexPattern.replace("*", ".*");
                if (fileName.contains(QRegularExpression(regexPattern))) {
                    result.append(entry.absoluteFilePath());
                }
            } else if (fileName == pattern) {
                result.append(entry.absoluteFilePath());
            }
        }
    }

    return result;
}

QString CompilerDetectorWorker::getCompilerVersion(const QString &compilerPath) {
    QProcess process;
    process.start(compilerPath, QStringList() << "--version");

    if (process.waitForFinished(2000)) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QStringList lines = output.split('\n');
        if (!lines.isEmpty()) {
            QString firstLine = lines.first();
            QStringList parts = firstLine.split(' ');
            for (int i = 0; i < parts.size(); ++i) {
                if (parts[i].contains(QRegularExpression("\\d+\\.\\d+"))) {
                    return parts[i];
                }
            }
        }
    }

    return "unknown";
}

CompilerInfo CompilerDetectorWorker::getCompilerInfo(const QString &compilerPath) {
    CompilerInfo info;
    info.path = compilerPath;
    info.installed = isCompilerAvailable(compilerPath);
    info.version = getCompilerVersion(compilerPath);

    QString fileName = QFileInfo(compilerPath).fileName();

    if (fileName.contains("clang")) {
        info.name = "Clang";
        info.target = "x86_64";
    } else if (fileName.contains("aarch64") || fileName.contains("arm64")) {
        info.name = "GCC";
        info.target = "aarch64";
    } else if (fileName.contains("arm-linux-gnueabihf")) {
        info.name = "GCC";
        info.target = "arm32";
    } else if (fileName.contains("armv7l") || fileName.contains("rpi")) {
        info.name = "GCC";
        info.target = "armv7l";
    } else if (fileName.contains("g++") || fileName.contains("gcc")) {
        info.name = "GCC";
        info.target = "x86_64";
    } else {
        info.name = fileName;
        info.target = "unknown";
    }

    return info;
}

bool CompilerDetectorWorker::isCompilerAvailable(const QString &compilerPath) {
    QProcess process;
    process.start(compilerPath, QStringList() << "--version");
    return process.waitForFinished(2000) && process.exitCode() == 0;
}

QString CompilerDetectorWorker::detectLinuxDistro() {
    QStringList files = {"/etc/os-release", "/etc/lsb-release"};

    for (const QString &file : files) {
        QFile f(file);
        if (f.open(QIODevice::ReadOnly)) {
            QString content = QString::fromUtf8(f.readAll());
            f.close();

            if (content.contains("Ubuntu") || content.contains("ubuntu")) return "ubuntu";
            if (content.contains("Debian") || content.contains("debian")) return "debian";
            if (content.contains("Fedora") || content.contains("fedora")) return "fedora";
            if (content.contains("Arch") || content.contains("arch")) return "arch";
            if (content.contains("openSUSE") || content.contains("opensuse")) return "opensuse";
        }
    }

    return "unknown";
}

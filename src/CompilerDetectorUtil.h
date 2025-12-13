# 1 "./src/CompilerDetectorUtil.h"
#ifndef COMPILERDETECTOR_H
#define COMPILERDETECTOR_H

#include <QMap>
#include <QProcess>
#include <QString>
#include <QStringList>

#include "CompilerDriver.h"

struct CompilerInfo {
    QString name;
    QString path;
    QString version;
    Language language;
};

class CompilerDetectorUtil {
   public:
    static QStringList getCompilersForLanguage(Language lang) {
        QStringList compilers;

        switch (lang) {
            case Language::Cpp:
            case Language::C:
                compilers << "g++" << "clang++" << "gcc" << "clang" << "cc";
                break;
            case Language::Java:
                compilers << "javac" << "java";
                break;
            case Language::Python:
                compilers << "python3" << "python3.11" << "python3.10" << "python3.9" << "python";
                break;
            case Language::CSharp:
                compilers << "csc" << "mcs" << "dotnet";
                break;
            case Language::Rust:
                compilers << "rustc" << "cargo";
                break;
            default:
                break;
        }

        return compilers;
    }

    static QStringList getAllCompilerVersions(Language lang);

    static bool compilerExists(const QString &compiler);

    static QStringList getAvailableCompilers(Language lang);

    static QString getCompilerVersion(const QString &compiler);
};

inline bool CompilerDetectorUtil::compilerExists(const QString &compiler) {
    QProcess process;
    process.start("sh", QStringList()
                            << "-c" << QString("which %1 > /dev/null 2>&1").arg(compiler));

    if (!process.waitForFinished(2000)) {
        process.kill();
        return false;
    }

    return process.exitCode() == 0;
}

inline QStringList CompilerDetectorUtil::getAllCompilerVersions(Language lang) {
    QStringList result;
    QStringList baseCompilers;

    switch (lang) {
        case Language::Cpp:
        case Language::C:
            baseCompilers << "g++" << "gcc" << "clang++" << "clang" << "cc";
            break;
        case Language::Python:
            baseCompilers << "python3" << "python" << "python3.11" << "python3.10" << "python3.9"
                          << "python3.8" << "python3.7";
            break;
        case Language::Java:
            baseCompilers << "javac" << "java";
            break;
        case Language::CSharp:
            baseCompilers << "csc" << "mcs" << "dotnet";
            break;
        case Language::Rust:
            baseCompilers << "rustc" << "cargo";
            break;
        default:
            break;
    }

    for (const QString &base : baseCompilers) {
        if (compilerExists(base)) {
            result.append(base);
        }
    }

    if (lang == Language::Cpp || lang == Language::C) {
        for (int ver = 4; ver <= 15; ++ver) {
            QString versioned = QString("g++-") + QString::number(ver);
            if (lang == Language::C) versioned = QString("gcc-") + QString::number(ver);

            if (compilerExists(versioned) && !result.contains(versioned)) {
                result.append(versioned);
            }
        }
    }

    return result;
}

inline QStringList CompilerDetectorUtil::getAvailableCompilers(Language lang) {
    QStringList all = getCompilersForLanguage(lang);
    QStringList available;

    for (const QString &compiler : all) {
        if (compilerExists(compiler)) {
            available.append(compiler);
        }
    }

    return available;
}

inline QString CompilerDetectorUtil::getCompilerVersion(const QString &compiler) {
    QProcess process;
    QString cmd = QString("%1 --version 2>&1").arg(compiler);
    process.start("sh", QStringList() << "-c" << cmd);

    if (!process.waitForFinished(2000)) {
        process.kill();
        return "unknown";
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n');

    if (!lines.isEmpty()) {
        return lines[0];
    }

    return "unknown";
}

#endif

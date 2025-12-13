# 1 "./src/CompilerDriver.cpp"
#include "CompilerDriver.h"

#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QTemporaryFile>
#include <QTextStream>

CompilerDriver::CompilerDriver() {}

QString CompilerDriver::getCompilerForLanguage(Language lang) {
    switch (lang) {
        case Language::C:
            return "gcc";
        case Language::Cpp:
            return "g++";
        case Language::Rust:
            return "rustc";
        case Language::CSharp:
            return "csc";
        case Language::Java:
            return "javac";
    }
    return "g++";
}

QStringList CompilerDriver::getAvailableCompilers(const QString &baseName) {
    QStringList result;

    for (int version = 8; version <= 14; ++version) {
        QString compiler = baseName + "-" + QString::number(version);

        QProcess process;
        process.start(compiler, QStringList() << "--version");

        if (process.waitForStarted(1000) && process.waitForFinished(1000)) {
            if (process.exitCode() == 0) {
                result << compiler;
            }
        }
    }

    result.prepend(baseName);

    result.removeDuplicates();

    return result;
}

CompileResult CompilerDriver::compile(const QString &source, const QString &compiler,
                                      const QString &flags, Language lang, SyntaxFlavor flavor) {
    CompileResult result;
    result.success = false;

    QString extension;
    switch (lang) {
        case Language::C:
            extension = ".c";
            break;
        case Language::Cpp:
            extension = ".cpp";
            break;
        case Language::Rust:
            extension = ".rs";
            break;
        case Language::CSharp:
            extension = ".cs";
            break;
        case Language::Java:
            extension = ".java";
            break;
    }

    QTemporaryFile tempFile(QDir::tempPath() + "/XXXXXX" + extension);
    tempFile.setAutoRemove(true);

    if (!tempFile.open()) {
        result.output = "Error: Could not create temporary file.";
        return result;
    }

    QTextStream stream(&tempFile);
    stream << source;
    stream.flush();
    tempFile.close();

    QStringList args;

    if (!flags.trimmed().isEmpty()) {
        static QRegularExpression splitRegex(R"(("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|\S+))");
        auto matches = splitRegex.globalMatch(flags);
        while (matches.hasNext()) {
            auto match = matches.next();
            QString arg = match.captured(1);
            if ((arg.startsWith('"') && arg.endsWith('"')) ||
                (arg.startsWith('\'') && arg.endsWith('\''))) {
                arg = arg.mid(1, arg.length() - 2);
            }
            args << arg;
        }
    }

    args << "-S";
    args << "-masm=" + QString(flavor == SyntaxFlavor::Intel ? "intel" : "att");
    args << "-fno-asynchronous-unwind-tables";
    args << "-o" << "-";
    args << tempFile.fileName();

    QProcess process;
    process.start(compiler, args);

    if (!process.waitForStarted(5000)) {
        result.output = "Error: Could not start compiler: " + compiler;
        return result;
    }

    if (!process.waitForFinished(30000)) {
        result.output = "Error: Compiler timed out.";
        process.kill();
        return result;
    }

    if (process.exitCode() != 0) {
        result.output = QString::fromUtf8(process.readAllStandardError());
        return result;
    }

    QString rawAsm = QString::fromUtf8(process.readAllStandardOutput());
    result.output = cleanAssembly(rawAsm);
    result.lineMapping = buildLineMapping(rawAsm);
    result.success = true;

    return result;
}

QString CompilerDriver::cleanAssembly(const QString &rawAsm) {
    QStringList lines = rawAsm.split('\n');
    QString output;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.isEmpty()) continue;

        if (trimmed.startsWith(".")) {
            continue;
        }

        if (trimmed.length() <= 3 && trimmed.endsWith(":")) {
            bool allDigits = true;
            for (const QChar &c : trimmed.left(trimmed.length() - 1)) {
                if (!c.isDigit()) {
                    allDigits = false;
                    break;
                }
            }
            if (allDigits) {
                continue;
            }
        }

        output.append(line + "\n");
    }

    return output;
}

QMap<int, int> CompilerDriver::buildLineMapping(const QString &rawAsm) {
    QMap<int, int> mapping;

    QStringList lines = rawAsm.split('\n');
    int asmLine = 0;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith(".loc ")) {
            QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                bool ok;
                int srcLine = parts[2].toInt(&ok);
                if (ok) {
                    mapping[srcLine] = asmLine;
                }
            }
        }

        if (!trimmed.isEmpty()) {
            asmLine++;
        }
    }

    return mapping;
}

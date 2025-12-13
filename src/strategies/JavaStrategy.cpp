# 1 "./src/strategies/JavaStrategy.cpp"
#include "JavaStrategy.h"

#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>

JavaStrategy::JavaStrategy() {}

CompilationResult JavaStrategy::compile(const QString &source, const QString &compiler,
                                        const QString &flags) {
    CompilationResult result;
    result.success = false;

    QString className = extractClassName(source);
    if (className.isEmpty()) {
        result.errorMessage = "Could not find public class definition";
        return result;
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        result.errorMessage = "Failed to create temporary directory";
        return result;
    }

    QString javaFile = tempDir.path() + "/" + className + ".java";
    QFile sourceFile(javaFile);
    if (!sourceFile.open(QIODevice::WriteOnly)) {
        result.errorMessage = "Failed to create Java source file";
        return result;
    }

    sourceFile.write(source.toUtf8());
    sourceFile.close();

    QProcess javac;
    QStringList javacArgs;
    if (!flags.isEmpty()) {
        javacArgs.append(flags.split(" ", Qt::SkipEmptyParts));
    }
    javacArgs.append(javaFile);

    javac.setWorkingDirectory(tempDir.path());
    javac.start("javac", javacArgs);

    if (!javac.waitForFinished(30000)) {
        result.errorMessage = "Compilation timeout";
        return result;
    }

    if (javac.exitCode() != 0) {
        result.errorMessage = QString::fromUtf8(javac.readAllStandardError());
        return result;
    }

    QProcess javap;
    QStringList javapArgs = {"-c", "-v", className};
    javap.setWorkingDirectory(tempDir.path());
    javap.start("javap", javapArgs);

    if (!javap.waitForFinished(10000)) {
        result.errorMessage = "Disassembly timeout";
        return result;
    }

    if (javap.exitCode() != 0) {
        result.errorMessage = QString::fromUtf8(javap.readAllStandardError());
        return result;
    }

    result.primaryOutput = QString::fromUtf8(javap.readAllStandardOutput());
    result.success = true;

    return result;
}

QString JavaStrategy::getHighlightingRules() { return "java"; }

QStringList JavaStrategy::getRecommendedCompilers() const {
    return {"javac", "openjdk-17-jdk", "openjdk-18-jdk", "openjdk-19-jdk"};
}

QString JavaStrategy::extractClassName(const QString &source) {
    QRegularExpression classRegex(R"(public\s+class\s+(\w+))");
    QRegularExpressionMatch match = classRegex.match(source);

    if (match.hasMatch()) {
        return match.captured(1);
    }

    classRegex.setPattern(R"(class\s+(\w+))");
    match = classRegex.match(source);

    if (match.hasMatch()) {
        return match.captured(1);
    }

    return QString();
}

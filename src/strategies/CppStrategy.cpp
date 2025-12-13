# 1 "./src/strategies/CppStrategy.cpp"
#include "CppStrategy.h"

#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryFile>

CppStrategy::CppStrategy() {}

CompilationResult CppStrategy::compile(const QString &source, const QString &compiler,
                                       const QString &flags) {
    CompilationResult result;
    result.success = false;

    QTemporaryFile tempSource;
    tempSource.setFileTemplate(QDir::temp().filePath("octos_XXXXXX.cpp"));
    if (!tempSource.open()) {
        result.errorMessage = "Failed to create temporary file";
        return result;
    }

    tempSource.write(source.toUtf8());
    tempSource.close();

    QString asmFile = tempSource.fileName().replace(".cpp", ".s");

    QStringList args;
    args << "-S" << "-masm=intel" << "-g";

    if (!flags.isEmpty()) {
        args.append(flags.split(" ", Qt::SkipEmptyParts));
    }

    args << tempSource.fileName() << "-o" << asmFile;

    QProcess process;
    process.start(compiler, args);

    if (!process.waitForFinished(30000)) {
        result.errorMessage = "Compilation timeout";
        return result;
    }

    if (process.exitCode() != 0) {
        result.errorMessage = QString::fromUtf8(process.readAllStandardError());
        return result;
    }

    QFile asmOutput(asmFile);
    if (!asmOutput.open(QIODevice::ReadOnly)) {
        result.errorMessage = "Failed to read assembly output";
        return result;
    }

    QString asm_content = QString::fromUtf8(asmOutput.readAll());
    asmOutput.close();

    result.primaryOutput = cleanAssembly(asm_content);
    result.success = true;

    return result;
}

QString CppStrategy::getHighlightingRules() { return "cpp"; }

QStringList CppStrategy::getRecommendedCompilers() const {
    return {"g++", "g++-12", "g++-11", "g++-10", "clang++", "clang++-15", "clang++-14"};
}

QString CppStrategy::cleanAssembly(const QString &assembly) {
    QString cleaned = assembly;

    cleaned.remove(QRegularExpression(R"(^\s*\.cfi_.*$)", QRegularExpression::MultilineOption));

    cleaned.remove(
        QRegularExpression(R"(^\s*\.gcc_except_table.*$)", QRegularExpression::MultilineOption));
    cleaned.remove(
        QRegularExpression(R"(^\s*\.note\.GNU-stack.*$)", QRegularExpression::MultilineOption));

    cleaned.remove(QRegularExpression(R"(^\s*\.section.*$)", QRegularExpression::MultilineOption));

    cleaned.remove(QRegularExpression(R"(^\s*$\n)", QRegularExpression::MultilineOption));

    return cleaned.trimmed();
}

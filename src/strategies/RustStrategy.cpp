# 1 "./src/strategies/RustStrategy.cpp"
#include "RustStrategy.h"

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextStream>

RustStrategy::RustStrategy() {}

CompilationResult RustStrategy::compile(const QString &source, const QString &compiler,
                                        const QString &flags) {
    CompilationResult result;
    result.success = false;

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        result.primaryOutput = "Error: Could not create temporary directory";
        result.errorMessage = "Temporary directory creation failed";
        return result;
    }

    QString projectDir = tempDir.path();
    QString srcDir = projectDir + "/src";

    QDir dir;
    if (!dir.mkpath(srcDir)) {
        result.primaryOutput = "Error: Could not create src directory";
        result.errorMessage = "Failed to create project structure";
        return result;
    }

    QString mainPath = srcDir + "/main.rs";
    QFile mainFile(mainPath);
    if (!mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.primaryOutput = "Error: Could not write main.rs";
        result.errorMessage = "File write failed";
        return result;
    }

    QTextStream out(&mainFile);
    out << source;
    mainFile.close();

    QString cargoPath = projectDir + "/Cargo.toml";
    QFile cargoFile(cargoPath);
    if (!cargoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.primaryOutput = "Error: Could not create Cargo.toml";
        result.errorMessage = "Cargo configuration failed";
        return result;
    }

    QTextStream cargoOut(&cargoFile);
    cargoOut << "[package]\n";
    cargoOut << "name = \"octos_rust\"\n";
    cargoOut << "version = \"0.1.0\"\n";
    cargoOut << "edition = \"2021\"\n";
    cargoOut << "\n[dependencies]\n";
    cargoFile.close();

    QProcess rustcProcess;
    rustcProcess.setWorkingDirectory(projectDir);

    QString asmCmd = QString("rustc --emit asm %1 -o %2/asm.s 2>&1").arg(mainPath, projectDir);

    rustcProcess.start("sh", QStringList() << "-c" << asmCmd);

    if (!rustcProcess.waitForFinished(30000)) {
        result.primaryOutput = "Error: Assembly generation timeout";
        result.errorMessage = "Assembly extraction failed";
        rustcProcess.kill();
        return result;
    }

    QString asmPath = projectDir + "/asm.s";
    QFile asmFile(asmPath);
    if (asmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream asmIn(&asmFile);
        QString assembly = asmIn.readAll();
        asmFile.close();

        QStringList lines = assembly.split('\n');
        QStringList cleanedLines;

        for (const QString &line : lines) {
            if (line.contains(".cfi") || line.contains(".section") || line.contains(".align") ||
                line.contains(".globl") || line.contains(".type") || line.contains(".size") ||
                line.trimmed().isEmpty()) {
                continue;
            }
            cleanedLines.append(line);
        }

        result.primaryOutput = cleanedLines.join('\n');
        result.success = true;
    } else {
        result.primaryOutput = "Error: Could not read assembly output";
        result.errorMessage = "Failed to read generated assembly";
    }

    return result;
}

QStringList RustStrategy::getRecommendedCompilers() const { return {"cargo", "rustc"}; }

void RustStrategy::setDualViewMode(bool enabled) {}

QString RustStrategy::getHighlightingRules() {
    return R"(
        Keyword|fn|let|mut|if|else|match|for|while|loop|return|break|continue|
                pub|const|static|struct|enum|trait|impl|use|crate|mod|unsafe|
                async|await|Box|Vec|Option|Result
        Type|i8|i16|i32|i64|i128|u8|u16|u32|u64|u128|f32|f64|bool|char|String
        String|"(?:[^"\\]|\\.)*"
        Comment|//.*|/\*.*?\*/
    )";
}

QString RustStrategy::getLanguageName() const { return "Rust"; }

QString RustStrategy::getFileExtension() const { return "rs"; }

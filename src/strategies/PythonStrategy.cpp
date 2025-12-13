# 1 "./src/strategies/PythonStrategy.cpp"
#include "PythonStrategy.h"

#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTemporaryFile>

PythonStrategy::PythonStrategy() { createNumbaHelper(); }

CompilationResult PythonStrategy::compile(const QString &source, const QString &compiler,
                                          const QString &flags) {
    CompilationResult result;

    result = compileBytecode(source, compiler);

    if (!result.success) {
        return result;
    }

    if (m_dualViewEnabled) {
        CompilationResult numbaResult = compileNumba(source, compiler);
        if (numbaResult.success) {
            result.secondaryOutput = numbaResult.primaryOutput;
        } else {
            result.secondaryOutput = "(Numba compilation failed: " + numbaResult.errorMessage + ")";
        }
    }

    return result;
}

CompilationResult PythonStrategy::compileBytecode(const QString &source, const QString &compiler) {
    CompilationResult result;
    result.success = false;

    QTemporaryFile tempSource;
    tempSource.setFileTemplate(QDir::temp().filePath("octos_XXXXXX.py"));
    if (!tempSource.open()) {
        result.errorMessage = "Failed to create temporary file";
        return result;
    }

    tempSource.write(source.toUtf8());
    tempSource.close();

    QProcess disassembler;
    disassembler.start("python3", QStringList() << "-m" << "dis" << tempSource.fileName());

    if (!disassembler.waitForFinished(10000)) {
        result.errorMessage = "Disassembly timeout";
        return result;
    }

    if (disassembler.exitCode() != 0) {
        result.errorMessage = QString::fromUtf8(disassembler.readAllStandardError());
        return result;
    }

    result.primaryOutput = QString::fromUtf8(disassembler.readAllStandardOutput());
    result.success = true;

    return result;
}

CompilationResult PythonStrategy::compileNumba(const QString &source, const QString &compiler) {
    CompilationResult result;
    result.success = false;

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        result.errorMessage = "Failed to create temporary directory";
        return result;
    }

    QString userSourcePath = tempDir.path() + "/user_module.py";
    QFile userSourceFile(userSourcePath);
    if (!userSourceFile.open(QIODevice::WriteOnly)) {
        result.errorMessage = "Failed to create user module file";
        return result;
    }
    userSourceFile.write(source.toUtf8());
    userSourceFile.close();

    QString helperPath = tempDir.path() + "/numba_bridge.py";
    QString pythonCode =
        QString(
            "import sys\n"
            "sys.path.insert(0, '%1')\n"
            "\n"
            "try:\n"
            "    import numba\n"
            "    import inspect\n"
            "    import user_module as um\n"
            "    \n"
            "    for name, obj in inspect.getmembers(um):\n"
            "        if inspect.isfunction(obj) and not name.startswith('_'):\n"
            "            try:\n"
            "                jitted = numba.jit(nopython=False)(obj)\n"
            "                print(f'=== Function: {name} ===')\n"
            "                try:\n"
            "                    asm = jitted.inspect_asm()\n"
            "                    if asm:\n"
            "                        print(asm)\n"
            "                    else:\n"
            "                        print('(No native ASM available)')\n"
            "                except:\n"
            "                    print('(inspect_asm not available for this function)')\n"
            "            except Exception as e:\n"
            "                print(f'Error JIT-compiling {name}: {e}')\n"
            "except ImportError:\n"
            "    print('Error: numba not installed. Install with: pip install numba')\n"
            "except Exception as e:\n"
            "    print(f'Error: {e}')\n")
            .arg(tempDir.path());

    QFile helperFile(helperPath);
    if (!helperFile.open(QIODevice::WriteOnly)) {
        result.errorMessage = "Failed to create Numba helper";
        return result;
    }
    helperFile.write(pythonCode.toUtf8());
    helperFile.close();

    QProcess numbaProcess;
    numbaProcess.setWorkingDirectory(tempDir.path());
    numbaProcess.start("python3", QStringList() << helperPath);

    if (!numbaProcess.waitForFinished(30000)) {
        result.errorMessage = "Numba compilation timeout";
        return result;
    }

    QString output = QString::fromUtf8(numbaProcess.readAllStandardOutput());
    QString errorOutput = QString::fromUtf8(numbaProcess.readAllStandardError());

    if (!output.isEmpty()) {
        result.primaryOutput = output;
        result.success = true;
    } else {
        result.errorMessage = errorOutput.isEmpty() ? "No output from Numba" : errorOutput;
    }

    return result;
}

QString PythonStrategy::getHighlightingRules() { return "python"; }

QStringList PythonStrategy::getRecommendedCompilers() const {
    return {"python3", "python3.10", "python3.11", "python3.9"};
}

QString PythonStrategy::extractNumbaCode(const QString &source) {
    QRegularExpression jitRegex(R"(@\s*jit\s*\n\s*def\s+\w+\(.*?\):)");
    QRegularExpressionMatch match = jitRegex.match(source);

    if (match.hasMatch()) {
        return match.captured(0);
    }

    return source;
}

void PythonStrategy::createNumbaHelper() {}

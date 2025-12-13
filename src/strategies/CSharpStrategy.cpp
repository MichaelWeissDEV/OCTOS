# 1 "./src/strategies/CSharpStrategy.cpp"
#include "CSharpStrategy.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

CSharpStrategy::CSharpStrategy() {}

CompilationResult CSharpStrategy::compile(const QString &source, const QString &compiler,
                                          const QString &flags) {
    CompilationResult result;
    result.success = false;

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        result.errorMessage = "Failed to create temporary directory";
        return result;
    }

    QString csprojContent = R"(
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Library</OutputType>
    <TargetFramework>net6.0</TargetFramework>
  </PropertyGroup>
</Project>
)";

    QString csprojPath = tempDir.path() + "/Program.csproj";
    QFile csprojFile(csprojPath);
    if (!csprojFile.open(QIODevice::WriteOnly)) {
        result.errorMessage = "Failed to create .csproj file";
        return result;
    }
    csprojFile.write(csprojContent.toUtf8());
    csprojFile.close();

    QString csPath = tempDir.path() + "/Program.cs";
    QFile sourceFile(csPath);
    if (!sourceFile.open(QIODevice::WriteOnly)) {
        result.errorMessage = "Failed to create C# source file";
        return result;
    }
    sourceFile.write(source.toUtf8());
    sourceFile.close();

    QProcess dotnet;
    QStringList dotnetArgs = {"build", "-c", "Release"};
    if (!flags.isEmpty()) {
        dotnetArgs.append(flags.split(" ", Qt::SkipEmptyParts));
    }

    dotnet.setWorkingDirectory(tempDir.path());
    dotnet.start("dotnet", dotnetArgs);

    if (!dotnet.waitForFinished(60000)) {
        result.errorMessage = "Compilation timeout";
        return result;
    }

    if (dotnet.exitCode() != 0) {
        result.errorMessage = QString::fromUtf8(dotnet.readAllStandardError());
        return result;
    }

    QString dllPath = tempDir.path() + "/bin/Release/net6.0/Program.dll";
    if (!QFile::exists(dllPath)) {
        result.errorMessage = "Compiled DLL not found";
        return result;
    }

    QProcess ildasm;
    ildasm.start("ildasm", QStringList() << dllPath << "/text" << "/out=/dev/stdout");

    if (ildasm.waitForFinished(10000)) {
        result.primaryOutput = QString::fromUtf8(ildasm.readAllStandardOutput());
        if (result.primaryOutput.isEmpty()) {
            result.primaryOutput = "(IL disassembly - ildasm not available on this system)\n\n";
            result.primaryOutput += "Compiled to: " + dllPath;
        }
        result.success = true;
    } else {
        result.primaryOutput = "(ildasm tool not found - IL disassembly unavailable)\n\n";
        result.primaryOutput += "Compiled to: " + dllPath + "\n";
        result.primaryOutput += "Use 'dotnet ildasm' or 'monodis' to view IL code manually.";
        result.success = true;
    }

    return result;
}

QString CSharpStrategy::getHighlightingRules() { return "csharp"; }

QStringList CSharpStrategy::getRecommendedCompilers() const { return {"dotnet", "mono", "csc"}; }

QString CSharpStrategy::extractIL(const QString &asmPath) { return ""; }

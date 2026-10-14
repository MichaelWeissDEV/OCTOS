#include "CompilerRegistry.h"
#include <QRegularExpression>

CompilerRegistry& CompilerRegistry::instance() {
    static CompilerRegistry inst;
    return inst;
}

CompilerRegistry::CompilerRegistry() {
    registerDefaults();
}

void CompilerRegistry::registerDefaults() {
    // GCC C++
    {
        CompilerDefinition gccCpp;
        gccCpp.id = "gcc-latest-cpp";
        gccCpp.displayName = "GCC (Latest) - C++";
        gccCpp.family = CompilerFamily::GCC;
        gccCpp.language = Language::Cpp;
        gccCpp.version = "Latest";
        gccCpp.containerImage = "gcc:latest";
        gccCpp.architecture = "x86_64";
        gccCpp.supportedArtifactTypes = {ArtifactType::Assembly, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        gccCpp.supportedSyntaxFlavors = {SyntaxFlavor::Intel, SyntaxFlavor::ATT};
        gccCpp.availableOptimizationOptions = {"-O0", "-O1", "-O2", "-O3", "-Os", "-Ofast", "-Og"};
        gccCpp.requiredBinaries = {"g++"};
        registerCompiler(gccCpp);
    }
    {
        CompilerDefinition gccC;
        gccC.id = "gcc-latest-c";
        gccC.displayName = "GCC (Latest) - C";
        gccC.family = CompilerFamily::GCC;
        gccC.language = Language::C;
        gccC.version = "Latest";
        gccC.containerImage = "gcc:latest";
        gccC.architecture = "x86_64";
        gccC.supportedArtifactTypes = {ArtifactType::Assembly, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        gccC.supportedSyntaxFlavors = {SyntaxFlavor::Intel, SyntaxFlavor::ATT};
        gccC.availableOptimizationOptions = {"-O0", "-O1", "-O2", "-O3", "-Os", "-Ofast", "-Og"};
        gccC.requiredBinaries = {"gcc"};
        registerCompiler(gccC);
    }
    // Clang C++
    {
        CompilerDefinition clangCpp;
        clangCpp.id = "clang-latest-cpp";
        clangCpp.displayName = "Clang (Latest) - C++";
        clangCpp.family = CompilerFamily::Clang;
        clangCpp.language = Language::Cpp;
        clangCpp.version = "Latest";
        clangCpp.containerImage = "silkeh/clang:latest";
        clangCpp.architecture = "x86_64";
        clangCpp.supportedArtifactTypes = {ArtifactType::Assembly, ArtifactType::LLVM_IR, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        clangCpp.supportedSyntaxFlavors = {SyntaxFlavor::Intel, SyntaxFlavor::ATT};
        clangCpp.availableOptimizationOptions = {"-O0", "-O1", "-O2", "-O3", "-Os", "-Oz", "-Og"};
        clangCpp.requiredBinaries = {"clang++"};
        registerCompiler(clangCpp);
    }
    // Rust
    {
        CompilerDefinition rust;
        rust.id = "rust-latest";
        rust.displayName = "rustc (Latest) - Rust";
        rust.family = CompilerFamily::Rustc;
        rust.language = Language::Rust;
        rust.version = "Latest";
        rust.containerImage = "rust:latest";
        rust.architecture = "x86_64";
        rust.supportedArtifactTypes = {ArtifactType::Assembly, ArtifactType::LLVM_IR, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        rust.supportedSyntaxFlavors = {SyntaxFlavor::Intel, SyntaxFlavor::ATT};
        rust.availableOptimizationOptions = {"-C opt-level=0", "-C opt-level=1", "-C opt-level=2", "-C opt-level=3", "-C opt-level=s", "-C opt-level=z"};
        rust.requiredBinaries = {"rustc"};
        registerCompiler(rust);
    }
    // Java OpenJDK / Temurin
    {
        CompilerDefinition java;
        java.id = "temurin-latest";
        java.displayName = "Eclipse Temurin / OpenJDK - Java";
        java.family = CompilerFamily::OpenJDK;
        java.language = Language::Java;
        java.version = "Latest";
        java.containerImage = "eclipse-temurin:latest";
        java.architecture = "x86_64";
        java.supportedArtifactTypes = {ArtifactType::JVM_Bytecode, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        java.availableOptimizationOptions = {"-g", "-g:none", "-nowarn"};
        java.requiredBinaries = {"javac", "javap"};
        registerCompiler(java);
    }
    // Python
    {
        CompilerDefinition py;
        py.id = "python-latest";
        py.displayName = "CPython 3 (Latest)";
        py.family = CompilerFamily::Python;
        py.language = Language::Python;
        py.version = "3.x";
        py.containerImage = "python:latest";
        py.architecture = "x86_64";
        py.supportedArtifactTypes = {ArtifactType::CPython_Bytecode, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        py.availableOptimizationOptions = {"-O", "-OO"};
        py.requiredBinaries = {"python3"};
        registerCompiler(py);
    }
    // C# (Experimental)
    {
        CompilerDefinition cs;
        cs.id = "mono-latest";
        cs.displayName = "Mono C# (Experimental)";
        cs.family = CompilerFamily::Mono;
        cs.language = Language::CSharp;
        cs.version = "Latest";
        cs.containerImage = "mono:latest";
        cs.architecture = "x86_64";
        cs.supportedArtifactTypes = {ArtifactType::CIL, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        cs.availableOptimizationOptions = {"-optimize+", "-optimize-"};
        cs.requiredBinaries = {"mcs", "monodis"};
        cs.isExperimental = true;
        cs.statusMessage = "CIL disassembly requires mono and monodis in container";
        registerCompiler(cs);
    }
    // Ada GNAT (Experimental)
    {
        CompilerDefinition ada;
        ada.id = "gnat-latest";
        ada.displayName = "GNAT Ada (Experimental)";
        ada.family = CompilerFamily::GNAT;
        ada.language = Language::Ada;
        ada.version = "Latest";
        ada.containerImage = "gnat:latest";
        ada.architecture = "x86_64";
        ada.supportedArtifactTypes = {ArtifactType::Assembly, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        ada.supportedSyntaxFlavors = {SyntaxFlavor::Intel, SyntaxFlavor::ATT};
        ada.availableOptimizationOptions = {"-O0", "-O1", "-O2", "-O3", "-Os"};
        ada.requiredBinaries = {"gnat", "gcc"};
        ada.isExperimental = true;
        ada.statusMessage = "Requires GNAT toolchain container";
        registerCompiler(ada);
    }
}

void CompilerRegistry::registerCompiler(const CompilerDefinition& def) {
    m_compilersById[def.id] = def;
    m_imageToId[def.containerImage] = def.id;
}

QVector<CompilerDefinition> CompilerRegistry::allCompilers() const {
    return m_compilersById.values().toVector();
}

QVector<CompilerDefinition> CompilerRegistry::compilersForLanguage(Language lang) const {
    QVector<CompilerDefinition> list;
    for (const auto& def : m_compilersById) {
        if (def.language == lang) {
            list.append(def);
        }
    }
    return list;
}

bool CompilerRegistry::hasCompilerForImage(const QString& imageName) const {
    return m_imageToId.contains(imageName);
}

CompilerDefinition CompilerRegistry::compilerForImage(const QString& imageName) const {
    if (m_imageToId.contains(imageName)) {
        QString id = m_imageToId.value(imageName);
        return m_compilersById.value(id);
    }
    return inferOrMatchDefinition(imageName);
}

bool CompilerRegistry::isOctosManagedImage(const QString& imageName) const {
    QString img = imageName.toLower();
    // Only genuine compiler images are managed
    return img.startsWith("gcc:") || img.startsWith("silkeh/clang:") ||
           img.startsWith("clang:") || img.startsWith("rust:") ||
           img.startsWith("openjdk:") || img.startsWith("eclipse-temurin:") ||
           img.startsWith("python:") || img.startsWith("mono:") ||
           img.startsWith("gnat:");
}

CompilerDefinition CompilerRegistry::inferOrMatchDefinition(const QString& imageName) const {
    if (m_imageToId.contains(imageName)) {
        return m_compilersById.value(m_imageToId.value(imageName));
    }

    CompilerDefinition def;
    def.containerImage = imageName;
    QString lower = imageName.toLower();

    // Split repo and tag
    QString repo = lower;
    QString tag = "latest";
    int colonIdx = lower.lastIndexOf(':');
    if (colonIdx >= 0) {
        repo = lower.left(colonIdx);
        tag = lower.mid(colonIdx + 1);
    }

    // Strip library/ prefix if present
    if (repo.startsWith("library/")) {
        repo = repo.mid(8);
    }

    if (repo == "gcc") {
        def.id = "gcc-" + tag;
        def.displayName = "GCC " + tag;
        def.family = CompilerFamily::GCC;
        def.language = Language::Cpp;
        def.version = tag;
        def.supportedArtifactTypes = {ArtifactType::Assembly, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        def.supportedSyntaxFlavors = {SyntaxFlavor::Intel, SyntaxFlavor::ATT};
        def.availableOptimizationOptions = {"-O0", "-O1", "-O2", "-O3", "-Os", "-Ofast"};
        def.requiredBinaries = {"gcc", "g++"};
    } else if (repo == "silkeh/clang" || repo == "clang") {
        def.id = "clang-" + tag;
        def.displayName = "Clang " + tag;
        def.family = CompilerFamily::Clang;
        def.language = Language::Cpp;
        def.version = tag;
        def.supportedArtifactTypes = {ArtifactType::Assembly, ArtifactType::LLVM_IR, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        def.supportedSyntaxFlavors = {SyntaxFlavor::Intel, SyntaxFlavor::ATT};
        def.availableOptimizationOptions = {"-O0", "-O1", "-O2", "-O3", "-Os", "-Oz"};
        def.requiredBinaries = {"clang", "clang++"};
    } else if (repo == "rust") {
        def.id = "rust-" + tag;
        def.displayName = "rustc " + tag;
        def.family = CompilerFamily::Rustc;
        def.language = Language::Rust;
        def.version = tag;
        def.supportedArtifactTypes = {ArtifactType::Assembly, ArtifactType::LLVM_IR, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        def.supportedSyntaxFlavors = {SyntaxFlavor::Intel, SyntaxFlavor::ATT};
        def.availableOptimizationOptions = {"-C opt-level=0", "-C opt-level=1", "-C opt-level=2", "-C opt-level=3"};
        def.requiredBinaries = {"rustc"};
    } else if (repo == "eclipse-temurin" || repo == "openjdk") {
        def.id = "java-" + tag;
        def.displayName = "OpenJDK " + tag;
        def.family = CompilerFamily::OpenJDK;
        def.language = Language::Java;
        def.version = tag;
        def.supportedArtifactTypes = {ArtifactType::JVM_Bytecode, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        def.availableOptimizationOptions = {"-g", "-g:none"};
        def.requiredBinaries = {"javac", "javap"};
    } else if (repo == "python") {
        def.id = "python-" + tag;
        def.displayName = "Python " + tag;
        def.family = CompilerFamily::Python;
        def.language = Language::Python;
        def.version = tag;
        def.supportedArtifactTypes = {ArtifactType::CPython_Bytecode, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        def.availableOptimizationOptions = {"-O", "-OO"};
        def.requiredBinaries = {"python3"};
    } else if (repo == "mono") {
        def.id = "mono-" + tag;
        def.displayName = "Mono " + tag;
        def.family = CompilerFamily::Mono;
        def.language = Language::CSharp;
        def.version = tag;
        def.supportedArtifactTypes = {ArtifactType::CIL, ArtifactType::RawOutput, ArtifactType::Diagnostics};
        def.availableOptimizationOptions = {"-optimize+"};
        def.requiredBinaries = {"mcs", "monodis"};
        def.isExperimental = true;
    } else {
        def.id = "generic-" + imageName;
        def.displayName = imageName;
        def.family = CompilerFamily::Generic;
        def.language = Language::Cpp;
        def.version = tag;
        def.isExperimental = true;
        def.statusMessage = "Unrecognized toolchain; may not be compatible";
    }

    return def;
}

bool CompilerRegistry::isImageCompatibleWithLanguage(const QString& imageName, Language lang) const {
    CompilerDefinition def = compilerForImage(imageName);

    switch (lang) {
        case Language::Cpp:
        case Language::C:
            return def.family == CompilerFamily::GCC || def.family == CompilerFamily::Clang;
        case Language::Rust:
            return def.family == CompilerFamily::Rustc;
        case Language::Java:
            return def.family == CompilerFamily::OpenJDK;
        case Language::Python:
            return def.family == CompilerFamily::Python;
        case Language::CSharp:
            return def.family == CompilerFamily::Mono;
        case Language::Ada:
            return def.family == CompilerFamily::GNAT || def.family == CompilerFamily::GCC;
    }
    return false;
}

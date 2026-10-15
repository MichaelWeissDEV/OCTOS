#ifndef COMPILER_ENUMS_H
#define COMPILER_ENUMS_H

#include <QMap>
#include <QString>

// Compiler output types - shared between UI and backend
enum class SyntaxFlavor {
    Intel,
    ATT
};

enum class Language {
    Cpp,
    C,
    Rust,
    CSharp,
    Java,
    Python,
    Ada
};

enum class CompilerCategory {
    Cpp,
    C,
    Clang,
    Java,
    Python,
    Rust,
    CSharp,
    Ada
};

// Rust crate type. Auto picks Binary if the source defines `fn main`, otherwise Library, so
// snippets consisting only of functions compile to assembly without a main().
enum class RustCrateType {
    Auto,
    Binary,
    Library
};

// What a compilation should produce. LlvmIr is available for Clang and Rust.
enum class OutputKind {
    Assembly,
    LlvmIr
};

inline QString outputKindToString(OutputKind k) { return k == OutputKind::LlvmIr ? "llvm-ir" : "assembly"; }
inline OutputKind stringToOutputKind(const QString& s) {
    const QString l = s.toLower();
    return (l == "llvm-ir" || l == "llvm_ir" || l == "ir") ? OutputKind::LlvmIr : OutputKind::Assembly;
}

enum class JobState {
    Queued,
    Starting,
    Running,
    Completed,
    Failed,
    Cancelled,
    TimedOut
};

inline QString languageToString(Language lang) {
    switch (lang) {
        case Language::Cpp:    return "cpp";
        case Language::C:      return "c";
        case Language::Rust:   return "rust";
        case Language::CSharp: return "csharp";
        case Language::Java:   return "java";
        case Language::Python: return "python";
        case Language::Ada:    return "ada";
    }
    return "cpp";
}

inline Language stringToLanguage(const QString& str) {
    QString lower = str.toLower().trimmed();
    if (lower == "c") return Language::C;
    if (lower == "rust" || lower == "rs") return Language::Rust;
    if (lower == "csharp" || lower == "cs" || lower == "c#") return Language::CSharp;
    if (lower == "java") return Language::Java;
    if (lower == "python" || lower == "py") return Language::Python;
    if (lower == "ada" || lower == "adb") return Language::Ada;
    return Language::Cpp;
}

inline QString syntaxFlavorToString(SyntaxFlavor flavor) {
    switch (flavor) {
        case SyntaxFlavor::Intel: return "Intel";
        case SyntaxFlavor::ATT:   return "AT&T";
    }
    return "Intel";
}

inline QString jobStateToString(JobState state) {
    switch (state) {
        case JobState::Queued:    return "Queued";
        case JobState::Starting:  return "Starting";
        case JobState::Running:   return "Running";
        case JobState::Completed: return "Completed";
        case JobState::Failed:    return "Failed";
        case JobState::Cancelled: return "Cancelled";
        case JobState::TimedOut:  return "TimedOut";
    }
    return "Unknown";
}

// Legacy compatibility type
struct CompileResult {
    bool success{false};
    QString output;
    QMap<int, int> lineMapping;
};

#endif // COMPILER_ENUMS_H

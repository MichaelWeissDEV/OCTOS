#ifndef COMPILER_ENUMS_H
#define COMPILER_ENUMS_H

#include <QMap>
#include <QString>

// Compiler output types - shared between UI and backend
enum class SyntaxFlavor { Intel, ATT };
enum class Language { Cpp, C, Rust, CSharp, Java, Python, Ada };

// Legacy compatibility type
struct CompileResult {
    bool success;
    QString output;
    QMap<int, int> lineMapping;
};

#endif

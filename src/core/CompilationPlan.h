#ifndef COMPILATIONPLAN_H
#define COMPILATIONPLAN_H

#include <QString>
#include <QStringList>

// Output types that OCTOS can produce
enum class CompilerOutputType {
    NativeAssembly,  // C, C++, Rust - actual assembly code
    JVMBytecode,     // Java - JVM bytecode via javap
    ILCode,          // C# - Intermediate Language
    SyntaxValidation, // Python - syntax check only
    Unknown
};

// Structure representing a complete compilation plan
struct CompilationPlan {
    QString image;              // Docker image to use
    QString containerExecutable; // Executable to run inside container
    QStringList arguments;     // Arguments for the container executable
    QString inputFile;         // Input source file path in container
    QString outputFile;        // Output file path in container
    CompilerOutputType outputType; // What type of output to expect
    bool requiresOutputFile;   // Whether to expect a separate output file
    QStringList dockerOptions;  // Additional docker run options
    bool useNetworkIsolation;  // Whether to use --network none
    
    // Validation method
    bool isValid() const {
        return !image.isEmpty() && !containerExecutable.isEmpty();
    }
};

#endif // COMPILATIONPLAN_H

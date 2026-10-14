#ifndef COMPILATIONARTIFACT_H
#define COMPILATIONARTIFACT_H

#include <QMap>
#include <QMetaType>
#include <QString>

enum class ArtifactType {
    Assembly,
    LLVM_IR,
    JVM_Bytecode,
    CPython_Bytecode,
    CIL,
    Diagnostics,
    CompilerMetadata,
    RawOutput,
    Other
};

inline QString artifactTypeToString(ArtifactType type) {
    switch (type) {
        case ArtifactType::Assembly: return "Assembly";
        case ArtifactType::LLVM_IR: return "LLVM IR";
        case ArtifactType::JVM_Bytecode: return "JVM Bytecode";
        case ArtifactType::CPython_Bytecode: return "CPython Bytecode";
        case ArtifactType::CIL: return "CIL";
        case ArtifactType::Diagnostics: return "Diagnostics";
        case ArtifactType::CompilerMetadata: return "Compiler Metadata";
        case ArtifactType::RawOutput: return "Raw Output";
        case ArtifactType::Other: return "Other";
    }
    return "Unknown";
}

struct CompilationArtifact {
    ArtifactType type{ArtifactType::Other};
    QString name;
    QString content;
    qint64 sizeBytes{0};
    QString mimeType{"text/plain"};
    QString originPath;
    QMap<QString, QString> metadata;
};

Q_DECLARE_METATYPE(ArtifactType);
Q_DECLARE_METATYPE(CompilationArtifact);

#endif // COMPILATIONARTIFACT_H

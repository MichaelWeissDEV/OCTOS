#ifndef COMPILATIONARTIFACT_H
#define COMPILATIONARTIFACT_H

#include <QCryptographicHash>
#include <QDateTime>
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

// Where the bytes of an artifact live. A host temp path is only valid while the
// compilation sandbox exists and must never be offered as a usable file path.
enum class ArtifactStorage {
    InMemory,        // `content` holds the data (default; always valid)
    PersistentFile   // `persistentPath` holds a durable copy
};

struct CompilationArtifact {
    QString id;                       // stable within a result: "<compilationId>:<index>:<type>"
    ArtifactType type{ArtifactType::Other};
    QString name;                     // display name
    QString content;
    qint64 sizeBytes{0};
    QString mimeType{"text/plain"};
    QMap<QString, QString> metadata;

    // Provenance
    QString source;                   // e.g. "g++ -S output file" or "compiler stderr"
    QString compilerProfile;          // profile / flags it was produced with
    quint64 compilationId{0};         // job id of the producing compilation
    QDateTime createdAt;
    QString checksumSha256;           // hex digest of the UTF-8 content, optional
    bool truncated{false};            // content is incomplete

    // Lifecycle: these paths are distinct on purpose.
    QString containerPath;            // path inside the container (informational only)
    QString hostTempPath;             // transient host path; invalid once the sandbox is deleted
    QString persistentPath;           // durable copy, empty unless explicitly saved
    ArtifactStorage storage{ArtifactStorage::InMemory};

    bool hasPersistentFile() const {
        return storage == ArtifactStorage::PersistentFile && !persistentPath.isEmpty();
    }

    // Writes the content to `path` atomically and records it as the durable copy. Only an explicit
    // save ever sets persistentPath; the sandbox directory the compiler used no longer exists.
    bool saveTo(const QString& path, QString* errorMsg = nullptr);

    void computeChecksum() {
        checksumSha256 = QString::fromLatin1(
            QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Sha256).toHex());
    }
};

Q_DECLARE_METATYPE(ArtifactType);
Q_DECLARE_METATYPE(CompilationArtifact);

#endif // COMPILATIONARTIFACT_H

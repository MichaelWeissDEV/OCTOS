#ifndef WORKSPACEMODEL_H
#define WORKSPACEMODEL_H

#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include "CompilerEnums.h"
#include "CompilationArtifact.h"
#include "CompilationRequest.h"

struct SourceFile {
    QString path;       // relative to the project root, '/'-separated, never absolute, never contains ".."
    QString content;
    bool isHeader{false};
    Language language{Language::Cpp};
};

// A reproducible way of compiling: everything needed to rerun it without manual configuration.
struct CompilerProfile {
    QString id;
    QString name;
    QString compilerImage;
    QString compilerVersion;   // informational (e.g. "13"); the image tag is authoritative
    Language language{Language::Cpp};
    QString optimization;      // e.g. "-O3" or "-C opt-level=3"; empty = compiler default
    QString userFlags;
    QString targetArchitecture; // informational, e.g. "x86_64"
    QString outputFormat{"assembly"}; // "assembly", "llvm-ir", "bytecode"
    RustCrateType rustCrateType{RustCrateType::Auto};
    SyntaxFlavor syntaxFlavor{SyntaxFlavor::Intel};

    // Flags exactly as handed to the compiler (optimization first, then free-form flags).
    QString effectiveFlags() const {
        return (optimization.trimmed() + " " + userFlags.trimmed()).trimmed();
    }
};

struct BuildTarget {
    QString name;
    QString mainSourceFile;
    QStringList sourceFiles;
    QStringList includePaths;   // relative to the project root
};

struct CompilationConfiguration {
    QString activeProfileId;
    QString activeTargetName;
    QMap<QString, QString> environmentVars;
};

// JSON (de)serialisation of a single profile, shared by workspaces and the user-level ProfileStore.
class ProfileJson {
public:
    static QJsonObject toObject(const CompilerProfile& p);
    // Lenient about missing optional fields (default + warning), strict about id/name/image types.
    static bool fromObject(const QJsonObject& o, CompilerProfile* out, QStringList* warnings);
};

class Workspace {
public:
    // Format identification. Files with a NEWER version are rejected, older ones are migrated.
    static constexpr int kCurrentFormatVersion = 1;
    static QString formatName(int version) { return QString("octos-project-v%1").arg(version); }
    // Parses "octos-project-vN"; returns -1 if the string has another shape.
    static int parseFormatVersion(const QString& s);

    // Hard limits for loading/saving (protect against hostile or corrupted project files)
    static constexpr qint64 kMaxJsonBytes = 256LL * 1024 * 1024;
    static constexpr int kMaxFiles = 10000;
    static constexpr qint64 kMaxFileBytes = 16LL * 1024 * 1024;
    static constexpr qint64 kMaxTotalContentBytes = 128LL * 1024 * 1024;

    Workspace(const QString& name = "Untitled Workspace");

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString projectFormatVersion() const { return m_projectFormatVersion; }

    const QVector<SourceFile>& sourceFiles() const { return m_sourceFiles; }
    void addSourceFile(const SourceFile& file);
    bool removeSourceFile(const QString& path);
    SourceFile findSourceFile(const QString& path) const;
    bool hasSourceFile(const QString& path) const;

    const QVector<CompilerProfile>& profiles() const { return m_profiles; }
    void addProfile(const CompilerProfile& profile);
    bool removeProfile(const QString& id);
    const CompilerProfile* findProfile(const QString& id) const;

    const QVector<BuildTarget>& targets() const { return m_targets; }
    void addTarget(const BuildTarget& target);
    const BuildTarget* findTarget(const QString& name) const;

    CompilationConfiguration configuration() const { return m_configuration; }
    void setConfiguration(const CompilationConfiguration& config) { m_configuration = config; }

    // Non-fatal findings produced while loading (e.g. missing optional fields, unknown language)
    QStringList warnings() const { return m_warnings; }

    // Checks structural integrity: unique ids, references, safe paths, limits. Empty = valid.
    QStringList validate() const;

    // --- paths -------------------------------------------------------------------------------
    // Normalises "a//b/./c.cpp" to "a/b/c.cpp". Returns an empty string and sets *ok=false for
    // absolute paths, drive letters, "~", NUL, empty segments that escape, or any ".." escape.
    static QString normalizeRelativePath(const QString& path, bool* ok = nullptr);
    static bool isSafeRelativePath(const QString& path) {
        bool ok = false;
        const QString n = normalizeRelativePath(path, &ok);
        return ok && n == path;
    }
    // Absolute location of `relativePath` below `rootDir`, verified to stay inside it (also
    // against symlinks). Returns an empty string and an error if it does not.
    static QString resolveInside(const QString& rootDir, const QString& relativePath, QString* errorMsg = nullptr);

    // Writes all source files below `rootDir` (e.g. to hand a project to a tool). Every path is
    // checked by resolveInside(); existing symlinks are never followed out of the root.
    bool materializeTo(const QString& rootDir, QString* errorMsg = nullptr) const;

    // --- request building --------------------------------------------------------------------
    // Builds a compile request for `target` (or the active/first one) with `profile` (or the active
    // one). Header and additional files of the target travel along as extra files.
    bool buildRequest(const QString& profileId, const QString& targetName,
                      CompilationRequest* out, QString* errorMsg = nullptr) const;

    // --- persistence --------------------------------------------------------------------------
    QByteArray toJson() const;
    static Workspace fromJson(const QByteArray& json, QString* errorMsg = nullptr);

    // Atomic: written through QSaveFile, an invalid workspace is never written, and a failure
    // leaves the previous file untouched.
    bool saveToFile(const QString& filePath, QString* errorMsg = nullptr) const;
    static Workspace loadFromFile(const QString& filePath, QString* errorMsg = nullptr);

    // Directory of the loaded/saved project file ("" if never saved); relative paths resolve against it.
    QString projectDirectory() const { return m_projectDirectory; }

private:
    QString m_name;
    QString m_projectFormatVersion{formatName(kCurrentFormatVersion)};
    QVector<SourceFile> m_sourceFiles;
    QVector<CompilerProfile> m_profiles;
    QVector<BuildTarget> m_targets;
    CompilationConfiguration m_configuration;
    QStringList m_warnings;
    QString m_projectDirectory;
};

#endif // WORKSPACEMODEL_H

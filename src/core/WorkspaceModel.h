#ifndef WORKSPACEMODEL_H
#define WORKSPACEMODEL_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include "CompilerEnums.h"
#include "CompilationArtifact.h"

struct SourceFile {
    QString path;
    QString content;
    bool isHeader{false};
    Language language{Language::Cpp};
};

struct CompilerProfile {
    QString id;
    QString name;
    QString compilerImage;
    QString userFlags;
    SyntaxFlavor syntaxFlavor{SyntaxFlavor::Intel};
};

struct BuildTarget {
    QString name;
    QString mainSourceFile;
    QStringList sourceFiles;
    QStringList includePaths;
};

struct CompilationConfiguration {
    QString activeProfileId;
    QString activeTargetName;
    QMap<QString, QString> environmentVars;
};

class Workspace {
public:
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

    const QVector<BuildTarget>& targets() const { return m_targets; }
    void addTarget(const BuildTarget& target);

    CompilationConfiguration configuration() const { return m_configuration; }
    void setConfiguration(const CompilationConfiguration& config) { m_configuration = config; }

    QByteArray toJson() const;
    static Workspace fromJson(const QByteArray& json, QString* errorMsg = nullptr);

    bool saveToFile(const QString& filePath, QString* errorMsg = nullptr) const;
    static Workspace loadFromFile(const QString& filePath, QString* errorMsg = nullptr);

private:
    QString m_name;
    QString m_projectFormatVersion{"octos-project-v1"};
    QVector<SourceFile> m_sourceFiles;
    QVector<CompilerProfile> m_profiles;
    QVector<BuildTarget> m_targets;
    CompilationConfiguration m_configuration;
};

#endif // WORKSPACEMODEL_H

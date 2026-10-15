#ifndef COMPILERREGISTRY_H
#define COMPILERREGISTRY_H

#include <QMap>
#include <QString>
#include <QVector>
#include "CompilerDefinition.h"

class CompilerRegistry {
public:
    static CompilerRegistry& instance();

    void registerCompiler(const CompilerDefinition& def);
    QVector<CompilerDefinition> allCompilers() const;
    QVector<CompilerDefinition> compilersForLanguage(Language lang) const;

    CompilerDefinition compilerForImage(const QString& imageName) const;
    bool hasCompilerForImage(const QString& imageName) const;

    bool isImageCompatibleWithLanguage(const QString& imageName, Language lang) const;
    // True if the NAME looks like a compiler image OCTOS knows how to use (gcc:*, rust:*, ...).
    // This says nothing about who pulled the image; ownership is tracked by DockerCompilerManager.
    bool isKnownCompilerImageName(const QString& imageName) const;
    bool isOctosManagedImage(const QString& imageName) const { return isKnownCompilerImageName(imageName); }

    // Probe results and the currently installed image IDs (fed by DockerCompilerManager).
    void recordProbeResult(const QString& imageName, const ToolchainProbeResult& result);
    ToolchainProbeResult probeResultForImage(const QString& imageName) const;
    void setInstalledImageId(const QString& imageName, const QString& imageId);
    QJsonObject exportProbes() const;
    void importProbes(const QJsonObject& obj);

    // Discover or construct metadata for an installed image based on verified heuristics
    CompilerDefinition inferOrMatchDefinition(const QString& imageName) const;

private:
    CompilerDefinition applyRuntimeState(CompilerDefinition def) const;

public:

private:
    CompilerRegistry();
    ~CompilerRegistry() = default;
    CompilerRegistry(const CompilerRegistry&) = delete;
    CompilerRegistry& operator=(const CompilerRegistry&) = delete;

    void registerDefaults();

    QMap<QString, CompilerDefinition> m_compilersById;
    QMap<QString, QString> m_imageToId;
    QMap<QString, ToolchainProbeResult> m_probes;
    QMap<QString, QString> m_installedIds;
};

#endif // COMPILERREGISTRY_H

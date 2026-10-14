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
    bool isOctosManagedImage(const QString& imageName) const;

    // Discover or construct metadata for an installed image based on verified heuristics
    CompilerDefinition inferOrMatchDefinition(const QString& imageName) const;

private:
    CompilerRegistry();
    ~CompilerRegistry() = default;
    CompilerRegistry(const CompilerRegistry&) = delete;
    CompilerRegistry& operator=(const CompilerRegistry&) = delete;

    void registerDefaults();

    QMap<QString, CompilerDefinition> m_compilersById;
    QMap<QString, QString> m_imageToId;
};

#endif // COMPILERREGISTRY_H

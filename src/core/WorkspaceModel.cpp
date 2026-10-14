#include "WorkspaceModel.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

Workspace::Workspace(const QString& name)
    : m_name(name)
{
}

void Workspace::addSourceFile(const SourceFile& file)
{
    for (int i = 0; i < m_sourceFiles.size(); ++i) {
        if (m_sourceFiles[i].path == file.path) {
            m_sourceFiles[i] = file;
            return;
        }
    }
    m_sourceFiles.append(file);
}

bool Workspace::removeSourceFile(const QString& path)
{
    for (int i = 0; i < m_sourceFiles.size(); ++i) {
        if (m_sourceFiles[i].path == path) {
            m_sourceFiles.removeAt(i);
            return true;
        }
    }
    return false;
}

SourceFile Workspace::findSourceFile(const QString& path) const
{
    for (const auto& file : m_sourceFiles) {
        if (file.path == path) {
            return file;
        }
    }
    return SourceFile{};
}

bool Workspace::hasSourceFile(const QString& path) const
{
    for (const auto& file : m_sourceFiles) {
        if (file.path == path) {
            return true;
        }
    }
    return false;
}

void Workspace::addProfile(const CompilerProfile& profile)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == profile.id) {
            m_profiles[i] = profile;
            return;
        }
    }
    m_profiles.append(profile);
}

void Workspace::addTarget(const BuildTarget& target)
{
    for (int i = 0; i < m_targets.size(); ++i) {
        if (m_targets[i].name == target.name) {
            m_targets[i] = target;
            return;
        }
    }
    m_targets.append(target);
}

QByteArray Workspace::toJson() const
{
    QJsonObject root;
    root["format_version"] = m_projectFormatVersion;
    root["name"] = m_name;

    // Files
    QJsonArray filesArray;
    for (const auto& file : m_sourceFiles) {
        QJsonObject fileObj;
        fileObj["path"] = file.path;
        fileObj["content"] = file.content;
        fileObj["is_header"] = file.isHeader;
        fileObj["language"] = languageToString(file.language);
        filesArray.append(fileObj);
    }
    root["source_files"] = filesArray;

    // Profiles
    QJsonArray profilesArray;
    for (const auto& prof : m_profiles) {
        QJsonObject profObj;
        profObj["id"] = prof.id;
        profObj["name"] = prof.name;
        profObj["compiler_image"] = prof.compilerImage;
        profObj["user_flags"] = prof.userFlags;
        profObj["syntax_flavor"] = (prof.syntaxFlavor == SyntaxFlavor::Intel) ? "Intel" : "ATT";
        profilesArray.append(profObj);
    }
    root["profiles"] = profilesArray;

    // Targets
    QJsonArray targetsArray;
    for (const auto& target : m_targets) {
        QJsonObject tgtObj;
        tgtObj["name"] = target.name;
        tgtObj["main_source_file"] = target.mainSourceFile;
        tgtObj["source_files"] = QJsonArray::fromStringList(target.sourceFiles);
        tgtObj["include_paths"] = QJsonArray::fromStringList(target.includePaths);
        targetsArray.append(tgtObj);
    }
    root["targets"] = targetsArray;

    // Configuration
    QJsonObject configObj;
    configObj["active_profile_id"] = m_configuration.activeProfileId;
    configObj["active_target_name"] = m_configuration.activeTargetName;
    QJsonObject envObj;
    for (auto it = m_configuration.environmentVars.constBegin(); it != m_configuration.environmentVars.constEnd(); ++it) {
        envObj[it.key()] = it.value();
    }
    configObj["environment_vars"] = envObj;
    root["configuration"] = configObj;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented);
}

Workspace Workspace::fromJson(const QByteArray& json, QString* errorMsg)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMsg) {
            *errorMsg = QString("JSON parse error: %1 at offset %2").arg(parseError.errorString()).arg(parseError.offset);
        }
        return Workspace{};
    }

    if (!doc.isObject()) {
        if (errorMsg) {
            *errorMsg = "Root JSON element must be an object";
        }
        return Workspace{};
    }

    QJsonObject root = doc.object();
    QString formatVer = root["format_version"].toString();
    if (formatVer != "octos-project-v1" && !formatVer.isEmpty()) {
        // Warning or unsupported format
    }

    Workspace ws(root["name"].toString("Untitled Workspace"));
    ws.m_projectFormatVersion = formatVer.isEmpty() ? "octos-project-v1" : formatVer;

    // Files
    QJsonArray filesArray = root["source_files"].toArray();
    for (const auto& val : filesArray) {
        QJsonObject fileObj = val.toObject();
        SourceFile sf;
        sf.path = fileObj["path"].toString();
        sf.content = fileObj["content"].toString();
        sf.isHeader = fileObj["is_header"].toBool(false);
        sf.language = stringToLanguage(fileObj["language"].toString("cpp"));
        ws.addSourceFile(sf);
    }

    // Profiles
    QJsonArray profilesArray = root["profiles"].toArray();
    for (const auto& val : profilesArray) {
        QJsonObject profObj = val.toObject();
        CompilerProfile prof;
        prof.id = profObj["id"].toString();
        prof.name = profObj["name"].toString();
        prof.compilerImage = profObj["compiler_image"].toString();
        prof.userFlags = profObj["user_flags"].toString();
        prof.syntaxFlavor = (profObj["syntax_flavor"].toString() == "ATT") ? SyntaxFlavor::ATT : SyntaxFlavor::Intel;
        ws.addProfile(prof);
    }

    // Targets
    QJsonArray targetsArray = root["targets"].toArray();
    for (const auto& val : targetsArray) {
        QJsonObject tgtObj = val.toObject();
        BuildTarget tgt;
        tgt.name = tgtObj["name"].toString();
        tgt.mainSourceFile = tgtObj["main_source_file"].toString();
        for (const auto& s : tgtObj["source_files"].toArray()) {
            tgt.sourceFiles.append(s.toString());
        }
        for (const auto& inc : tgtObj["include_paths"].toArray()) {
            tgt.includePaths.append(inc.toString());
        }
        ws.addTarget(tgt);
    }

    // Configuration
    if (root.contains("configuration")) {
        QJsonObject configObj = root["configuration"].toObject();
        CompilationConfiguration config;
        config.activeProfileId = configObj["active_profile_id"].toString();
        config.activeTargetName = configObj["active_target_name"].toString();
        QJsonObject envObj = configObj["environment_vars"].toObject();
        for (auto it = envObj.constBegin(); it != envObj.constEnd(); ++it) {
            config.environmentVars[it.key()] = it.value().toString();
        }
        ws.setConfiguration(config);
    }

    return ws;
}

bool Workspace::saveToFile(const QString& filePath, QString* errorMsg) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMsg) {
            *errorMsg = QString("Failed to open file for writing: %1 (%2)").arg(filePath, file.errorString());
        }
        return false;
    }

    QByteArray data = toJson();
    if (file.write(data) != data.size()) {
        if (errorMsg) {
            *errorMsg = QString("Failed to write complete data to %1").arg(filePath);
        }
        return false;
    }

    return true;
}

Workspace Workspace::loadFromFile(const QString& filePath, QString* errorMsg)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMsg) {
            *errorMsg = QString("Failed to open file for reading: %1 (%2)").arg(filePath, file.errorString());
        }
        return Workspace{};
    }

    QByteArray data = file.readAll();
    return fromJson(data, errorMsg);
}

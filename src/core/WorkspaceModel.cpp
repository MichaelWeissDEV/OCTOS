#include "WorkspaceModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>

namespace {

QString syntaxToString(SyntaxFlavor f) { return f == SyntaxFlavor::Intel ? "Intel" : "ATT"; }

QString crateTypeToString(RustCrateType t) {
    switch (t) {
        case RustCrateType::Auto: return "auto";
        case RustCrateType::Binary: return "bin";
        case RustCrateType::Library: return "lib";
    }
    return "auto";
}

RustCrateType crateTypeFromString(const QString& s) {
    if (s == "bin") return RustCrateType::Binary;
    if (s == "lib") return RustCrateType::Library;
    return RustCrateType::Auto;
}

bool isKnownLanguage(const QString& s) {
    static const QSet<QString> known = {"cpp", "c", "rust", "csharp", "java", "python", "ada"};
    return known.contains(s.toLower().trimmed());
}

}

// ---------------------------------------------------------------------------------------------
// ProfileJson
// ---------------------------------------------------------------------------------------------

QJsonObject ProfileJson::toObject(const CompilerProfile& p) {
    QJsonObject o;
    o["id"] = p.id;
    o["name"] = p.name;
    o["compiler_image"] = p.compilerImage;
    o["compiler_version"] = p.compilerVersion;
    o["language"] = languageToString(p.language);
    o["optimization"] = p.optimization;
    o["user_flags"] = p.userFlags;
    o["target_architecture"] = p.targetArchitecture;
    o["output_format"] = p.outputFormat;
    o["rust_crate_type"] = crateTypeToString(p.rustCrateType);
    o["syntax_flavor"] = syntaxToString(p.syntaxFlavor);
    return o;
}

bool ProfileJson::fromObject(const QJsonObject& o, CompilerProfile* out, QStringList* warnings) {
    auto strField = [&](const char* key, QString* dst, bool required) -> bool {
        const QJsonValue v = o.value(key);
        if (v.isUndefined() || v.isNull()) {
            if (required) return false;
            return true;  // optional: keep default
        }
        if (!v.isString()) {
            if (warnings) warnings->append(QString("profile field '%1' is not a string and was ignored").arg(key));
            return !required;
        }
        *dst = v.toString();
        return true;
    };
    CompilerProfile p;
    if (!strField("id", &p.id, true) || p.id.isEmpty()) return false;
    if (!strField("name", &p.name, true)) return false;
    strField("compiler_image", &p.compilerImage, false);
    strField("compiler_version", &p.compilerVersion, false);
    QString lang;
    if (o.contains("language")) {
        strField("language", &lang, false);
        if (!lang.isEmpty() && !isKnownLanguage(lang) && warnings)
            warnings->append(QString("profile '%1': unknown language '%2', using cpp").arg(p.id, lang));
        p.language = stringToLanguage(lang.isEmpty() ? "cpp" : lang);
    }
    strField("optimization", &p.optimization, false);
    strField("user_flags", &p.userFlags, false);
    strField("target_architecture", &p.targetArchitecture, false);
    QString fmt;
    if (strField("output_format", &fmt, false) && !fmt.isEmpty()) p.outputFormat = fmt;
    QString crate;
    strField("rust_crate_type", &crate, false);
    p.rustCrateType = crateTypeFromString(crate);
    QString syn;
    strField("syntax_flavor", &syn, false);
    p.syntaxFlavor = (syn == "ATT") ? SyntaxFlavor::ATT : SyntaxFlavor::Intel;
    *out = p;
    return true;
}

// ---------------------------------------------------------------------------------------------
// Workspace
// ---------------------------------------------------------------------------------------------

Workspace::Workspace(const QString& name)
    : m_name(name)
{
}

int Workspace::parseFormatVersion(const QString& s)
{
    static const QRegularExpression re(R"(^octos-project-v(\d{1,6})$)");
    const auto m = re.match(s);
    return m.hasMatch() ? m.captured(1).toInt() : -1;
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
        if (file.path == path) return file;
    }
    return SourceFile{};
}

bool Workspace::hasSourceFile(const QString& path) const
{
    for (const auto& file : m_sourceFiles) {
        if (file.path == path) return true;
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

bool Workspace::removeProfile(const QString& id)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == id) {
            m_profiles.removeAt(i);
            if (m_configuration.activeProfileId == id) m_configuration.activeProfileId.clear();
            return true;
        }
    }
    return false;
}

const CompilerProfile* Workspace::findProfile(const QString& id) const
{
    for (const auto& p : m_profiles) if (p.id == id) return &p;
    return nullptr;
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

const BuildTarget* Workspace::findTarget(const QString& name) const
{
    for (const auto& t : m_targets) if (t.name == name) return &t;
    return nullptr;
}

// --- paths --------------------------------------------------------------------------------------

QString Workspace::normalizeRelativePath(const QString& path, bool* ok)
{
    auto fail = [&]() { if (ok) *ok = false; return QString(); };
    if (path.isEmpty() || path.contains(QChar('\0')) || path.size() > 1024) return fail();
    if (path.contains('\\')) return fail();                    // one separator style only
    for (const QChar c : path) if (c.unicode() < 0x20 || c.unicode() == 0x7f) return fail();  // control characters
    if (path.startsWith('/') || path.startsWith('~')) return fail();
    static const QRegularExpression drive(R"(^[A-Za-z]:)");
    if (drive.match(path).hasMatch()) return fail();

    QStringList out;
    for (const QString& seg : path.split('/')) {
        if (seg.isEmpty() || seg == ".") continue;
        if (seg == "..") {
            if (out.isEmpty()) return fail();   // would escape the project root
            out.removeLast();
            continue;
        }
        out.append(seg);
    }
    if (out.isEmpty()) return fail();
    if (ok) *ok = true;
    return out.join('/');
}

QString Workspace::resolveInside(const QString& rootDir, const QString& relativePath, QString* errorMsg)
{
    auto fail = [&](const QString& m) { if (errorMsg) *errorMsg = m; return QString(); };
    bool ok = false;
    const QString norm = normalizeRelativePath(relativePath, &ok);
    if (!ok) return fail(QString("Unsafe path in project: '%1'").arg(relativePath));

    const QDir root(rootDir);
    const QString rootCanon = QFileInfo(rootDir).canonicalFilePath();
    if (rootCanon.isEmpty()) return fail("Project root does not exist: " + rootDir);

    // Walk down segment by segment; existing components must not be symlinks leaving the root.
    QString current = rootCanon;
    const QStringList segs = norm.split('/');
    for (int i = 0; i < segs.size(); ++i) {
        const QString next = current + "/" + segs[i];
        const QFileInfo fi(next);
        if (fi.exists() || fi.isSymLink()) {
            const QString canon = fi.canonicalFilePath();
            if (fi.isSymLink() || canon.isEmpty() || !(canon + "/").startsWith(rootCanon + "/"))
                return fail(QString("Path escapes the project directory: '%1'").arg(relativePath));
            current = canon;
        } else {
            current = next;   // not created yet: still a plain child of a verified directory
        }
    }
    return current;
}

bool Workspace::materializeTo(const QString& rootDir, QString* errorMsg) const
{
    const QStringList problems = validate();
    if (!problems.isEmpty()) {
        if (errorMsg) *errorMsg = "Workspace is invalid: " + problems.first();
        return false;
    }
    if (!QDir().mkpath(rootDir)) {
        if (errorMsg) *errorMsg = "Cannot create directory: " + rootDir;
        return false;
    }
    for (const SourceFile& sf : m_sourceFiles) {
        QString err;
        const QString abs = resolveInside(rootDir, sf.path, &err);
        if (abs.isEmpty()) { if (errorMsg) *errorMsg = err; return false; }
        if (!QDir().mkpath(QFileInfo(abs).absolutePath())) {
            if (errorMsg) *errorMsg = "Cannot create directory for " + sf.path;
            return false;
        }
        // Re-verify after creating parent directories (a race or pre-existing symlink would show here).
        err.clear();
        if (resolveInside(rootDir, sf.path, &err).isEmpty()) { if (errorMsg) *errorMsg = err; return false; }
        QSaveFile f(abs);
        if (!f.open(QIODevice::WriteOnly) || f.write(sf.content.toUtf8()) < 0 || !f.commit()) {
            if (errorMsg) *errorMsg = QString("Cannot write %1: %2").arg(sf.path, f.errorString());
            return false;
        }
    }
    return true;
}

// --- validation --------------------------------------------------------------------------------

QStringList Workspace::validate() const
{
    QStringList problems;

    if (m_sourceFiles.size() > kMaxFiles) problems << QString("Too many source files (%1, limit %2)").arg(m_sourceFiles.size()).arg(kMaxFiles);

    QSet<QString> paths;
    qint64 total = 0;
    for (const SourceFile& sf : m_sourceFiles) {
        if (!isSafeRelativePath(sf.path)) {
            problems << QString("Unsafe or non-normalised source path: '%1'").arg(sf.path);
            continue;
        }
        if (paths.contains(sf.path)) problems << QString("Duplicate source path: '%1'").arg(sf.path);
        paths.insert(sf.path);
        const qint64 sz = sf.content.toUtf8().size();
        if (sz > kMaxFileBytes) problems << QString("Source file '%1' is too large (%2 bytes)").arg(sf.path).arg(sz);
        total += sz;
    }
    if (total > kMaxTotalContentBytes) problems << QString("Project content is too large (%1 bytes)").arg(total);

    QSet<QString> profileIds;
    for (const CompilerProfile& p : m_profiles) {
        if (p.id.isEmpty()) problems << "A compiler profile has an empty id";
        else if (profileIds.contains(p.id)) problems << QString("Duplicate profile id: '%1'").arg(p.id);
        profileIds.insert(p.id);
    }

    QSet<QString> targetNames;
    for (const BuildTarget& t : m_targets) {
        if (t.name.isEmpty()) { problems << "A build target has an empty name"; continue; }
        if (targetNames.contains(t.name)) problems << QString("Duplicate target name: '%1'").arg(t.name);
        targetNames.insert(t.name);
        if (!t.mainSourceFile.isEmpty() && !paths.contains(t.mainSourceFile))
            problems << QString("Target '%1': main source '%2' is not part of the project").arg(t.name, t.mainSourceFile);
        for (const QString& f : t.sourceFiles)
            if (!paths.contains(f)) problems << QString("Target '%1': source '%2' is not part of the project").arg(t.name, f);
        for (const QString& inc : t.includePaths)
            if (!isSafeRelativePath(inc)) problems << QString("Target '%1': include path '%2' must be a relative path inside the project").arg(t.name, inc);
    }

    if (!m_configuration.activeProfileId.isEmpty() && !profileIds.contains(m_configuration.activeProfileId))
        problems << QString("Active profile '%1' does not exist").arg(m_configuration.activeProfileId);
    if (!m_configuration.activeTargetName.isEmpty() && !targetNames.contains(m_configuration.activeTargetName))
        problems << QString("Active target '%1' does not exist").arg(m_configuration.activeTargetName);

    static const QRegularExpression envName(R"(^[A-Za-z_][A-Za-z0-9_]*$)");
    for (auto it = m_configuration.environmentVars.constBegin(); it != m_configuration.environmentVars.constEnd(); ++it)
        if (!envName.match(it.key()).hasMatch()) problems << QString("Invalid environment variable name: '%1'").arg(it.key());

    return problems;
}

// --- request building ---------------------------------------------------------------------------

bool Workspace::buildRequest(const QString& profileId, const QString& targetName,
                             CompilationRequest* out, QString* errorMsg) const
{
    auto fail = [&](const QString& m) { if (errorMsg) *errorMsg = m; return false; };
    const QString pid = profileId.isEmpty() ? m_configuration.activeProfileId : profileId;
    const CompilerProfile* prof = findProfile(pid);
    if (!prof) return fail("No such compiler profile: '" + pid + "'");

    QString tname = targetName.isEmpty() ? m_configuration.activeTargetName : targetName;
    const BuildTarget* target = tname.isEmpty() ? (m_targets.isEmpty() ? nullptr : &m_targets.first())
                                                : findTarget(tname);
    if (!target) return fail("No such build target: '" + tname + "'");
    if (target->mainSourceFile.isEmpty() || !hasSourceFile(target->mainSourceFile))
        return fail("Target '" + target->name + "' has no valid main source file");

    CompilationRequest req;
    req.sourceCode = findSourceFile(target->mainSourceFile).content;
    req.compilerImage = prof->compilerImage;
    req.language = findSourceFile(target->mainSourceFile).language;
    req.syntaxFlavor = prof->syntaxFlavor;
    req.userArgs = prof->effectiveFlags();
    req.rustCrateType = prof->rustCrateType;
    req.outputKind = stringToOutputKind(prof->outputFormat);
    // Every other file of the target (headers, modules) is sent along, next to the main file.
    for (const QString& f : target->sourceFiles) {
        if (f == target->mainSourceFile) continue;
        if (hasSourceFile(f)) req.extraFiles.append({f, findSourceFile(f).content});
    }
    for (const SourceFile& sf : m_sourceFiles) {
        if (sf.isHeader && sf.path != target->mainSourceFile) {
            bool dup = false;
            for (const auto& e : req.extraFiles) if (e.first == sf.path) dup = true;
            if (!dup) req.extraFiles.append({sf.path, sf.content});
        }
    }
    *out = req;
    return true;
}

// --- persistence ---------------------------------------------------------------------------------

QByteArray Workspace::toJson() const
{
    QJsonObject root;
    root["format_version"] = m_projectFormatVersion;
    root["name"] = m_name;

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

    QJsonArray profilesArray;
    for (const auto& prof : m_profiles) profilesArray.append(ProfileJson::toObject(prof));
    root["profiles"] = profilesArray;

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

    QJsonObject configObj;
    configObj["active_profile_id"] = m_configuration.activeProfileId;
    configObj["active_target_name"] = m_configuration.activeTargetName;
    QJsonObject envObj;
    for (auto it = m_configuration.environmentVars.constBegin(); it != m_configuration.environmentVars.constEnd(); ++it)
        envObj[it.key()] = it.value();
    configObj["environment_vars"] = envObj;
    root["configuration"] = configObj;

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

Workspace Workspace::fromJson(const QByteArray& json, QString* errorMsg)
{
    auto fail = [&](const QString& m) {
        if (errorMsg) *errorMsg = m;
        return Workspace{};
    };

    if (json.size() > kMaxJsonBytes)
        return fail(QString("Project file is too large (%1 bytes, limit %2)").arg(json.size()).arg(kMaxJsonBytes));

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return fail(QString("JSON parse error: %1 at offset %2").arg(parseError.errorString()).arg(parseError.offset));
    if (!doc.isObject()) return fail("Root JSON element must be an object");

    const QJsonObject root = doc.object();
    QStringList warnings;

    // Version gate: unknown future formats are rejected, never half-understood.
    int fileVersion = kCurrentFormatVersion;
    if (root.contains("format_version")) {
        if (!root["format_version"].isString()) return fail("'format_version' must be a string");
        fileVersion = parseFormatVersion(root["format_version"].toString());
        if (fileVersion < 0)
            return fail(QString("Unrecognised project format '%1'").arg(root["format_version"].toString()));
        if (fileVersion > kCurrentFormatVersion)
            return fail(QString("This project was created by a newer OCTOS (format %1, supported up to %2). "
                                "Please update OCTOS to open it.").arg(root["format_version"].toString(), formatName(kCurrentFormatVersion)));
        if (fileVersion < 1) return fail("Invalid project format version");
    } else {
        warnings << "Project file has no 'format_version'; assuming " + formatName(kCurrentFormatVersion);
    }
    // Migration hook: older formats are upgraded step by step here once they exist
    // (for (v = fileVersion; v < kCurrentFormatVersion; ++v) migrate(root, v);). Format 1 is the
    // oldest and the current one, so there is nothing to migrate yet.

    auto arrayOrEmpty = [&](const char* key, bool* ok) -> QJsonArray {
        const QJsonValue v = root.value(key);
        if (v.isUndefined() || v.isNull()) return QJsonArray();
        if (!v.isArray()) { *ok = false; return QJsonArray(); }
        return v.toArray();
    };

    bool structOk = true;
    const QJsonArray filesArray = arrayOrEmpty("source_files", &structOk);
    const QJsonArray profilesArray = arrayOrEmpty("profiles", &structOk);
    const QJsonArray targetsArray = arrayOrEmpty("targets", &structOk);
    if (!structOk) return fail("'source_files', 'profiles' and 'targets' must be arrays");
    if (root.contains("name") && !root["name"].isString()) return fail("'name' must be a string");

    Workspace ws(root["name"].toString("Untitled Workspace"));
    ws.m_projectFormatVersion = formatName(kCurrentFormatVersion);

    if (filesArray.size() > kMaxFiles) return fail(QString("Too many source files (limit %1)").arg(kMaxFiles));

    for (const auto& val : filesArray) {
        if (!val.isObject()) return fail("Every entry of 'source_files' must be an object");
        const QJsonObject fileObj = val.toObject();
        SourceFile sf;
        if (!fileObj["path"].isString()) return fail("A source file has no 'path'");
        sf.path = fileObj["path"].toString();
        if (fileObj.contains("content") && !fileObj["content"].isString())
            return fail(QString("Source file '%1': 'content' must be a string").arg(sf.path));
        sf.content = fileObj["content"].toString();
        sf.isHeader = fileObj["is_header"].toBool(false);
        const QString lang = fileObj["language"].toString("cpp");
        if (!isKnownLanguage(lang)) warnings << QString("Source file '%1': unknown language '%2', using cpp").arg(sf.path, lang);
        sf.language = stringToLanguage(lang);
        ws.m_sourceFiles.append(sf);   // duplicates are caught by validate(), not silently merged
    }

    for (const auto& val : profilesArray) {
        if (!val.isObject()) return fail("Every entry of 'profiles' must be an object");
        CompilerProfile prof;
        if (!ProfileJson::fromObject(val.toObject(), &prof, &warnings))
            return fail("A compiler profile is missing a valid 'id' or 'name'");
        ws.m_profiles.append(prof);
    }

    for (const auto& val : targetsArray) {
        if (!val.isObject()) return fail("Every entry of 'targets' must be an object");
        const QJsonObject tgtObj = val.toObject();
        BuildTarget tgt;
        tgt.name = tgtObj["name"].toString();
        tgt.mainSourceFile = tgtObj["main_source_file"].toString();
        for (const auto& s : tgtObj["source_files"].toArray()) tgt.sourceFiles.append(s.toString());
        for (const auto& inc : tgtObj["include_paths"].toArray()) tgt.includePaths.append(inc.toString());
        ws.m_targets.append(tgt);
    }

    if (root.contains("configuration")) {
        if (!root["configuration"].isObject()) return fail("'configuration' must be an object");
        const QJsonObject configObj = root["configuration"].toObject();
        CompilationConfiguration config;
        config.activeProfileId = configObj["active_profile_id"].toString();
        config.activeTargetName = configObj["active_target_name"].toString();
        const QJsonObject envObj = configObj["environment_vars"].toObject();
        for (auto it = envObj.constBegin(); it != envObj.constEnd(); ++it)
            config.environmentVars[it.key()] = it.value().toString();
        ws.m_configuration = config;
    }

    const QStringList problems = ws.validate();
    if (!problems.isEmpty()) {
        QStringList shown = problems.mid(0, 5);
        if (problems.size() > 5) shown << QString("... and %1 more").arg(problems.size() - 5);
        return fail("Invalid project: " + shown.join("; "));
    }

    ws.m_warnings = warnings;
    return ws;
}

bool Workspace::saveToFile(const QString& filePath, QString* errorMsg) const
{
    // Never overwrite a good file with an invalid project.
    const QStringList problems = validate();
    if (!problems.isEmpty()) {
        if (errorMsg) *errorMsg = "Project not saved, it is invalid: " + problems.first();
        return false;
    }

    QSaveFile file(filePath);   // writes to a temporary file and renames on commit
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMsg) *errorMsg = QString("Failed to open %1 for writing: %2").arg(filePath, file.errorString());
        return false;
    }
    const QByteArray data = toJson();
    if (file.write(data) != data.size()) {
        file.cancelWriting();
        if (errorMsg) *errorMsg = QString("Failed to write %1: %2").arg(filePath, file.errorString());
        return false;
    }
    if (!file.commit()) {
        if (errorMsg) *errorMsg = QString("Failed to finalise %1: %2").arg(filePath, file.errorString());
        return false;
    }
    return true;
}

Workspace Workspace::loadFromFile(const QString& filePath, QString* errorMsg)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMsg) *errorMsg = QString("Failed to open file for reading: %1 (%2)").arg(filePath, file.errorString());
        return Workspace{};
    }
    if (file.size() > kMaxJsonBytes) {
        if (errorMsg) *errorMsg = QString("Project file is too large (%1 bytes)").arg(file.size());
        return Workspace{};
    }
    const QByteArray data = file.readAll();
    Workspace ws = fromJson(data, errorMsg);
    ws.m_projectDirectory = QFileInfo(filePath).absolutePath();
    return ws;
}

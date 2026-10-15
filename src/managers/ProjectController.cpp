#include "ProjectController.h"
#include <QFileInfo>

ProjectController::ProjectController(QObject* parent) : QObject(parent) {}

bool ProjectController::looksLikeHeader(const QString& path) {
    const QString ext = QFileInfo(path).suffix().toLower();
    return ext == "h" || ext == "hpp" || ext == "hh" || ext == "hxx" || ext == "inc" || ext == "ads";
}

Language ProjectController::languageForPath(const QString& path) {
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "c") return Language::C;
    if (ext == "rs") return Language::Rust;
    if (ext == "java") return Language::Java;
    if (ext == "py") return Language::Python;
    if (ext == "cs") return Language::CSharp;
    if (ext == "adb" || ext == "ads") return Language::Ada;
    return Language::Cpp;   // .cpp/.cc/.cxx/.h/.hpp and anything unknown
}

void ProjectController::newProject(const QString& name) {
    const bool before = isDirty();
    m_working = Workspace(name);
    m_saved = m_working;
    m_filePath.clear();
    m_open = true;
    m_structureDirty = false;
    emit projectChanged();
    emitDirtyIfChanged(before);
}

bool ProjectController::openProject(const QString& filePath, QString* errorMsg) {
    QString err;
    Workspace ws = Workspace::loadFromFile(filePath, &err);
    if (!err.isEmpty()) {
        if (errorMsg) *errorMsg = err;
        return false;
    }
    const bool before = isDirty();
    m_working = ws;
    m_saved = ws;
    m_filePath = filePath;
    m_open = true;
    m_structureDirty = false;
    emit projectChanged();
    emitDirtyIfChanged(before);
    return true;
}

void ProjectController::closeProject() {
    if (!m_open) return;
    const bool before = isDirty();
    m_open = false;
    m_filePath.clear();
    m_working = Workspace();
    m_saved = Workspace();
    m_structureDirty = false;
    emit projectChanged();
    emitDirtyIfChanged(before);
}

bool ProjectController::saveProjectAs(const QString& filePath, QString* errorMsg) {
    if (!m_open) { if (errorMsg) *errorMsg = "No project is open"; return false; }
    if (!m_working.saveToFile(filePath, errorMsg)) return false;   // atomic; nothing changes on failure
    const bool before = isDirty();
    const QSet<QString> wasDirty = dirtyFiles();
    m_saved = m_working;
    m_filePath = filePath;
    m_structureDirty = false;
    for (const QString& p : wasDirty) emit fileDirtyChanged(p, false);
    emitDirtyIfChanged(before);
    return true;
}

bool ProjectController::saveProject(QString* errorMsg) {
    if (m_filePath.isEmpty()) { if (errorMsg) *errorMsg = "The project has no file yet: use Save As"; return false; }
    return saveProjectAs(m_filePath, errorMsg);
}

bool ProjectController::saveFile(const QString& path, QString* errorMsg) {
    if (!m_open) { if (errorMsg) *errorMsg = "No project is open"; return false; }
    if (m_filePath.isEmpty()) { if (errorMsg) *errorMsg = "The project has no file yet: use Save Project As"; return false; }
    if (!m_working.hasSourceFile(path)) { if (errorMsg) *errorMsg = "No such file in the project: " + path; return false; }

    // Write the on-disk state plus exactly this file's new content.
    Workspace toWrite = m_saved;
    toWrite.addSourceFile(m_working.findSourceFile(path));
    if (!toWrite.saveToFile(m_filePath, errorMsg)) return false;

    const bool before = isDirty();
    const bool fileWasDirty = isFileDirty(path);
    m_saved = toWrite;
    if (fileWasDirty) emit fileDirtyChanged(path, false);
    emitDirtyIfChanged(before);
    return true;
}

bool ProjectController::addFile(const QString& path, const QString& content, QString* errorMsg) {
    bool ok = false;
    const QString norm = Workspace::normalizeRelativePath(path, &ok);
    if (!ok) { if (errorMsg) *errorMsg = QString("'%1' is not a valid project-relative path").arg(path); return false; }
    if (m_working.hasSourceFile(norm)) { if (errorMsg) *errorMsg = QString("'%1' already exists in the project").arg(norm); return false; }
    if (content.toUtf8().size() > Workspace::kMaxFileBytes) { if (errorMsg) *errorMsg = "File is too large for a project"; return false; }

    const bool before = isDirty();
    SourceFile sf;
    sf.path = norm;
    sf.content = content;
    sf.isHeader = looksLikeHeader(norm);
    sf.language = languageForPath(norm);
    m_working.addSourceFile(sf);
    m_structureDirty = true;
    emit fileAdded(norm);
    emit projectChanged();
    emit fileDirtyChanged(norm, true);   // a file that is not on disk yet counts as unsaved
    emitDirtyIfChanged(before);
    return true;
}

bool ProjectController::removeFile(const QString& path, QString* errorMsg) {
    if (!m_working.hasSourceFile(path)) { if (errorMsg) *errorMsg = "No such file in the project: " + path; return false; }
    const bool before = isDirty();
    m_working.removeSourceFile(path);
    // Targets must not keep dangling references.
    for (BuildTarget t : m_working.targets()) {
        bool changed = t.sourceFiles.removeAll(path) > 0;
        if (t.mainSourceFile == path) { t.mainSourceFile.clear(); changed = true; }
        if (changed) m_working.addTarget(t);
    }
    m_structureDirty = true;
    emit fileRemoved(path);
    emit projectChanged();
    emitDirtyIfChanged(before);
    return true;
}

QString ProjectController::fileContent(const QString& path) const {
    return m_working.findSourceFile(path).content;
}

void ProjectController::setFileContent(const QString& path, const QString& content) {
    if (!m_working.hasSourceFile(path)) return;
    SourceFile sf = m_working.findSourceFile(path);
    if (sf.content == content) return;
    const bool before = isDirty();
    const bool fileBefore = isFileDirty(path);
    sf.content = content;
    m_working.addSourceFile(sf);
    if (fileBefore != isFileDirty(path)) emit fileDirtyChanged(path, isFileDirty(path));
    emitDirtyIfChanged(before);
}

QStringList ProjectController::filePaths() const {
    QStringList out;
    for (const SourceFile& f : m_working.sourceFiles()) out << f.path;
    return out;
}

bool ProjectController::isFileDirty(const QString& path) const {
    if (!m_working.hasSourceFile(path)) return false;
    if (!m_saved.hasSourceFile(path)) return true;                       // new, not written yet
    return m_working.findSourceFile(path).content != m_saved.findSourceFile(path).content;
}

QSet<QString> ProjectController::dirtyFiles() const {
    QSet<QString> out;
    for (const SourceFile& f : m_working.sourceFiles()) if (isFileDirty(f.path)) out.insert(f.path);
    return out;
}

bool ProjectController::isDirty() const {
    if (!m_open) return false;
    if (m_structureDirty) return true;
    return !dirtyFiles().isEmpty();
}

void ProjectController::markStructureChanged() {
    const bool before = isDirty();
    m_structureDirty = true;
    emit projectChanged();
    emitDirtyIfChanged(before);
}

void ProjectController::emitDirtyIfChanged(bool before) {
    const bool now = isDirty();
    if (now != before) emit dirtyChanged(now);
}

bool ProjectController::addOrUpdateProfile(const CompilerProfile& profile) {
    if (!m_open || profile.id.isEmpty()) return false;
    m_working.addProfile(profile);
    markStructureChanged();
    return true;
}

bool ProjectController::removeProfile(const QString& id) {
    if (!m_open || !m_working.removeProfile(id)) return false;
    markStructureChanged();
    return true;
}

bool ProjectController::setActiveProfile(const QString& id) {
    if (!m_open || !m_working.findProfile(id)) return false;
    CompilationConfiguration c = m_working.configuration();
    c.activeProfileId = id;
    m_working.setConfiguration(c);
    markStructureChanged();
    return true;
}

bool ProjectController::setActiveTarget(const QString& name) {
    if (!m_open || !m_working.findTarget(name)) return false;
    CompilationConfiguration c = m_working.configuration();
    c.activeTargetName = name;
    m_working.setConfiguration(c);
    markStructureChanged();
    return true;
}

bool ProjectController::ensureTargetForMain(const QString& targetName, const QString& mainFile) {
    if (!m_open || !m_working.hasSourceFile(mainFile)) return false;
    BuildTarget t = m_working.findTarget(targetName) ? *m_working.findTarget(targetName) : BuildTarget();
    t.name = targetName;
    t.mainSourceFile = mainFile;
    t.sourceFiles.clear();
    t.sourceFiles << mainFile;
    for (const SourceFile& f : m_working.sourceFiles())
        if (f.isHeader && f.path != mainFile) t.sourceFiles << f.path;
    m_working.addTarget(t);
    markStructureChanged();
    return true;
}

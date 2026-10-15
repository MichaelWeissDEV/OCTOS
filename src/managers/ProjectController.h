#ifndef PROJECTCONTROLLER_H
#define PROJECTCONTROLLER_H

#include <QObject>
#include <QSet>
#include <QString>
#include "../core/WorkspaceModel.h"

/**
 * Owns the open project (Workspace) and everything about its files that is not widget code:
 * working copies, dirty state per file, saving single files or the whole project, adding and
 * removing files, active profile/target. MainWindow only talks to this class and renders it.
 *
 * Two copies are kept: `m_working` (what the editors show) and `m_saved` (what is on disk).
 * Saving a single file writes the saved copy with that file's new content - atomically - so
 * other unsaved files are never written half-way.
 */
class ProjectController : public QObject {
    Q_OBJECT

public:
    explicit ProjectController(QObject* parent = nullptr);

    bool isOpen() const { return m_open; }
    QString projectFilePath() const { return m_filePath; }   // empty for a project that was never saved
    const Workspace& workspace() const { return m_working; }

    // --- lifecycle -----------------------------------------------------------------------------
    void newProject(const QString& name);
    bool openProject(const QString& filePath, QString* errorMsg = nullptr);
    bool saveProject(QString* errorMsg = nullptr);                        // needs a path (see saveProjectAs)
    bool saveProjectAs(const QString& filePath, QString* errorMsg = nullptr);
    bool saveFile(const QString& path, QString* errorMsg = nullptr);      // one file
    void closeProject();                                                  // discards unsaved changes: ask first

    // --- files ---------------------------------------------------------------------------------
    // Adds a file (normalised, must be safe and unique). Headers are recognised by extension if not given.
    bool addFile(const QString& path, const QString& content, QString* errorMsg = nullptr);
    bool removeFile(const QString& path, QString* errorMsg = nullptr);
    QString fileContent(const QString& path) const;
    void setFileContent(const QString& path, const QString& content);     // from the editor
    QStringList filePaths() const;

    // --- dirty state ---------------------------------------------------------------------------
    bool isFileDirty(const QString& path) const;
    QSet<QString> dirtyFiles() const;
    bool isDirty() const;                                                 // any file or structure change

    // --- profiles / targets --------------------------------------------------------------------
    bool addOrUpdateProfile(const CompilerProfile& profile);
    bool removeProfile(const QString& id);
    bool setActiveProfile(const QString& id);
    bool setActiveTarget(const QString& name);
    // Creates/updates the target that builds `mainFile` together with all headers.
    bool ensureTargetForMain(const QString& targetName, const QString& mainFile);

    static bool looksLikeHeader(const QString& path);
    static Language languageForPath(const QString& path);

signals:
    void projectChanged();                       // opened, closed, files/profiles/targets changed
    void fileAdded(const QString& path);
    void fileRemoved(const QString& path);
    void fileDirtyChanged(const QString& path, bool dirty);
    void dirtyChanged(bool anyDirty);

private:
    void markStructureChanged();
    void emitDirtyIfChanged(bool before);

    bool m_open{false};
    QString m_filePath;
    Workspace m_working;
    Workspace m_saved;
    bool m_structureDirty{false};   // files added/removed or profile/target edits not yet written
};

#endif // PROJECTCONTROLLER_H

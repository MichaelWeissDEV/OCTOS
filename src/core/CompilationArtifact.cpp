#include "CompilationArtifact.h"
#include <QSaveFile>

bool CompilationArtifact::saveTo(const QString& path, QString* errorMsg) {
    if (path.isEmpty()) {
        if (errorMsg) *errorMsg = "No target path";
        return false;
    }
    QSaveFile f(path);
    const QByteArray bytes = content.toUtf8();
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit()) {
        if (errorMsg) *errorMsg = QString("Could not write %1: %2").arg(path, f.errorString());
        return false;
    }
    persistentPath = path;
    storage = ArtifactStorage::PersistentFile;
    return true;
}

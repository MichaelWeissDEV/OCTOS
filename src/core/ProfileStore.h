#ifndef PROFILESTORE_H
#define PROFILESTORE_H

#include <QString>
#include <QVector>
#include "WorkspaceModel.h"

// User-level compiler profiles that survive between sessions (stored in the application settings).
// Example: save "GCC -O0" and "GCC -O3" once and compare them in any project without re-entering flags.
class ProfileStore {
public:
    // Loads from AppSettings. Corrupt entries are skipped (and reported through `warnings`), never fatal.
    static ProfileStore load(QStringList* warnings = nullptr);
    bool save(QString* errorMsg = nullptr) const;

    const QVector<CompilerProfile>& profiles() const { return m_profiles; }
    bool add(const CompilerProfile& profile, QString* errorMsg = nullptr);  // replaces the same id
    bool remove(const QString& id);
    const CompilerProfile* find(const QString& id) const;
    // Generates an id that is unique within the store, based on the profile name.
    QString makeUniqueId(const QString& baseName) const;

    // Two requests for the same source, differing only in the profile. Both use identical inputs.
    static bool buildComparisonRequests(const CompilerProfile& a, const CompilerProfile& b,
                                        const QString& source, CompilationRequest* outA, CompilationRequest* outB);

private:
    QVector<CompilerProfile> m_profiles;
};

#endif // PROFILESTORE_H

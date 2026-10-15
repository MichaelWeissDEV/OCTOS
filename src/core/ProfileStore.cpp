#include "ProfileStore.h"
#include "AppSettings.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

ProfileStore ProfileStore::load(QStringList* warnings) {
    ProfileStore store;
    const QByteArray raw = AppSettings::instance().compilerProfilesJson();
    if (raw.isEmpty()) return store;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        if (warnings) warnings->append("Stored compiler profiles are corrupt and were ignored: " + err.errorString());
        return store;
    }
    for (const auto& v : doc.array()) {
        CompilerProfile p;
        QStringList w;
        if (!v.isObject() || !ProfileJson::fromObject(v.toObject(), &p, &w)) {
            if (warnings) warnings->append("A stored compiler profile was invalid and skipped");
            continue;
        }
        if (store.find(p.id)) {
            if (warnings) warnings->append(QString("Duplicate stored profile id '%1' skipped").arg(p.id));
            continue;
        }
        if (warnings) warnings->append(w);
        store.m_profiles.append(p);
    }
    return store;
}

bool ProfileStore::save(QString* errorMsg) const {
    QJsonArray arr;
    for (const auto& p : m_profiles) arr.append(ProfileJson::toObject(p));
    AppSettings::instance().setCompilerProfilesJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    AppSettings::instance().sync();
    Q_UNUSED(errorMsg);
    return true;
}

bool ProfileStore::add(const CompilerProfile& profile, QString* errorMsg) {
    if (profile.id.trimmed().isEmpty() || profile.name.trimmed().isEmpty()) {
        if (errorMsg) *errorMsg = "A profile needs an id and a name";
        return false;
    }
    for (auto& p : m_profiles) {
        if (p.id == profile.id) { p = profile; return true; }
    }
    m_profiles.append(profile);
    return true;
}

bool ProfileStore::remove(const QString& id) {
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == id) { m_profiles.removeAt(i); return true; }
    }
    return false;
}

const CompilerProfile* ProfileStore::find(const QString& id) const {
    for (const auto& p : m_profiles) if (p.id == id) return &p;
    return nullptr;
}

QString ProfileStore::makeUniqueId(const QString& baseName) const {
    QString base = baseName.toLower();
    base.replace(QRegularExpression("[^a-z0-9]+"), "-");
    base = base.trimmed();
    while (base.startsWith('-')) base.remove(0, 1);
    while (base.endsWith('-')) base.chop(1);
    if (base.isEmpty()) base = "profile";
    QString id = base;
    for (int n = 2; find(id); ++n) id = QString("%1-%2").arg(base).arg(n);
    return id;
}

bool ProfileStore::buildComparisonRequests(const CompilerProfile& a, const CompilerProfile& b,
                                           const QString& source, CompilationRequest* outA, CompilationRequest* outB) {
    if (!outA || !outB) return false;
    auto make = [&](const CompilerProfile& p) {
        CompilationRequest r;
        r.sourceCode = source;
        r.compilerImage = p.compilerImage;
        r.language = p.language;
        r.syntaxFlavor = p.syntaxFlavor;
        r.userArgs = p.effectiveFlags();
        r.rustCrateType = p.rustCrateType;
        r.outputKind = stringToOutputKind(p.outputFormat);
        return r;
    };
    *outA = make(a);
    *outB = make(b);
    return true;
}

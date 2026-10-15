#include "ToolchainProbe.h"
#include "CompilerDefinition.h"

QJsonObject ToolchainProbeResult::toJson() const {
    QJsonObject o;
    o["performed"] = performed;
    o["success"] = success;
    o["found"] = QJsonArray::fromStringList(foundBinaries);
    o["missing"] = QJsonArray::fromStringList(missingBinaries);
    o["version"] = version;
    o["architecture"] = architecture;
    o["imageDigest"] = imageDigest;
    o["checkedAt"] = checkedAt.toUTC().toString(Qt::ISODate);
    o["message"] = message;
    return o;
}

ToolchainProbeResult ToolchainProbeResult::fromJson(const QJsonObject& o) {
    ToolchainProbeResult r;
    r.performed = o["performed"].toBool();
    r.success = o["success"].toBool();
    for (const auto& v : o["found"].toArray()) r.foundBinaries << v.toString();
    for (const auto& v : o["missing"].toArray()) r.missingBinaries << v.toString();
    r.version = o["version"].toString();
    r.architecture = o["architecture"].toString();
    r.imageDigest = o["imageDigest"].toString();
    r.checkedAt = QDateTime::fromString(o["checkedAt"].toString(), Qt::ISODate);
    r.message = o["message"].toString();
    return r;
}

namespace ToolchainProbe {

namespace {
QString versionFlag(CompilerFamily f) {
    return f == CompilerFamily::OpenJDK ? "-version" : "--version";
}
}

QStringList buildDockerArgs(const QString& image, const CompilerDefinition& def) {
    // $1 = version flag, $2.. = required binaries. Nothing is interpolated into the script.
    static const char* script =
        "flag=\"$1\"; shift; "
        "for b in \"$@\"; do if command -v \"$b\" >/dev/null 2>&1; then echo \"FOUND:$b\"; else echo \"MISSING:$b\"; fi; done; "
        "echo \"ARCH:$(uname -m)\"; "
        "first=\"$1\"; if [ -n \"$first\" ] && command -v \"$first\" >/dev/null 2>&1; then "
        "  echo \"VERSION:$(\"$first\" \"$flag\" 2>&1 | head -n 1)\"; fi";

    QStringList args;
    args << "run" << "--rm" << "--network" << "none" << "--cap-drop=ALL"
         << "--security-opt" << "no-new-privileges" << "--memory" << "256m" << "--pids-limit" << "64"
         << "--entrypoint" << "sh" << image << "-c" << QString::fromLatin1(script) << "octos_probe"
         << versionFlag(def.family);
    args.append(def.requiredBinaries);
    return args;
}

ToolchainProbeResult parseOutput(const CompilerDefinition& def, int exitCode, const QString& out,
                                 const QString& err, const QString& imageDigest) {
    ToolchainProbeResult r;
    r.performed = true;
    r.checkedAt = QDateTime::currentDateTimeUtc();
    r.imageDigest = imageDigest;

    for (const QString& raw : out.split('\n')) {
        const QString line = raw.trimmed();
        if (line.startsWith("FOUND:")) r.foundBinaries << line.mid(6);
        else if (line.startsWith("MISSING:")) r.missingBinaries << line.mid(8);
        else if (line.startsWith("ARCH:")) r.architecture = line.mid(5).trimmed();
        else if (line.startsWith("VERSION:")) r.version = line.mid(8).trimmed();
    }

    if (exitCode != 0) {
        r.message = QString("Probe could not run in the image (exit %1): %2").arg(exitCode).arg(err.trimmed().left(300));
        return r;
    }
    // Only binaries that were explicitly reported count; a binary neither FOUND nor MISSING is missing.
    for (const QString& need : def.requiredBinaries) {
        if (!r.foundBinaries.contains(need) && !r.missingBinaries.contains(need)) r.missingBinaries << need;
    }
    if (!r.missingBinaries.isEmpty()) {
        r.message = "Required tools missing in image: " + r.missingBinaries.join(", ");
        return r;
    }
    if (def.requiredBinaries.isEmpty()) {
        r.message = "No required tools are defined for this image type; it cannot be verified.";
        return r;
    }
    if (r.version.isEmpty()) {
        r.message = "The compiler did not report a version.";
        return r;
    }
    r.success = true;
    return r;
}

}

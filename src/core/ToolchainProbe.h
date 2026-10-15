#ifndef TOOLCHAINPROBE_H
#define TOOLCHAINPROBE_H

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

struct CompilerDefinition;

// Outcome of actually running a probe inside the image. Recognising an image NAME proves
// nothing about its contents; only a successful probe does.
struct ToolchainProbeResult {
    bool performed{false};
    bool success{false};
    QStringList foundBinaries;
    QStringList missingBinaries;
    QString version;          // first line printed by the compiler's version command
    QString architecture;     // `uname -m` inside the container
    QString imageDigest;      // image ID the probe ran against
    QDateTime checkedAt;
    QString message;          // human readable reason on failure

    QJsonObject toJson() const;
    static ToolchainProbeResult fromJson(const QJsonObject& o);
};

namespace ToolchainProbe {

// docker arguments of the probe container (no network, all caps dropped, small limits).
QStringList buildDockerArgs(const QString& image, const CompilerDefinition& def);

// Interprets the probe output. `imageDigest` is the ID obtained from `docker image inspect`.
ToolchainProbeResult parseOutput(const CompilerDefinition& def, int exitCode, const QString& out,
                                 const QString& err, const QString& imageDigest);

}

#endif // TOOLCHAINPROBE_H

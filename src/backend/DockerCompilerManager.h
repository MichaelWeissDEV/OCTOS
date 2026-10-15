#ifndef DOCKERCOMPILERMANAGER_H
#define DOCKERCOMPILERMANAGER_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMetaType>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>
#include <memory>

#include "../core/CompilerEnums.h"
#include "../core/CompilationRequest.h"
#include "../core/CompilationResult.h"
#include "../core/ToolchainProbe.h"
#include <functional>

struct DockerImage {
    QString registry;
    QString tag;
    QString fullName;
    QString architecture;
    QString imageId;      // local image ID (only for installed images)
    bool isInstalled{false};
    qint64 size{0};

    QString displayName() const { return fullName; }
};

struct CompilationJob {
    QString sourceCode;
    QString dockerImage;
    QString compilerFlags;
    QString language;
    int jobId;
};

struct CompilationOutput {
    bool success;
    QString stdout_;
    QString stderr_;
    int exitCode;
    int jobId;
};

Q_DECLARE_METATYPE(CompilationOutput);

class JobManager;

class DockerCompilerManager : public QObject {
    Q_OBJECT

public:
    explicit DockerCompilerManager(QObject* parent = nullptr);
    ~DockerCompilerManager() override;

    bool isDockerAvailable() const { return m_dockerAvailable; }
    // Architecture of the Docker daemon ("aarch64", "x86_64", ...); empty until known. Images are
    // native to it unless pulled with an explicit platform.
    QString daemonArchitecture() const { return m_daemonArchitecture; }
    void checkDockerConnectionAsync();

    void fetchAvailableTags(CompilerCategory category);
    QVector<DockerImage> getFilteredTags(CompilerCategory category) const;

    void pullImage(const QString& imageName);
    // Deleting an image is always an explicit user decision made in the UI. Images OCTOS did not
    // pull itself are only removed with confirmedForeign=true (the UI shows a stronger warning).
    void removeImage(const QString& imageName, bool confirmedForeign = false);
    // Cached list only - never blocks. Call refreshInstalledImagesAsync() and listen to
    // installedImagesRefreshed() for updates.
    QVector<DockerImage> listInstalledImages() const { return m_installedImages; }
    void refreshInstalledImagesAsync();
    bool isImageInstalled(const QString& imageName) const;
    bool isOwnedByOctos(const QString& imageName) const;

    // Runs the toolchain probe inside the (installed) image. Result arrives via toolchainProbed().
    void probeToolchain(const QString& imageName);

    int submitCompilationJob(const QString& sourceCode, const QString& dockerImage,
                             const QString& compilerFlags, const QString& language,
                             quint64 paneId = 0, quint64 revision = 0);
    quint64 submitRequest(const CompilationRequest& req);
    void cancelJob(int jobId);
    void cancelPaneJobs(quint64 paneId);

    QString getCategoryRegistry(CompilerCategory category) const;
    QStringList getSupportedArchitectures() const;

    JobManager* jobManager() const { return m_jobManager.get(); }

signals:
    void dockerConnectionStatusChanged(bool available, const QString& message);
    void installedImagesRefreshed(const QVector<DockerImage>& images);
    void installedImagesRefreshFailed(const QString& message);
    void toolchainProbed(const QString& imageName, const ToolchainProbeResult& result);
    void imageRemoved(const QString& imageName, bool success, const QString& message);

    void tagsReceived(CompilerCategory category, const QVector<DockerImage>& images);
    void tagsFetchError(CompilerCategory category, const QString& error);

    void pullProgress(const QString& imageName, int percentage);
    void pullFinished(const QString& imageName, bool success, const QString& message);

    void compilationFinished(int jobId, const CompilationOutput& output);
    void compilationResultReceived(const CompilationResult& result);
    void compilationProgress(int jobId, int percentage);

private slots:
    void onTagsReplyFinished();
    void onPullProcessOutput();
    void onPullProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onJobManagerResult(const CompilationResult& result);
    void onJobManagerStateChanged(quint64 jobId, quint64 paneId, JobState state);

private:
    QNetworkAccessManager* m_networkManager;
    QMap<CompilerCategory, QVector<DockerImage>> m_availableTags;
    QVector<DockerImage> m_installedImages;
    bool m_dockerAvailable{false};
    QString m_daemonArchitecture;

    QProcess* m_currentPullProcess{nullptr};
    QString m_currentPullImage;
    bool m_pullTargetWasInstalled{false};
    bool m_refreshInFlight{false};

    std::unique_ptr<JobManager> m_jobManager;

    QVector<DockerImage> parseDockerHubTags(const QJsonDocument& doc, const QString& repository);
    QVector<DockerImage> filterStableTags(const QVector<DockerImage>& tags);
    bool isStableTag(const QString& tag);
    using DockerDone = std::function<void(int exitCode, const QString& out, const QString& err, bool timedOut)>;
    void runDockerAsync(const QStringList& args, int timeoutMs, DockerDone done);
    void persistProbes();
    void parsePullProgress(const QString& output, int& percentage);
};

#endif // DOCKERCOMPILERMANAGER_H

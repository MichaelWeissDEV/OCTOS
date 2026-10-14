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

struct DockerImage {
    QString registry;
    QString tag;
    QString fullName;
    QString architecture;
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
    void checkDockerConnectionAsync();

    void fetchAvailableTags(CompilerCategory category);
    QVector<DockerImage> getFilteredTags(CompilerCategory category) const;

    void pullImage(const QString& imageName);
    void removeImage(const QString& imageName);
    QVector<DockerImage> listInstalledImages();
    void refreshInstalledImagesAsync();
    bool isImageInstalled(const QString& imageName);

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

    QProcess* m_currentPullProcess{nullptr};
    QString m_currentPullImage;

    std::unique_ptr<JobManager> m_jobManager;

    QVector<DockerImage> parseDockerHubTags(const QJsonDocument& doc, const QString& repository);
    QVector<DockerImage> filterStableTags(const QVector<DockerImage>& tags);
    bool isStableTag(const QString& tag);
    void refreshInstalledImagesSync();
    void parsePullProgress(const QString& output, int& percentage);
};

#endif // DOCKERCOMPILERMANAGER_H

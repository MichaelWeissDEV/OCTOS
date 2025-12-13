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
#include <QThreadPool>
#include <QVector>

struct DockerImage {
    QString registry;
    QString tag;
    QString fullName;
    QString architecture;
    bool isInstalled;
    qint64 size;

    QString displayName() const { return fullName; }
};

enum class CompilerCategory { Cpp, C, Java, Python, Rust, Clang };

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

class CompilationTask;

class DockerCompilerManager : public QObject {
    Q_OBJECT

   public:
    explicit DockerCompilerManager(QObject* parent = nullptr);
    ~DockerCompilerManager();

    void fetchAvailableTags(CompilerCategory category);
    QVector<DockerImage> getFilteredTags(CompilerCategory category) const;

    void pullImage(const QString& imageName);
    void removeImage(const QString& imageName);
    QVector<DockerImage> listInstalledImages();
    bool isImageInstalled(const QString& imageName);

    int submitCompilationJob(const QString& sourceCode, const QString& dockerImage,
                             const QString& compilerFlags, const QString& language);
    void cancelJob(int jobId);

    QString getCategoryRegistry(CompilerCategory category) const;
    QStringList getSupportedArchitectures() const;

   signals:
    void tagsReceived(CompilerCategory category, const QVector<DockerImage>& images);
    void tagsFetchError(CompilerCategory category, const QString& error);

    void pullProgress(const QString& imageName, int percentage);
    void pullFinished(const QString& imageName, bool success, const QString& message);

    void compilationFinished(int jobId, const CompilationOutput& output);
    void compilationProgress(int jobId, int percentage);

   private slots:
    void onTagsReplyFinished();
    void onPullProcessOutput();
    void onPullProcessFinished(int exitCode, QProcess::ExitStatus status);
    void handleTaskFinished(const CompilationOutput& output, CompilationTask* task);

   private:
    friend class CompilationTask;

    QNetworkAccessManager* m_networkManager;
    QMap<CompilerCategory, QVector<DockerImage>> m_availableTags;
    QVector<DockerImage> m_installedImages;

    QProcess* m_currentPullProcess;
    QString m_currentPullImage;

    QMap<int, CompilationTask*> m_activeJobs;
    QThreadPool m_threadPool;
    QMutex m_jobMutex;
    int m_nextJobId;

    QVector<DockerImage> parseDockerHubTags(const QJsonDocument& doc, const QString& repository);
    QVector<DockerImage> filterStableTags(const QVector<DockerImage>& tags);
    bool isStableTag(const QString& tag);
    void refreshInstalledImages();
    void parsePullProgress(const QString& output, int& percentage);
};

#endif

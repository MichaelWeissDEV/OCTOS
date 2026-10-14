#include "DockerCompilerManager.h"
#include "JobManager.h"
#include "../core/AppSettings.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QList>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QUrl>

// Timeout constants
constexpr int kDockerStartTimeoutMs = 5000;    // 5 seconds to start Docker process
constexpr int kCleanupTimeoutMs = 10000;       // 10 seconds for cleanup operations

DockerCompilerManager::DockerCompilerManager(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_currentPullProcess(nullptr),
      m_jobManager(std::make_unique<JobManager>(nullptr, this)) {
    qRegisterMetaType<CompilationOutput>("CompilationOutput");
    qRegisterMetaType<CompilationResult>("CompilationResult");
    qRegisterMetaType<CompilerCategory>("CompilerCategory");

    connect(m_jobManager.get(), &JobManager::jobFinished, this,
            &DockerCompilerManager::onJobManagerResult);
    connect(m_jobManager.get(), &JobManager::jobStateChanged, this,
            &DockerCompilerManager::onJobManagerStateChanged);

    refreshInstalledImages();
}

DockerCompilerManager::~DockerCompilerManager() {
    if (m_currentPullProcess) {
        m_currentPullProcess->kill();
        delete m_currentPullProcess;
        m_currentPullProcess = nullptr;
    }
}

QString DockerCompilerManager::getCategoryRegistry(CompilerCategory category) const {
    switch (category) {
        case CompilerCategory::Cpp:
        case CompilerCategory::C:
            return "library/gcc";
        case CompilerCategory::Clang:
            return "silkeh/clang";
        case CompilerCategory::Java:
            return "library/openjdk";
        case CompilerCategory::Python:
            return "library/python";
        case CompilerCategory::Rust:
            return "library/rust";
        case CompilerCategory::CSharp:
            return "library/mono";
        case CompilerCategory::Ada:
            return "library/gcc";
        default:
            return "";
    }
}

void DockerCompilerManager::fetchAvailableTags(CompilerCategory category) {
    QString registry = getCategoryRegistry(category);
    if (registry.isEmpty()) {
        emit tagsFetchError(category, "Unknown category or unsupported registry");
        return;
    }

    QString url = QString("https://registry.hub.docker.com/v2/repositories/%1/tags?page_size=100")
                      .arg(registry);

    QUrl requestUrl(url);
    QNetworkRequest request{requestUrl};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("category", QVariant::fromValue(category));

    connect(reply, &QNetworkReply::finished, this, &DockerCompilerManager::onTagsReplyFinished);
}

void DockerCompilerManager::onTagsReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    CompilerCategory category = reply->property("category").value<CompilerCategory>();

    if (reply->error() != QNetworkReply::NoError) {
        emit tagsFetchError(category, reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        emit tagsFetchError(category, "Invalid JSON response from Docker Hub");
        reply->deleteLater();
        return;
    }

    QString registry = getCategoryRegistry(category);
    QVector<DockerImage> images = parseDockerHubTags(doc, registry);
    QVector<DockerImage> filtered = filterStableTags(images);

    m_availableTags[category] = filtered;

    emit tagsReceived(category, filtered);
    reply->deleteLater();
}

QVector<DockerImage> DockerCompilerManager::parseDockerHubTags(const QJsonDocument& doc,
                                                               const QString& repository) {
    QVector<DockerImage> images;

    QJsonObject root = doc.object();
    QJsonArray results = root["results"].toArray();

    for (const QJsonValue& value : results) {
        QJsonObject tagObj = value.toObject();

        DockerImage image;
        image.registry = repository;
        image.tag = tagObj["name"].toString();
        image.fullName = repository + ":" + image.tag;
        image.size = tagObj["full_size"].toInteger();

        QJsonArray images_arr = tagObj["images"].toArray();
        if (!images_arr.isEmpty()) {
            image.architecture = images_arr[0].toObject()["architecture"].toString();
        } else {
            image.architecture = "amd64";
        }

        image.isInstalled = isImageInstalled(image.fullName);

        images.append(image);
    }

    return images;
}

QVector<DockerImage> DockerCompilerManager::filterStableTags(const QVector<DockerImage>& tags) {
    QVector<DockerImage> filtered;
    for (const DockerImage& image : tags) {
        if (isStableTag(image.tag)) {
            filtered.append(image);
        }
    }
    return filtered;
}

bool DockerCompilerManager::isStableTag(const QString& tag) {
    if (tag.contains("rc", Qt::CaseInsensitive) || tag.contains("beta", Qt::CaseInsensitive) ||
        tag.contains("nightly", Qt::CaseInsensitive) || tag.contains("dev", Qt::CaseInsensitive) ||
        tag.contains("alpha", Qt::CaseInsensitive) ||
        tag.contains("snapshot", Qt::CaseInsensitive)) {
        return false;
    }
    return true;
}

QVector<DockerImage> DockerCompilerManager::getFilteredTags(CompilerCategory category) const {
    return m_availableTags.value(category, QVector<DockerImage>());
}

void DockerCompilerManager::pullImage(const QString& imageName) {
    if (m_currentPullProcess && m_currentPullProcess->state() == QProcess::Running) {
        emit pullFinished(imageName, false, "Another pull operation is in progress");
        return;
    }

    m_currentPullImage = imageName;

    m_currentPullProcess = new QProcess(this);
    connect(m_currentPullProcess, &QProcess::readyReadStandardOutput, this,
            &DockerCompilerManager::onPullProcessOutput);
    connect(m_currentPullProcess, &QProcess::readyReadStandardError, this,
            &DockerCompilerManager::onPullProcessOutput);
    connect(m_currentPullProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DockerCompilerManager::onPullProcessFinished);

    m_currentPullProcess->start("docker", QStringList() << "pull" << imageName);

    if (!m_currentPullProcess->waitForStarted(kDockerStartTimeoutMs)) {
        emit pullFinished(imageName, false, "Failed to start Docker pull process");
        delete m_currentPullProcess;
        m_currentPullProcess = nullptr;
    }
}

void DockerCompilerManager::onPullProcessOutput() {
    if (!m_currentPullProcess) return;

    QString output = QString::fromUtf8(m_currentPullProcess->readAllStandardOutput());
    QString error = QString::fromUtf8(m_currentPullProcess->readAllStandardError());

    int percentage = 0;
    parsePullProgress(output + error, percentage);

    if (percentage > 0) {
        emit pullProgress(m_currentPullImage, percentage);
    }
}

void DockerCompilerManager::parsePullProgress(const QString& output, int& percentage) {
    QRegularExpression progressRegex(R"((\d+)%)");
    QRegularExpressionMatch match = progressRegex.match(output);

    if (match.hasMatch()) {
        percentage = match.captured(1).toInt();
    } else {
        QRegularExpression byteRegex(R"((\d+\.?\d*)\s*[KMG]B\s*/\s*(\d+\.?\d*)\s*[KMG]B)");
        QRegularExpressionMatch byteMatch = byteRegex.match(output);

        if (byteMatch.hasMatch()) {
            percentage = 50;
        }
    }
}

void DockerCompilerManager::onPullProcessFinished(int exitCode, QProcess::ExitStatus status) {
    bool success = (exitCode == 0 && status == QProcess::NormalExit);
    QString message = success ? "Image pulled successfully" : "Failed to pull image";

    if (!success && m_currentPullProcess) {
        message += ": " + QString::fromUtf8(m_currentPullProcess->readAllStandardError());
    }

    emit pullFinished(m_currentPullImage, success, message);

    if (success) {
        refreshInstalledImages();
    }

    if (m_currentPullProcess) {
        m_currentPullProcess->deleteLater();
        m_currentPullProcess = nullptr;
    }
}

void DockerCompilerManager::removeImage(const QString& imageName) {
    QProcess process;
    process.start("docker", QStringList() << "rmi" << imageName);
    process.waitForFinished(kCleanupTimeoutMs);

    refreshInstalledImages();
}

QVector<DockerImage> DockerCompilerManager::listInstalledImages() {
    refreshInstalledImages();
    return m_installedImages;
}

void DockerCompilerManager::refreshInstalledImages() {
    m_installedImages.clear();

    QProcess process;
    process.start("docker", QStringList()
                                << "images" << "--format" << "{{.Repository}}:{{.Tag}}|{{.Size}}");
    if (!process.waitForFinished(kCleanupTimeoutMs)) {
        return;
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() != 2) continue;

        QString fullName = parts[0];

        if (fullName.contains("gcc") || fullName.contains("clang") ||
            fullName.contains("openjdk") || fullName.contains("python") ||
            fullName.contains("rust") || fullName.contains("mono") ||
            fullName.contains("temurin") || fullName.contains("llvm")) {
            DockerImage image;
            QStringList nameParts = fullName.split(':');
            if (nameParts.size() == 2) {
                image.registry = nameParts[0];
                image.tag = nameParts[1];
            }
            image.fullName = fullName;
            image.isInstalled = true;
            image.size = 0;

            m_installedImages.append(image);
        }
    }
}

bool DockerCompilerManager::isImageInstalled(const QString& imageName) {
    QProcess process;
    process.start("docker", QStringList() << "images" << "-q" << imageName);
    if (!process.waitForFinished(kCleanupTimeoutMs)) {
        return false;
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return !output.isEmpty();
}

int DockerCompilerManager::submitCompilationJob(const QString& sourceCode,
                                                const QString& dockerImage,
                                                const QString& compilerFlags,
                                                const QString& language,
                                                quint64 paneId,
                                                quint64 revision) {
    CompilationRequest req;
    req.sourceCode = sourceCode;
    req.compilerImage = dockerImage;
    req.userArgs = compilerFlags;
    req.language = stringToLanguage(language);
    req.paneId = paneId;
    req.revision = revision;

    // Detect syntax flavor from flags if present, otherwise default to Intel
    if (compilerFlags.contains("-masm=att")) {
        req.syntaxFlavor = SyntaxFlavor::ATT;
    } else {
        req.syntaxFlavor = SyntaxFlavor::Intel;
    }

    quint64 jobId = m_jobManager->submitJob(req);
    return static_cast<int>(jobId);
}

quint64 DockerCompilerManager::submitRequest(const CompilationRequest& req) {
    return m_jobManager->submitJob(req);
}

void DockerCompilerManager::cancelJob(int jobId) {
    m_jobManager->cancelJob(static_cast<quint64>(jobId));
}

void DockerCompilerManager::cancelPaneJobs(quint64 paneId) {
    m_jobManager->cancelPaneJobs(paneId);
}

void DockerCompilerManager::onJobManagerStateChanged(quint64 jobId, quint64 /*paneId*/, JobState state) {
    if (state == JobState::Running) {
        emit compilationProgress(static_cast<int>(jobId), 50);
    } else if (state == JobState::Completed || state == JobState::Failed ||
               state == JobState::Cancelled || state == JobState::TimedOut) {
        emit compilationProgress(static_cast<int>(jobId), 100);
    }
}

void DockerCompilerManager::onJobManagerResult(const CompilationResult& result) {
    emit compilationResultReceived(result);

    // Provide legacy CompilationOutput for compatibility
    CompilationOutput legacy;
    legacy.jobId = static_cast<int>(result.jobId);
    legacy.success = result.success();
    legacy.exitCode = result.exitCode;
    legacy.stdout_ = result.assemblyOutput.isEmpty() ? result.rawOutput : result.assemblyOutput;
    legacy.stderr_ = result.errorOutput;

    emit compilationFinished(legacy.jobId, legacy);
}

QStringList DockerCompilerManager::getSupportedArchitectures() const {
    return QStringList() << "amd64" << "arm64v8" << "arm32v7";
}

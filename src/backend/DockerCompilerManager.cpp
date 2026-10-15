#include "DockerCompilerManager.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>

#include "JobManager.h"
#include "../core/CompilerRegistry.h"
#include "../core/AppSettings.h"
#include <QTimer>

namespace {
constexpr int kDockerStartTimeoutMs = 5000;
constexpr int kCleanupTimeoutMs = 3000;
}

DockerCompilerManager::DockerCompilerManager(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_jobManager(std::make_unique<JobManager>(nullptr, this)) {
    connect(m_jobManager.get(), &JobManager::jobFinished, this,
            &DockerCompilerManager::onJobManagerResult);
    connect(m_jobManager.get(), &JobManager::jobStateChanged, this,
            &DockerCompilerManager::onJobManagerStateChanged);

    // Previously stored probe results (only trusted while the image ID still matches).
    const QByteArray stored = AppSettings::instance().toolchainProbesJson();
    if (!stored.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(stored);
        if (doc.isObject()) CompilerRegistry::instance().importProbes(doc.object());
    }

    // Initial async connection and image query
    checkDockerConnectionAsync();
}

DockerCompilerManager::~DockerCompilerManager() {
    // Outstanding async docker commands (image list, probe, rmi): stop them cleanly so no QProcess
    // is destroyed while running and no callback fires into a half-destroyed manager.
    const auto procs = findChildren<QProcess*>(QString(), Qt::FindDirectChildrenOnly);
    for (QProcess* p : procs) {
        if (p == m_currentPullProcess) continue;
        p->disconnect(this);
        p->kill();
        p->waitForFinished(kCleanupTimeoutMs);
    }
    if (m_currentPullProcess) {
        m_currentPullProcess->kill();
        m_currentPullProcess->waitForFinished(kCleanupTimeoutMs);
    }
}

void DockerCompilerManager::checkDockerConnectionAsync() {
    auto* proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus status) {
        m_dockerAvailable = (exitCode == 0 && status == QProcess::NormalExit);
        if (m_dockerAvailable) m_daemonArchitecture = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        QString msg = m_dockerAvailable ? "Docker daemon is running and reachable."
                                        : "Cannot connect to Docker daemon. Is Docker installed and running?";
        emit dockerConnectionStatusChanged(m_dockerAvailable, msg);
        if (m_dockerAvailable) {
            refreshInstalledImagesAsync();
        }
        proc->deleteLater();
    });

    proc->start("docker", QStringList() << "info" << "--format" << "{{.Architecture}}");
    if (!proc->waitForStarted(2000)) {
        m_dockerAvailable = false;
        emit dockerConnectionStatusChanged(false, "Failed to launch 'docker' command. Ensure Docker is in PATH.");
        proc->deleteLater();
    }
}

void DockerCompilerManager::fetchAvailableTags(CompilerCategory category) {
    QString repository = getCategoryRegistry(category);
    if (repository.isEmpty()) {
        emit tagsFetchError(category, "Invalid compiler category");
        return;
    }

    QUrl url(QString("https://registry.hub.docker.com/v2/repositories/%1/tags").arg(repository));
    QUrlQuery query;
    query.addQueryItem("page_size", "50");
    query.addQueryItem("ordering", "last_updated");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "OCTOS-Compiler-Explorer/0.4.1");

    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("category", static_cast<int>(category));
    reply->setProperty("repository", repository);

    connect(reply, &QNetworkReply::finished, this, &DockerCompilerManager::onTagsReplyFinished);
}

void DockerCompilerManager::onTagsReplyFinished() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();

    CompilerCategory category = static_cast<CompilerCategory>(reply->property("category").toInt());
    QString repository = reply->property("repository").toString();

    if (reply->error() != QNetworkReply::NoError) {
        emit tagsFetchError(category, reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        emit tagsFetchError(category, "Failed to parse Docker Hub response");
        return;
    }

    QVector<DockerImage> images = parseDockerHubTags(doc, repository);
    QVector<DockerImage> stableImages = filterStableTags(images);

    m_availableTags[category] = stableImages;
    emit tagsReceived(category, stableImages);
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
    // Only an image that was NOT there before can be said to belong to OCTOS afterwards.
    m_pullTargetWasInstalled = isImageInstalled(imageName);

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
        if (!m_pullTargetWasInstalled) AppSettings::instance().addOctosPulledImage(m_currentPullImage);
        refreshInstalledImagesAsync();
    }

    if (m_currentPullProcess) {
        m_currentPullProcess->deleteLater();
        m_currentPullProcess = nullptr;
    }
}

void DockerCompilerManager::runDockerAsync(const QStringList& args, int timeoutMs, DockerDone done) {
    auto* proc = new QProcess(this);
    auto* timer = new QTimer(proc);
    timer->setSingleShot(true);
    auto timedOut = std::make_shared<bool>(false);
    auto finishedFlag = std::make_shared<bool>(false);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [proc, done, timedOut, finishedFlag](int exitCode, QProcess::ExitStatus status) {
        if (*finishedFlag) return;
        *finishedFlag = true;
        const int code = (status == QProcess::NormalExit) ? exitCode : -1;
        done(code, QString::fromUtf8(proc->readAllStandardOutput()),
             QString::fromUtf8(proc->readAllStandardError()), *timedOut);
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this,
            [proc, done, finishedFlag](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart || *finishedFlag) return;
        *finishedFlag = true;
        done(-1, QString(), "Could not start the docker command. Is Docker installed and in PATH?", false);
        proc->deleteLater();
    });
    connect(timer, &QTimer::timeout, proc, [proc, timedOut]() {
        *timedOut = true;
        proc->kill();
    });
    timer->start(timeoutMs);
    proc->start("docker", args);
}

void DockerCompilerManager::removeImage(const QString& imageName, bool confirmedForeign) {
    // Only images that look like compiler images can be handled here at all (never e.g. a database image).
    if (!CompilerRegistry::instance().isKnownCompilerImageName(imageName)) {
        emit imageRemoved(imageName, false, "Not a recognised compiler image; OCTOS will not delete it.");
        return;
    }
    // An image OCTOS did not pull may belong to other projects: require the explicit foreign confirmation.
    if (!isOwnedByOctos(imageName) && !confirmedForeign) {
        emit imageRemoved(imageName, false,
                          "This image was not downloaded by OCTOS and may be used by other projects. "
                          "Deletion needs explicit confirmation.");
        return;
    }

    runDockerAsync({"rmi", imageName}, 60000,
                   [this, imageName](int exitCode, const QString&, const QString& err, bool timedOut) {
        const bool success = exitCode == 0;
        QString msg;
        if (success) msg = "Image removed successfully.";
        else if (timedOut) msg = "Removing the image timed out.";
        else msg = "Failed to remove image: " + err.trimmed();   // e.g. "image is being used by a container"
        if (success) AppSettings::instance().removeOctosPulledImage(imageName);
        emit imageRemoved(imageName, success, msg);
        if (success) refreshInstalledImagesAsync();
    });
}

void DockerCompilerManager::refreshInstalledImagesAsync() {
    if (m_refreshInFlight) return;
    m_refreshInFlight = true;

    runDockerAsync({"images", "--format", "{{.Repository}}:{{.Tag}}|{{.ID}}|{{.Size}}"}, 15000,
                   [this](int exitCode, const QString& out, const QString& err, bool timedOut) {
        m_refreshInFlight = false;
        if (exitCode != 0) {
            // A failed query must not look like "no images installed": keep the old list, report the error.
            emit installedImagesRefreshFailed(timedOut ? QString("Querying Docker images timed out.")
                                                       : QString("Could not list Docker images: ") + err.trimmed());
            return;
        }
        QVector<DockerImage> images;
        auto& registry = CompilerRegistry::instance();
        for (const DockerImage& old : std::as_const(m_installedImages)) registry.setInstalledImageId(old.fullName, QString());
        for (const QString& line : out.split('\n', Qt::SkipEmptyParts)) {
            const QStringList parts = line.split('|');
            if (parts.size() != 3) continue;
            const QString fullName = parts[0];
            if (!registry.isKnownCompilerImageName(fullName)) continue;
            DockerImage image;
            const QStringList nameParts = fullName.split(':');
            if (nameParts.size() == 2) { image.registry = nameParts[0]; image.tag = nameParts[1]; }
            image.fullName = fullName;
            image.imageId = parts[1];
            image.isInstalled = true;
            images.append(image);
            registry.setInstalledImageId(fullName, image.imageId);
        }
        m_installedImages = images;
        emit installedImagesRefreshed(m_installedImages);
    });
}

bool DockerCompilerManager::isImageInstalled(const QString& imageName) const {
    for (const auto& img : m_installedImages) {
        if (img.fullName == imageName) return true;
    }
    return false;
}

bool DockerCompilerManager::isOwnedByOctos(const QString& imageName) const {
    return AppSettings::instance().octosPulledImages().contains(imageName);
}

void DockerCompilerManager::persistProbes() {
    AppSettings::instance().setToolchainProbesJson(
        QJsonDocument(CompilerRegistry::instance().exportProbes()).toJson(QJsonDocument::Compact));
}

void DockerCompilerManager::probeToolchain(const QString& imageName) {
    const CompilerDefinition def = CompilerRegistry::instance().compilerForImage(imageName);

    auto fail = [this, imageName](const QString& msg, const QString& digest) {
        ToolchainProbeResult r;
        r.performed = true;
        r.success = false;
        r.checkedAt = QDateTime::currentDateTimeUtc();
        r.imageDigest = digest;
        r.message = msg;
        CompilerRegistry::instance().recordProbeResult(imageName, r);
        persistProbes();
        emit toolchainProbed(imageName, r);
    };

    // Step 1: which image exactly are we about to probe? (never pulls)
    runDockerAsync({"image", "inspect", "--format", "{{.Id}}", imageName}, 15000,
                   [this, imageName, def, fail](int exitCode, const QString& out, const QString& err, bool) {
        if (exitCode != 0) {
            fail("Image is not available locally: " + err.trimmed(), QString());
            return;
        }
        const QString digest = out.trimmed();
        // Step 2: run the probe in a locked-down container.
        runDockerAsync(ToolchainProbe::buildDockerArgs(imageName, def), 60000,
                       [this, imageName, def, digest, fail](int code, const QString& pout, const QString& perr, bool timedOut) {
            if (timedOut) { fail("The probe timed out.", digest); return; }
            ToolchainProbeResult r = ToolchainProbe::parseOutput(def, code, pout, perr, digest);
            CompilerRegistry::instance().recordProbeResult(imageName, r);
            CompilerRegistry::instance().setInstalledImageId(imageName, digest);
            persistProbes();
            emit toolchainProbed(imageName, r);
        });
    });
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

    CompilationOutput legacy;
    legacy.jobId = static_cast<int>(result.jobId);
    legacy.success = result.success();
    legacy.exitCode = result.exitCode;
    legacy.stdout_ = result.assemblyOutput.isEmpty() ? result.rawOutput : result.assemblyOutput;
    legacy.stderr_ = result.errorOutput;

    emit compilationFinished(legacy.jobId, legacy);
}

QString DockerCompilerManager::getCategoryRegistry(CompilerCategory category) const {
    switch (category) {
        case CompilerCategory::Cpp:
        case CompilerCategory::C:
        case CompilerCategory::Ada:
            return "gcc";
        case CompilerCategory::Clang:
            return "silkeh/clang";
        case CompilerCategory::Rust:
            return "rust";
        case CompilerCategory::Java:
            return "eclipse-temurin";
        case CompilerCategory::Python:
            return "python";
        case CompilerCategory::CSharp:
            return "mono";
    }
    return QString();
}

QStringList DockerCompilerManager::getSupportedArchitectures() const {
    return QStringList() << "amd64" << "arm64v8" << "arm32v7";
}

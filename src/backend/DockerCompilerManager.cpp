#include "DockerCompilerManager.h"

#include <QDebug>
#include <QJsonArray>
#include <QList>
#include <QMetaObject>
#include <QMutexLocker>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QRunnable>
#include <QThread>
#include <QUrl>
#include <atomic>
#include <memory>

static QString buildDockerCommand(const CompilationJob& job) {
    const QString tempFile = QString("/tmp/octos_src_%1.tmp").arg(job.jobId);

    QString extension = ".cpp";
    QString compilerCmd = "g++";

    if (job.language == "c") {
        extension = ".c";
        compilerCmd = "gcc";
    } else if (job.language == "rust") {
        extension = ".rs";
        compilerCmd = "rustc";
    } else if (job.language == "java") {
        extension = ".java";
        compilerCmd = "javac";
    } else if (job.language == "python") {
        extension = ".py";
        compilerCmd = "python3 -m py_compile";
    } else if (job.language == "csharp") {
        extension = ".cs";
        compilerCmd = "csc";
    } else if (job.language == "ada") {
        extension = ".adb";
        compilerCmd = "gnatmake";
    }

    QString dockerImage = job.dockerImage;

    if (job.language == "ada" && !dockerImage.contains("gcc")) {
        dockerImage = "gcc:latest";
    }

    if (job.dockerImage.contains("clang")) {
        if (job.language == "cpp" || job.language == "c++") {
            compilerCmd = "clang++";
        } else if (job.language == "c") {
            compilerCmd = "clang";
        }
    }

    QString flags = job.compilerFlags.trimmed();

    if (job.language != "java" && job.language != "python") {
        if (!flags.contains("-S")) {
            flags = flags.isEmpty() ? "-S" : "-S " + flags;
        }
    }

    if (!flags.isEmpty()) {
        flags = flags.trimmed();
    }

    const QString sourceFileName = "source" + extension;

    QString command = QString("cat <<'OCTOS_SRC_%1' > %2\n").arg(job.jobId).arg(tempFile);
    command += job.sourceCode;
    command += "\nOCTOS_SRC_" + QString::number(job.jobId) + "\n";

    if (job.language == "ada") {
        command += "docker run --rm -v " + tempFile + ":/workspace/" + sourceFileName +
                   ":ro -w /workspace " + dockerImage +
                   " bash -c 'apt-get update > /dev/null 2>&1 && apt-get install -y gnat > "
                   "/dev/null 2>&1 && " +
                   compilerCmd + " -c /workspace/" + sourceFileName +
                   " && objdump -d /workspace/source.o' 2>&1\n";
    } else if (job.language == "rust") {
        command += "docker run --rm -v " + tempFile + ":/workspace/" + sourceFileName +
                   ":ro -w /workspace " + job.dockerImage + " bash -c '" + compilerCmd +
                   " --emit asm /workspace/" + sourceFileName +
                   " -o output.s && cat output.s' 2>&1\n";
    } else if (job.language == "python") {
        command += "docker run --rm -v " + tempFile + ":/workspace/" + sourceFileName +
                   ":ro -w /workspace " + job.dockerImage + " " + compilerCmd + " /workspace/" +
                   sourceFileName + " 2>&1\n";
    } else if (job.language == "java") {
        command += "docker run --rm -v " + tempFile + ":/workspace/" + sourceFileName +
                   ":ro -w /workspace " + job.dockerImage + " bash -c '" + compilerCmd +
                   " /workspace/" + sourceFileName +
                   " && javap -c /workspace/$(basename /workspace/" + sourceFileName +
                   " .java).class' 2>&1\n";
    } else {
        command += "docker run --rm -v " + tempFile + ":/workspace/" + sourceFileName +
                   ":ro -w /workspace " + job.dockerImage + " " + compilerCmd;
        if (!flags.isEmpty()) {
            command += " " + flags;
        }
        command += " /workspace/" + sourceFileName + " -o - 2>&1\n";
    }

    command += "rm -f " + tempFile;

    return command;
}

class CompilationTask : public QRunnable {
   public:
    CompilationTask(const CompilationJob& job, DockerCompilerManager* owner)
        : m_job(job), m_owner(owner) {
        setAutoDelete(false);
    }

    void cancel() {
        m_cancelled.store(true, std::memory_order_release);
        QMutexLocker locker(&m_processMutex);
        if (m_process) {
            m_process->kill();
        }
    }

    void run() override {
        CompilationOutput output;
        output.jobId = m_job.jobId;
        output.success = false;
        output.exitCode = -1;

        if (m_cancelled.load(std::memory_order_acquire)) {
            output.stderr_ = "Compilation cancelled";
            dispatchResult(output);
            return;
        }

        {
            QMutexLocker locker(&m_processMutex);
            m_process = std::make_unique<QProcess>();
        }

        QProcess* process = nullptr;
        {
            QMutexLocker locker(&m_processMutex);
            process = m_process.get();
        }

        const QString command = buildDockerCommand(m_job);
        process->start("/bin/sh", QStringList() << "-c" << command);

        if (!process->waitForStarted()) {
            output.stderr_ = "Failed to start Docker process";
            cleanupProcess();
            dispatchResult(output);
            return;
        }

        while (!process->waitForFinished(100)) {
            if (m_cancelled.load(std::memory_order_acquire)) {
                process->kill();
            }
        }

        output.stdout_ = QString::fromUtf8(process->readAllStandardOutput());
        output.stderr_ = QString::fromUtf8(process->readAllStandardError());
        output.exitCode = process->exitCode();
        output.success = (output.exitCode == 0) && !m_cancelled.load(std::memory_order_acquire);

        if (m_cancelled.load(std::memory_order_acquire)) {
            if (output.stderr_.trimmed().isEmpty()) {
                output.stderr_ = "Compilation cancelled";
            }
            output.exitCode = -1;
            output.success = false;
        }

        cleanupProcess();
        dispatchResult(output);
    }

   private:
    void cleanupProcess() {
        QMutexLocker locker(&m_processMutex);
        m_process.reset();
    }

    void dispatchResult(const CompilationOutput& output) {
        auto owner = m_owner;
        auto self = this;
        QMetaObject::invokeMethod(
            owner, [owner, output, self]() { owner->handleTaskFinished(output, self); },
            Qt::QueuedConnection);
    }

    CompilationJob m_job;
    DockerCompilerManager* m_owner;
    std::atomic_bool m_cancelled{false};
    QMutex m_processMutex;
    std::unique_ptr<QProcess> m_process;
};

DockerCompilerManager::DockerCompilerManager(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_currentPullProcess(nullptr),
      m_nextJobId(1) {
    qRegisterMetaType<CompilationOutput>("CompilationOutput");
    m_threadPool.setMaxThreadCount(qMax(2, QThread::idealThreadCount()));
    m_threadPool.setExpiryTimeout(15000);
    refreshInstalledImages();
}

DockerCompilerManager::~DockerCompilerManager() {
    QList<CompilationTask*> tasks;
    {
        QMutexLocker locker(&m_jobMutex);
        tasks = m_activeJobs.values();
        m_activeJobs.clear();
    }

    for (CompilationTask* task : tasks) {
        if (task) {
            task->cancel();
        }
    }

    m_threadPool.waitForDone();

    qDeleteAll(tasks);
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
        default:
            return "";
    }
}

void DockerCompilerManager::fetchAvailableTags(CompilerCategory category) {
    QString registry = getCategoryRegistry(category);
    if (registry.isEmpty()) {
        emit tagsFetchError(category, "Unknown category");
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
        emit tagsFetchError(category, "Invalid JSON response");
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

    QRegularExpression stableRegex(R"(^\d+\.\d+(\.\d+)?(-slim|-alpine)?$)");

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

    if (!m_currentPullProcess->waitForStarted()) {
        emit pullFinished(imageName, false, "Failed to start Docker pull");
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

    if (!success) {
        message += ": " + QString::fromUtf8(m_currentPullProcess->readAllStandardError());
    }

    emit pullFinished(m_currentPullImage, success, message);

    if (success) {
        refreshInstalledImages();
    }

    m_currentPullProcess->deleteLater();
    m_currentPullProcess = nullptr;
}

void DockerCompilerManager::removeImage(const QString& imageName) {
    QProcess process;
    process.start("docker", QStringList() << "rmi" << imageName);
    process.waitForFinished();

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
    process.waitForFinished();

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() != 2) continue;

        QString fullName = parts[0];
        QString sizeStr = parts[1];

        if (fullName.contains("gcc") || fullName.contains("clang") ||
            fullName.contains("openjdk") || fullName.contains("python") ||
            fullName.contains("rust")) {
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
    process.waitForFinished();

    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return !output.isEmpty();
}

int DockerCompilerManager::submitCompilationJob(const QString& sourceCode,
                                                const QString& dockerImage,
                                                const QString& compilerFlags,
                                                const QString& language) {
    int jobId = m_nextJobId++;

    CompilationJob job;
    job.sourceCode = sourceCode;
    job.dockerImage = dockerImage;
    job.compilerFlags = compilerFlags;
    job.language = language;
    job.jobId = jobId;

    auto* task = new CompilationTask(job, this);

    emit compilationProgress(jobId, 0);

    {
        QMutexLocker locker(&m_jobMutex);
        m_activeJobs.insert(jobId, task);
    }

    m_threadPool.start(task);

    return jobId;
}

void DockerCompilerManager::cancelJob(int jobId) {
    CompilationTask* task = nullptr;
    {
        QMutexLocker locker(&m_jobMutex);
        task = m_activeJobs.value(jobId, nullptr);
    }

    if (task) {
        task->cancel();
    }
}

void DockerCompilerManager::handleTaskFinished(const CompilationOutput& output,
                                               CompilationTask* task) {
    {
        QMutexLocker locker(&m_jobMutex);
        if (m_activeJobs.contains(output.jobId) && m_activeJobs[output.jobId] == task) {
            m_activeJobs.remove(output.jobId);
        }
    }

    delete task;

    emit compilationProgress(output.jobId, 100);
    emit compilationFinished(output.jobId, output);
}

QStringList DockerCompilerManager::getSupportedArchitectures() const {
    return QStringList() << "amd64" << "arm64v8" << "arm32v7";
}

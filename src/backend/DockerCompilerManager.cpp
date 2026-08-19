#include "DockerCompilerManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QList>
#include <QMetaObject>
#include <QMutexLocker>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QRunnable>
#include <QThread>
#include <QTemporaryDir>
#include <QUrl>
#include <atomic>
#include <memory>

// Build Docker arguments as a QStringList (no shell involved)
static void buildDockerArguments(const CompilationJob& job, QStringList& dockerArgs, 
                                QTemporaryDir*& tempDir, QString& hostSourcePath) {
    // Create a temporary directory for this job
    tempDir = new QTemporaryDir();
    if (!tempDir->isValid()) {
        delete tempDir;
        tempDir = nullptr;
        return;
    }

    // Write source code to temporary file
    QString extension;
    QString compilerCmd;
    QString sourceFileName;

    if (job.language == "c") {
        extension = ".c";
        compilerCmd = "gcc";
        sourceFileName = "source.c";
    } else if (job.language == "cpp" || job.language == "c++") {
        extension = ".cpp";
        compilerCmd = "g++";
        sourceFileName = "source.cpp";
    } else if (job.language == "rust") {
        extension = ".rs";
        compilerCmd = "rustc";
        sourceFileName = "source.rs";
    } else if (job.language == "java") {
        extension = ".java";
        compilerCmd = "javac";
        sourceFileName = "Source.java";
    } else if (job.language == "python") {
        extension = ".py";
        compilerCmd = "python3";
        sourceFileName = "source.py";
    } else if (job.language == "csharp") {
        extension = ".cs";
        compilerCmd = "csc";
        sourceFileName = "Program.cs";
    } else if (job.language == "ada") {
        extension = ".adb";
        compilerCmd = "gnatmake";
        sourceFileName = "source.adb";
    } else {
        extension = ".cpp";
        compilerCmd = "g++";
        sourceFileName = "source.cpp";
    }

    hostSourcePath = tempDir->path() + "/" + sourceFileName;
    QFile sourceFile(hostSourcePath);
    if (sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&sourceFile);
        out << job.sourceCode;
        sourceFile.close();
    } else {
        delete tempDir;
        tempDir = nullptr;
        return;
    }

    // Handle image selection based on compiler
    QString dockerImage = job.dockerImage;
    if (job.language == "ada" && !dockerImage.contains("gcc")) {
        dockerImage = "gcc:latest";
    }

    // Handle clang image
    if (dockerImage.contains("clang")) {
        if (job.language == "cpp" || job.language == "c++") {
            compilerCmd = "clang++";
        } else if (job.language == "c") {
            compilerCmd = "clang";
        }
    }

    // Container name for cleanup
    QString containerName = QString("octos_job_%1").arg(job.jobId);

    // Start building docker arguments
    dockerArgs << "run";
    dockerArgs << "--rm";
    dockerArgs << "--name";
    dockerArgs << containerName;

    // Network isolation - default to none for security
    bool useNetworkNone = true;
    if (job.language == "csharp") {
        useNetworkNone = false;
    }
    
    if (useNetworkNone) {
        dockerArgs << "--network";
        dockerArgs << "none";
    }

    // Mount the temporary directory as read-only
    dockerArgs << "-v";
    dockerArgs << (tempDir->path() + ":/workspace:ro");
    dockerArgs << "-w";
    dockerArgs << "/workspace";
    dockerArgs << dockerImage;

    // Parse and add compiler flags
    QString flags = job.compilerFlags.trimmed();
    QStringList flagList;
    if (!flags.isEmpty()) {
        flagList = flags.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
    }

    // Language-specific handling
    if (job.language == "java") {
        // Java: compile then disassemble using bash inside container
        dockerArgs << "bash";
        dockerArgs << "-c";
        QString javaCmd = "javac ";
        for (const QString& flag : flagList) {
            // Escape any special characters in flags for the bash -c command
            // Since this is inside the container, we control the environment
            javaCmd += " " + flag;
        }
        javaCmd += " /workspace/Source.java && javap -c -v /workspace/Source 2>&1";
        dockerArgs << javaCmd;
    } else if (job.language == "python") {
        // Python: syntax checking
        dockerArgs << "python3";
        dockerArgs << "-m";
        dockerArgs << "py_compile";
        dockerArgs << ("/workspace/" + sourceFileName);
    } else if (job.language == "csharp") {
        dockerArgs << compilerCmd;
        for (const QString& flag : flagList) {
            dockerArgs << flag;
        }
        dockerArgs << ("/workspace/" + sourceFileName);
    } else if (job.language == "ada") {
        dockerArgs << compilerCmd;
        for (const QString& flag : flagList) {
            dockerArgs << flag;
        }
        dockerArgs << ("/workspace/" + sourceFileName);
    } else {
        // C/C++/Rust: compile to assembly
        dockerArgs << compilerCmd;
        
        // Add -S flag if not present in user flags
        bool hasS = false;
        for (const QString& flag : flagList) {
            if (flag == "-S") {
                hasS = true;
                break;
            }
        }
        if (!hasS) {
            dockerArgs << "-S";
        }
        
        // Add -fno-asynchronous-unwind-tables for cleaner output
        dockerArgs << "-fno-asynchronous-unwind-tables";
        
        // Add user flags
        for (const QString& flag : flagList) {
            dockerArgs << flag;
        }
        
        dockerArgs << ("/workspace/" + sourceFileName);
        dockerArgs << "-o";
        dockerArgs << "/workspace/output.s";
    }
}

class CompilationTask : public QRunnable {
   public:
    CompilationTask(const CompilationJob& job, DockerCompilerManager* owner)
        : m_job(job), m_owner(owner), m_tempDir(nullptr) {
        setAutoDelete(false);
    }

    ~CompilationTask() {
        if (m_tempDir) {
            delete m_tempDir;
        }
    }

    void cancel() {
        m_cancelled.store(true, std::memory_order_release);
        QMutexLocker locker(&m_processMutex);
        if (m_process) {
            m_process->kill();
            // Clean up docker container
            QString containerName = QString("octos_job_%1").arg(m_job.jobId);
            QProcess::execute("docker", QStringList() << "kill" << containerName);
            QProcess::execute("docker", QStringList() << "rm" << "-f" << containerName);
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

        QStringList dockerArgs;
        QString hostSourcePath;
        
        buildDockerArguments(m_job, dockerArgs, m_tempDir, hostSourcePath);
        
        if (!m_tempDir) {
            output.stderr_ = "Failed to create temporary workspace";
            dispatchResult(output);
            return;
        }

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

        // Execute docker directly with argument list - NO SHELL
        process->start("docker", dockerArgs);

        if (!process->waitForStarted()) {
            output.stderr_ = "Failed to start Docker process";
            cleanupProcess();
            dispatchResult(output);
            return;
        }

        // Wait with timeout (30 seconds)
        const int timeoutSeconds = 30;
        const int intervalMs = 100;
        int elapsedMs = 0;
        
        while (!process->waitForFinished(intervalMs)) {
            elapsedMs += intervalMs;
            if (elapsedMs >= timeoutSeconds * 1000) {
                // Timeout
                process->kill();
                
                // Clean up container
                QString containerName = QString("octos_job_%1").arg(m_job.jobId);
                QProcess::execute("docker", QStringList() << "kill" << containerName);
                QProcess::execute("docker", QStringList() << "rm" << "-f" << containerName);
                
                output.stderr_ = "Compilation timed out after " + QString::number(timeoutSeconds) + " seconds";
                output.exitCode = -2;
                cleanupProcess();
                dispatchResult(output);
                return;
            }
            
            if (m_cancelled.load(std::memory_order_acquire)) {
                process->kill();
                
                // Clean up container
                QString containerName = QString("octos_job_%1").arg(m_job.jobId);
                QProcess::execute("docker", QStringList() << "kill" << containerName);
                QProcess::execute("docker", QStringList() << "rm" << "-f" << containerName);
                
                output.stderr_ = "Compilation cancelled";
                output.exitCode = -1;
                cleanupProcess();
                dispatchResult(output);
                return;
            }
        }

        // Read output
        output.stdout_ = QString::fromUtf8(process->readAllStandardOutput());
        output.stderr_ = QString::fromUtf8(process->readAllStandardError());
        output.exitCode = process->exitCode();
        output.success = (output.exitCode == 0) && !m_cancelled.load(std::memory_order_acquire);

        // For C/C++/Rust, read the output file if docker command succeeded
        if ((m_job.language == "c" || m_job.language == "cpp" || m_job.language == "c++" || 
             m_job.language == "rust") && output.stdout_.isEmpty() && output.exitCode == 0) {
            QString outputFilePath = m_tempDir->path() + "/output.s";
            QFile outputFile(outputFilePath);
            if (outputFile.exists() && outputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                output.stdout_ = QString::fromUtf8(outputFile.readAll());
                outputFile.close();
            }
        }

        if (m_cancelled.load(std::memory_order_acquire)) {
            if (output.stderr_.trimmed().isEmpty()) {
                output.stderr_ = "Compilation cancelled";
            }
            output.exitCode = -1;
            output.success = false;
        }

        // Clean up container
        QString containerName = QString("octos_job_%1").arg(m_job.jobId);
        QProcess::execute("docker", QStringList() << "rm" << "-f" << containerName);
        
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
    QTemporaryDir* m_tempDir;
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

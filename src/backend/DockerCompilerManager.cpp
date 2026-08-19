#include "DockerCompilerManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QList>
#include <QMetaObject>
#include <QMutexLocker>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QRunnable>
#include <QThread>
#include <QTextStream>
#include <QTemporaryDir>
#include <QUrl>
#include <QUuid>
#include <atomic>
#include <memory>

// Timeout constants
constexpr int kDockerStartTimeoutMs = 5000;    // 5 seconds to start Docker process
constexpr int kCompilationTimeoutMs = 30000;   // 30 seconds for compilation
constexpr int kCleanupTimeoutMs = 10000;       // 10 seconds for cleanup operations

namespace {

// Helper function to detect public class name in Java source
QString detectJavaPublicClass(const QString& sourceCode) {
    // Match patterns like: public class Name, public final class Name, etc.
    static const QRegularExpression javaClassRegex(
        R"(\bpublic\s+(?:final\s+|abstract\s+|strictfp\s+)?class\s+([A-Za-z_][A-Za-z0-9_]*)\s*)"
    );
    
    QRegularExpressionMatch match = javaClassRegex.match(sourceCode);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    
    return QString();
}

// Helper function to split compiler arguments correctly
QStringList splitCompilerArguments(const QString& flags) {
    QStringList result;
    QString currentArg;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool escaped = false;
    
    for (int i = 0; i < flags.size(); ++i) {
        QChar c = flags[i];
        
        if (escaped) {
            currentArg += c;
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            currentArg += c;
            continue;
        }
        
        if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            continue;
        }
        
        if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        
        if ((c == ' ' || c == '\t' || c == '\n' || c == '\r') && 
            !inSingleQuote && !inDoubleQuote) {
            if (!currentArg.isEmpty()) {
                result.append(currentArg);
                currentArg.clear();
            }
            continue;
        }
        
        currentArg += c;
    }
    
    if (!currentArg.isEmpty()) {
        result.append(currentArg);
    }
    
    return result;
}

// Validate and sanitize user flags to prevent overriding OCTOS output
QStringList sanitizeUserFlags(const QStringList& userFlags) {
    QStringList sanitized;
    bool skipNext = false;
    
    for (int i = 0; i < userFlags.size(); ++i) {
        const QString& flag = userFlags[i];
        
        // Skip this argument if we're skipping the next one
        if (skipNext) {
            skipNext = false;
            continue;
        }
        
        // Check for output override attempts
        if (flag == "-o" || flag == "--output") {
            // Skip this flag and the next argument (the output path)
            skipNext = true;
            continue;
        }
        
        // Check for flags that start with the output flag
        if (flag.startsWith("-o=") || flag.startsWith("--output=")) {
            continue;
        }
        
        // Check for other potentially dangerous patterns
        if (flag.contains(";") || flag.contains("|") || flag.contains("&") ||
            flag.contains("$") || flag.contains("`") || flag.contains("(") ||
            flag.contains(")") || flag.contains("{") || flag.contains("}")) {
            continue;
        }
        
        sanitized.append(flag);
    }
    
    return sanitized;
}

// Build Docker arguments using the new plan-based approach
void buildDockerArguments(const CompilationJob& job, QStringList& dockerArgs,
                          QTemporaryDir*& inputDir, QTemporaryDir*& outputDir,
                          QString& hostSourcePath, QString& hostOutputPath) {
    // Create separate directories for input and output
    inputDir = new QTemporaryDir();
    outputDir = new QTemporaryDir();
    
    if (!inputDir->isValid() || !outputDir->isValid()) {
        delete inputDir;
        delete outputDir;
        inputDir = nullptr;
        outputDir = nullptr;
        return;
    }
    
    // Determine file extension and source filename based on language
    QString extension;
    QString sourceFileName;
    
    if (job.language == "c") {
        extension = ".c";
        sourceFileName = "source.c";
    } else if (job.language == "cpp" || job.language == "c++") {
        extension = ".cpp";
        sourceFileName = "source.cpp";
    } else if (job.language == "rust") {
        extension = ".rs";
        sourceFileName = "source.rs";
    } else if (job.language == "java") {
        // For Java, try to detect the public class name
        QString publicClass = detectJavaPublicClass(job.sourceCode);
        if (publicClass.isEmpty()) {
            publicClass = "Main";
        }
        extension = ".java";
        sourceFileName = publicClass + ".java";
    } else if (job.language == "python") {
        extension = ".py";
        sourceFileName = "source.py";
    } else if (job.language == "csharp") {
        extension = ".cs";
        sourceFileName = "Program.cs";
    } else if (job.language == "ada") {
        extension = ".adb";
        sourceFileName = "source.adb";
    } else {
        extension = ".cpp";
        sourceFileName = "source.cpp";
    }
    
    // Write source code to input directory
    hostSourcePath = inputDir->path() + "/" + sourceFileName;
    QFile sourceFile(hostSourcePath);
    if (sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&sourceFile);
        out << job.sourceCode;
        sourceFile.close();
    } else {
        delete inputDir;
        delete outputDir;
        inputDir = nullptr;
        outputDir = nullptr;
        return;
    }
    
    hostOutputPath = outputDir->path();
    
    // Handle image selection
    QString dockerImage = job.dockerImage;
    
    // Determine compiler command based on language and image
    QString compilerCmd;
    if (job.language == "c") {
        compilerCmd = dockerImage.contains("clang") ? "clang" : "gcc";
    } else if (job.language == "cpp" || job.language == "c++") {
        compilerCmd = dockerImage.contains("clang") ? "clang++" : "g++";
    } else if (job.language == "rust") {
        compilerCmd = "rustc";
    } else if (job.language == "java") {
        compilerCmd = "bash";
    } else if (job.language == "python") {
        compilerCmd = "python3";
    } else if (job.language == "csharp") {
        compilerCmd = "csc";
    } else if (job.language == "ada") {
        compilerCmd = "gnatmake";
    } else {
        compilerCmd = "g++";
    }
    
    // Container name with PID for uniqueness
    QString containerName = QString("octos_%1_%2").arg(QCoreApplication::applicationPid()).arg(job.jobId);
    
    // Start building docker arguments
    dockerArgs << "run";
    dockerArgs << "--rm";
    dockerArgs << "--name";
    dockerArgs << containerName;
    
    // Network isolation - default to none for security
    // Only C# might need network, but we'll be conservative
    dockerArgs << "--network";
    dockerArgs << "none";
    
    // Mount input directory as read-only
    dockerArgs << "-v";
    dockerArgs << (inputDir->path() + ":/input:ro");
    
    // Mount output directory as writable
    dockerArgs << "-v";
    dockerArgs << (outputDir->path() + ":/output");
    
    // Set working directory to /output for the compiler
    dockerArgs << "-w";
    dockerArgs << "/output";
    
    dockerArgs << dockerImage;
    
    // Parse and sanitize compiler flags
    QStringList userFlags = splitCompilerArguments(job.compilerFlags.trimmed());
    userFlags = sanitizeUserFlags(userFlags);
    
    // Language-specific handling
    if (job.language == "java") {
        // Java is Experimental - we do simple compilation without user flags
        // to avoid shell injection. User flags are intentionally ignored for security.
        QString publicClass = detectJavaPublicClass(job.sourceCode);
        if (publicClass.isEmpty()) {
            publicClass = "Main";
        }
        
        // Use a simple, safe javac + javap command without user flags
        QString compileCmd = QString("javac /input/%1 -d /output && javap -c -v /output/%1.class").
            arg(publicClass);
        
        dockerArgs << "bash";
        dockerArgs << "-c";
        dockerArgs << compileCmd;
        
    } else if (job.language == "python") {
        // Python: syntax checking - no user flags passed through
        dockerArgs << "python3";
        dockerArgs << "-m";
        dockerArgs << "py_compile";
        dockerArgs << ("/input/" + sourceFileName);
        
    } else if (job.language == "csharp") {
        // C# is Experimental - ignore user flags for security
        dockerArgs << compilerCmd;
        dockerArgs << ("/input/" + sourceFileName);
        
    } else if (job.language == "ada") {
        // Ada is Planned - ignore user flags for security
        dockerArgs << compilerCmd;
        dockerArgs << ("/input/" + sourceFileName);
        
    } else {
        // C/C++/Rust: compile to assembly
        dockerArgs << compilerCmd;
        
        if (job.language == "rust") {
            // Rust specific flags
            dockerArgs << "--emit";
            dockerArgs << "asm";
            // Note: Rust doesn't support -fno-asynchronous-unwind-tables
        } else {
            // C/C++ specific flags
            dockerArgs << "-S";
            dockerArgs << "-fno-asynchronous-unwind-tables";
        }
        
        // Add user flags
        dockerArgs.append(userFlags);
        
        dockerArgs << ("/input/" + sourceFileName);
        dockerArgs << "-o";
        dockerArgs << "/output/output.s";
    }
}

// Clean up Docker container
void cleanupContainer(const QString& containerName) {
    QProcess::execute("docker", QStringList() << "kill" << containerName);
    QProcess::execute("docker", QStringList() << "rm" << "-f" << containerName);
}

} // namespace

class CompilationTask : public QRunnable {
   public:
    CompilationTask(const CompilationJob& job, DockerCompilerManager* owner)
        : m_job(job), m_owner(owner), m_inputDir(nullptr), m_outputDir(nullptr) {
        setAutoDelete(false);
    }

    ~CompilationTask() {
        delete m_inputDir;
        delete m_outputDir;
    }

    void cancel() {
        m_cancelled.store(true, std::memory_order_release);
        QMutexLocker locker(&m_processMutex);
        if (m_process) {
            m_process->kill();
            // Clean up docker container
            QString containerName = QString("octos_%1_%2").arg(
                QCoreApplication::applicationPid()).arg(m_job.jobId);
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
        QString hostOutputPath;
        
        buildDockerArguments(m_job, dockerArgs, m_inputDir, m_outputDir, hostSourcePath, hostOutputPath);
        
        if (!m_inputDir || !m_outputDir) {
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

        if (!process->waitForStarted(kDockerStartTimeoutMs)) {
            output.stderr_ = "Docker process could not be started.";
            cleanupProcess();
            dispatchResult(output);
            return;
        }

        // Wait with timeout
        if (!process->waitForFinished(kCompilationTimeoutMs)) {
            // Timeout
            process->kill();
            
            // Clean up container
            QString containerName = QString("octos_%1_%2").arg(
                QCoreApplication::applicationPid()).arg(m_job.jobId);
            cleanupContainer(containerName);
            
            output.stderr_ = "Compilation timed out after " + 
                QString::number(kCompilationTimeoutMs / 1000) + " seconds";
            output.exitCode = -2;
            cleanupProcess();
            dispatchResult(output);
            return;
        }
        
        if (m_cancelled.load(std::memory_order_acquire)) {
            // Clean up container
            QString containerName = QString("octos_%1_%2").arg(
                QCoreApplication::applicationPid()).arg(m_job.jobId);
            cleanupContainer(containerName);
            
            process->kill();
            output.stderr_ = "Compilation cancelled";
            output.exitCode = -1;
            output.success = false;
            cleanupProcess();
            dispatchResult(output);
            return;
        }

        // Read output
        output.stdout_ = QString::fromUtf8(process->readAllStandardOutput());
        output.stderr_ = QString::fromUtf8(process->readAllStandardError());
        output.exitCode = process->exitCode();
        output.success = (output.exitCode == 0) && !m_cancelled.load(std::memory_order_acquire);

        // Clean up container
        QString containerName = QString("octos_%1_%2").arg(
            QCoreApplication::applicationPid()).arg(m_job.jobId);
        cleanupContainer(containerName);

        // For C/C++/Rust, read the output file if compilation succeeded
        if ((m_job.language == "c" || m_job.language == "cpp" || m_job.language == "c++" || 
             m_job.language == "rust") && output.exitCode == 0) {
            QString outputFilePath = m_outputDir->path() + "/output.s";
            QFile outputFile(outputFilePath);
            if (outputFile.exists() && outputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                output.stdout_ = QString::fromUtf8(outputFile.readAll());
                outputFile.close();
            } else if (output.stdout_.isEmpty() && !output.stderr_.isEmpty()) {
                // If we didn't get the output file but there's stderr, use that
                // This handles cases where the compiler output goes to stderr
                output.stdout_ = output.stderr_;
                output.stderr_.clear();
            }
        } else if (m_job.language == "java" && output.exitCode == 0) {
            // For Java, the output should be from javap
            // If stdout is empty but we had success, try to read any output files
            if (output.stdout_.isEmpty()) {
                // Try to find .class files and disassemble them
                QDir outputDir(m_outputDir->path());
                QStringList classFiles = outputDir.entryList(QStringList() << "*.class");
                for (const QString& classFile : classFiles) {
                    QString baseName = classFile.left(classFile.length() - 6); // Remove .class
                    QString disassembleCmd = QString("javap -c -v /output/%1").arg(classFile);
                    // Note: We can't run another docker command here easily
                    // This is handled in the docker command itself
                }
            }
        } else if (m_job.language == "python") {
            // For Python, if exit code is 0, it means syntax validation succeeded
            if (output.exitCode == 0) {
                if (output.stdout_.isEmpty()) {
                    output.stdout_ = "Python syntax validation succeeded.\n\nOCTOS currently does not generate native assembly for Python.";
                }
            } else {
                // Python syntax error - show the actual error
                if (output.stderr_.isEmpty() && !output.stdout_.isEmpty()) {
                    output.stderr_ = output.stdout_;
                    output.stdout_.clear();
                }
            }
        } else if (m_job.language == "csharp" || m_job.language == "ada") {
            // For C# and Ada, if we get output, use it
            // These are experimental/planned
            if (output.stdout_.isEmpty() && !output.stderr_.isEmpty()) {
                output.stdout_ = output.stderr_;
                output.stderr_.clear();
            }
        }

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
    QTemporaryDir* m_inputDir;
    QTemporaryDir* m_outputDir;
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
        case CompilerCategory::Clang:
            return "library/gcc";
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

    if (!m_currentPullProcess->waitForStarted(kDockerStartTimeoutMs)) {
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
    process.waitForFinished(kCleanupTimeoutMs);

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() != 2) continue;

        QString fullName = parts[0];
        QString sizeStr = parts[1];

        if (fullName.contains("gcc") || fullName.contains("clang") ||
            fullName.contains("openjdk") || fullName.contains("python") ||
            fullName.contains("rust") || fullName.contains("mono")) {
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
    process.waitForFinished(kCleanupTimeoutMs);

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

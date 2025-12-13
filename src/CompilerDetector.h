# 1 "./src/CompilerDetector.h"
#ifndef COMPILERDETECTOR_H
#define COMPILERDETECTOR_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>

struct CompilerInfo {
    QString name;
    QString path;
    QString version;
    QString target;
    bool installed;
    QString installCommand;
    QString displayName() const { return QString("%1 (%2) - %3").arg(name, version, target); }
};

class CompilerDetectorWorker : public QObject {
    Q_OBJECT

   public:
    CompilerDetectorWorker();

    void detectAllCompilers();

   signals:
    void detectionsComplete(const QList<CompilerInfo> &allCompilers);
    void detectionProgress(const QString &message);

   private:
    QList<CompilerInfo> detectGCC();
    QList<CompilerInfo> detectClang();
    QList<CompilerInfo> detectARM();
    QList<CompilerInfo> detectSpark();

    QStringList findCompilersInPath(const QString &pattern);
    QString getCompilerVersion(const QString &compilerPath);
    CompilerInfo getCompilerInfo(const QString &compilerPath);
    bool isCompilerAvailable(const QString &compilerPath);
    QString detectLinuxDistro();

    friend class CompilerDetector;
};

class CompilerDetector : public QObject {
    Q_OBJECT

   public:
    CompilerDetector(QObject *parent = nullptr);
    ~CompilerDetector();

    void detectAsyncronously();

    QList<CompilerInfo> detectGCC();
    QList<CompilerInfo> detectClang();
    QList<CompilerInfo> detectARM();
    QList<CompilerInfo> detectSpark();

    QString getInstallCommand(const QString &compilerName, const QString &distro);

    bool isCompilerAvailable(const QString &compilerPath);

    QString detectLinuxDistro();

    CompilerInfo getCompilerInfo(const QString &compilerPath);

    QList<CompilerInfo> getAllDetectedCompilers() const;

   signals:
    void detectionsComplete(const QList<CompilerInfo> &allCompilers);
    void detectionProgress(const QString &message);

   private slots:
    void onDetectionsComplete(const QList<CompilerInfo> &allCompilers);

   private:
    QThread *m_detectorThread;
    CompilerDetectorWorker *m_worker;
    QList<CompilerInfo> m_allDetectedCompilers;
};

#endif

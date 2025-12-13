# 1 "./src/ui/AsyncCompiler.cpp"
#include "AsyncCompiler.h"

AsyncCompiler::AsyncCompiler(QObject *parent)
    : QObject(parent), m_workerThread(nullptr), m_worker(nullptr) {
    m_workerThread = new QThread(this);
    m_worker = new CompileWorker();
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &CompileWorker::compilationFinished, this,
            &AsyncCompiler::compilationFinished);

    m_workerThread->start();
}

AsyncCompiler::~AsyncCompiler() {
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void AsyncCompiler::compileAsync(const QString &source, const QString &compiler,
                                 const QString &flags, ILanguageStrategy *strategy) {
    QMetaObject::invokeMethod(m_worker, "compile", Qt::QueuedConnection, Q_ARG(QString, source),
                              Q_ARG(QString, compiler), Q_ARG(QString, flags),
                              Q_ARG(ILanguageStrategy *, strategy));
}

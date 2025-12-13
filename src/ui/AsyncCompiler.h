# 1 "./src/ui/AsyncCompiler.h"
#ifndef ASYNCCOMPILER_H
#define ASYNCCOMPILER_H

#include <QObject>
#include <QThread>
#include <memory>

#include "ILanguageStrategy.h"

class CompileWorker : public QObject {
    Q_OBJECT

   public:
    CompileWorker() = default;

   public slots:
    void compile(const QString &source, const QString &compiler, const QString &flags,
                 ILanguageStrategy *strategy) {
        if (!strategy) {
            CompilationResult result;
            result.success = false;
            result.primaryOutput = "No strategy";
            result.secondaryOutput = "";
            result.lineMapping = QMap<int, int>();
            emit compilationFinished(result);
            return;
        }

        CompilationResult result = strategy->compile(source, compiler, flags);
        emit compilationFinished(result);
    }

   signals:
    void compilationFinished(const CompilationResult &result);
};

class AsyncCompiler : public QObject {
    Q_OBJECT

   public:
    explicit AsyncCompiler(QObject *parent = nullptr);
    ~AsyncCompiler();

    void compileAsync(const QString &source, const QString &compiler, const QString &flags,
                      ILanguageStrategy *strategy);

   signals:
    void compilationFinished(const CompilationResult &result);

   private:
    QThread *m_workerThread;
    CompileWorker *m_worker;
};

#endif

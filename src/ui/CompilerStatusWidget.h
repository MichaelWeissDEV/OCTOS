# 1 "./src/ui/CompilerStatusWidget.h"
#ifndef COMPILERSTATUSWIDGET_H
#define COMPILERSTATUSWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

#include "PackageManager.h"

class CompilerStatusWidget : public QWidget {
    Q_OBJECT

   public:
    explicit CompilerStatusWidget(QWidget *parent = nullptr);

    void updateCompilerStatus();
    void updatePythonVersion();
    void updateRustVersion();

   private:
    QLabel *m_cppStatusLabel;
    QLabel *m_pythonStatusLabel;
    QLabel *m_rustStatusLabel;
    QLabel *m_javaStatusLabel;

    QString checkCompiler(const QString &command);
};

#endif

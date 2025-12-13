# 1 "./src/ui/CompilerStatusWidget.cpp"
#include "CompilerStatusWidget.h"

#include <QProcess>
#include <QTimer>

CompilerStatusWidget::CompilerStatusWidget(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);

    m_cppStatusLabel = new QLabel("C++: ");
    m_pythonStatusLabel = new QLabel("Python: ");
    m_rustStatusLabel = new QLabel("Rust: ");
    m_javaStatusLabel = new QLabel("Java: ");

    layout->addWidget(m_cppStatusLabel);
    layout->addWidget(m_pythonStatusLabel);
    layout->addWidget(m_rustStatusLabel);
    layout->addWidget(m_javaStatusLabel);
    layout->addStretch();

    setLayout(layout);

    updateCompilerStatus();

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CompilerStatusWidget::updateCompilerStatus);
    timer->start(5000);
}

void CompilerStatusWidget::updateCompilerStatus() {
    QString cppStatus = checkCompiler("g++ --version");
    m_cppStatusLabel->setText(cppStatus.isEmpty() ? "C++: " : "C++: ");

    updatePythonVersion();

    updateRustVersion();

    QString javaStatus = checkCompiler("java -version");
    m_javaStatusLabel->setText(javaStatus.isEmpty() ? "Java: " : "Java: ");
}

void CompilerStatusWidget::updatePythonVersion() {
    QString pythonVersion = checkCompiler("python3 --version");
    if (pythonVersion.isEmpty()) {
        m_pythonStatusLabel->setText("Python: ");
    } else {
        m_pythonStatusLabel->setText("Python: ");
        m_pythonStatusLabel->setToolTip(pythonVersion);
    }
}

void CompilerStatusWidget::updateRustVersion() {
    QString rustVersion = checkCompiler("rustc --version");
    if (rustVersion.isEmpty()) {
        m_rustStatusLabel->setText("Rust: ");
    } else {
        m_rustStatusLabel->setText("Rust: ");
        m_rustStatusLabel->setToolTip(rustVersion);
    }
}

QString CompilerStatusWidget::checkCompiler(const QString& command) {
    QProcess process;
    process.start("sh", QStringList() << "-c" << command);

    if (!process.waitForFinished(2000)) {
        process.kill();
        return "";
    }

    if (process.exitCode() != 0) {
        return "";
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        output = QString::fromUtf8(process.readAllStandardError()).trimmed();
    }

    return output;
}

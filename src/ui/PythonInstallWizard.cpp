# 1 "./src/ui/PythonInstallWizard.cpp"
#include "PythonInstallWizard.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>

PythonInstallWizard::PythonInstallWizard(const PackageManager &packageManager, QWidget *parent)
    : QWizard(parent), m_packageManager(packageManager) {
    setWindowTitle("Python Installation Wizard");
    setWindowModality(Qt::WindowModal);
    setMinimumWidth(500);
    setMinimumHeight(400);

    addPage(createIntroPage());
    addPage(createVersionPage());
    addPage(createPackagesPage());
    addPage(createSummaryPage());

    connect(this, QOverload<int>::of(&QWizard::finished), this, [this](int result) {
        if (result == QDialog::Accepted) {
            onFinished();
        }
    });
}

QWizardPage *PythonInstallWizard::createIntroPage() {
    QWizardPage *page = new QWizardPage;
    page->setTitle("Python Installation");
    page->setSubTitle("This wizard will help you install Python and useful development packages");

    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *label = new QLabel(
        "OCTOS supports Python compilation and analysis.\n\n"
        "This wizard will guide you through:\n"
        "• Selecting a Python version\n"
        "• Installing useful packages (NumPy, Pandas, Numba, etc.)\n"
        "• Configuring your Python environment\n\n"
        "Click 'Next' to continue.");
    label->setWordWrap(true);
    layout->addWidget(label);
    layout->addStretch();

    return page;
}

QWizardPage *PythonInstallWizard::createVersionPage() {
    QWizardPage *page = new QWizardPage;
    page->setTitle("Select Python Version");
    page->setSubTitle("Choose which version of Python to install");

    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *label = new QLabel("Python Version:");
    m_pythonVersionCombo = new QComboBox;
    m_pythonVersionCombo->addItems({"Python 3.11", "Python 3.10", "Python 3.9", "Python 3.8"});

    layout->addWidget(label);
    layout->addWidget(m_pythonVersionCombo);
    layout->addStretch();

    return page;
}

QWizardPage *PythonInstallWizard::createPackagesPage() {
    QWizardPage *page = new QWizardPage;
    page->setTitle("Select Packages");
    page->setSubTitle("Choose optional packages to install with Python");

    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *label = new QLabel("Package Selection:");
    layout->addWidget(label);

    m_jupyterCheckbox = new QCheckBox("Jupyter Notebook (interactive Python)");
    m_numpyCheckbox = new QCheckBox("NumPy (numerical computing)");
    m_numpyCheckbox->setChecked(true);

    m_pandasCheckbox = new QCheckBox("Pandas (data analysis)");
    m_pandasCheckbox->setChecked(true);

    m_matplotlibCheckbox = new QCheckBox("Matplotlib (plotting)");
    m_matplotlibCheckbox->setChecked(true);

    m_scipyCheckbox = new QCheckBox("SciPy (scientific computing)");
    m_numbaCheckbox = new QCheckBox("Numba (JIT compilation for Python)");
    m_numbaCheckbox->setChecked(true);

    layout->addWidget(m_jupyterCheckbox);
    layout->addWidget(m_numpyCheckbox);
    layout->addWidget(m_pandasCheckbox);
    layout->addWidget(m_matplotlibCheckbox);
    layout->addWidget(m_scipyCheckbox);
    layout->addWidget(m_numbaCheckbox);
    layout->addStretch();

    return page;
}

QWizardPage *PythonInstallWizard::createSummaryPage() {
    QWizardPage *page = new QWizardPage;
    page->setTitle("Installation Summary");
    page->setSubTitle("Review your selections before installation");

    QVBoxLayout *layout = new QVBoxLayout(page);

    m_summaryLabel = new QLabel;
    m_summaryLabel->setWordWrap(true);
    layout->addWidget(m_summaryLabel);
    layout->addStretch();

    connect(this, QOverload<int>::of(&QWizard::currentIdChanged), this, [this]() {
        if (currentPage() == qobject_cast<QWizardPage *>(sender()) ||
            (currentPage() && currentPage()->title() == "Installation Summary")) {
            QString summary = "<b>Installation Summary:</b><br><br>";

            if (m_pythonVersionCombo) {
                summary += "<b>Version:</b> " + m_pythonVersionCombo->currentText() + "<br><br>";
            }

            summary += "<b>Packages to install:</b><br>";
            if (m_jupyterCheckbox && m_jupyterCheckbox->isChecked())
                summary += "• Jupyter Notebook<br>";
            if (m_numpyCheckbox && m_numpyCheckbox->isChecked()) summary += "• NumPy<br>";
            if (m_pandasCheckbox && m_pandasCheckbox->isChecked()) summary += "• Pandas<br>";
            if (m_matplotlibCheckbox && m_matplotlibCheckbox->isChecked())
                summary += "• Matplotlib<br>";
            if (m_scipyCheckbox && m_scipyCheckbox->isChecked()) summary += "• SciPy<br>";
            if (m_numbaCheckbox && m_numbaCheckbox->isChecked()) summary += "• Numba<br>";

            summary += "<br><b>Package Manager:</b> " + m_packageManager.getPackageManagerName();

            m_summaryLabel->setText(summary);
        }
    });

    return page;
}

void PythonInstallWizard::onFinished() {
    QStringList packagesToInstall;

    QString pythonVersion = m_pythonVersionCombo->currentText();
    QString pythonPackage = "python3";
    if (pythonVersion.contains("3.11"))
        pythonPackage = "python3.11";
    else if (pythonVersion.contains("3.10"))
        pythonPackage = "python3.10";
    else if (pythonVersion.contains("3.9"))
        pythonPackage = "python3.9";
    else if (pythonVersion.contains("3.8"))
        pythonPackage = "python3.8";

    packagesToInstall.append(pythonPackage);
    packagesToInstall.append("python3-pip");

    if (m_jupyterCheckbox->isChecked()) packagesToInstall.append("jupyter");
    if (m_numbaCheckbox->isChecked()) packagesToInstall.append("python3-numba");

    QStringList pipPackages;
    if (m_numpyCheckbox->isChecked()) pipPackages.append("numpy");
    if (m_pandasCheckbox->isChecked()) pipPackages.append("pandas");
    if (m_matplotlibCheckbox->isChecked()) pipPackages.append("matplotlib");
    if (m_scipyCheckbox->isChecked()) pipPackages.append("scipy");

    QProgressDialog progress("Installing Python and packages...", "Cancel", 0, 0, parentWidget());
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    QApplication::processEvents();

    bool success = m_packageManager.installPackages(packagesToInstall);

    if (success && !pipPackages.isEmpty()) {
        QProcess pipProcess;
        QString pipCmd = pythonPackage + " -m pip install " + pipPackages.join(" ");
        pipProcess.start("sh", QStringList() << "-c" << pipCmd);
        success = pipProcess.waitForFinished(60000) && pipProcess.exitCode() == 0;
    }

    progress.close();

    if (success) {
        QMessageBox::information(this, "Success",
                                 "Python and selected packages have been installed successfully!");
    } else {
        QMessageBox::warning(this, "Installation Failed",
                             "There was an error installing Python. Please check your system.");
    }
}

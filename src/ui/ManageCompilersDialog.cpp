# 1 "./src/ui/ManageCompilersDialog.cpp"
#include "ManageCompilersDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

ManageCompilersDialog::ManageCompilersDialog(const PackageManager &packageManager, QWidget *parent)
    : QDialog(parent), m_packageManager(packageManager) {
    setWindowTitle("Manage Compilers");
    setMinimumWidth(500);
    setMinimumHeight(600);

    setupUI();
    loadAvailableCompilers();
}

void ManageCompilersDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);

    QLabel *titleLabel = new QLabel("Select compilers to install:");
    m_mainLayout->addWidget(titleLabel);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);

    QWidget *scrollWidget = new QWidget();
    m_compilerListLayout = new QVBoxLayout(scrollWidget);
    m_compilerListLayout->setSpacing(8);

    scrollArea->setWidget(scrollWidget);
    m_mainLayout->addWidget(scrollArea);

    m_statusLabel = new QLabel("Package Manager: " + m_packageManager.getPackageManagerName());
    m_mainLayout->addWidget(m_statusLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_selectAllButton = new QPushButton("Select All");
    m_deselectAllButton = new QPushButton("Deselect All");
    m_installButton = new QPushButton("Install Selected");

    connect(m_selectAllButton, &QPushButton::clicked, this, &ManageCompilersDialog::onSelectAll);
    connect(m_deselectAllButton, &QPushButton::clicked, this,
            &ManageCompilersDialog::onDeselectAll);
    connect(m_installButton, &QPushButton::clicked, this,
            &ManageCompilersDialog::onInstallSelected);

    buttonLayout->addWidget(m_selectAllButton);
    buttonLayout->addWidget(m_deselectAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_installButton);

    m_mainLayout->addLayout(buttonLayout);

    setLayout(m_mainLayout);
}

void ManageCompilersDialog::loadAvailableCompilers() {
    QStringList compilers = {"g++",           "gcc",   "clang",          "clang++",
                             "rustc",         "cargo", "openjdk-17-jdk", "python3",
                             "dotnet-sdk-7.0"};

    for (const QString &compiler : compilers) {
        QCheckBox *checkbox = new QCheckBox(compiler);
        m_compilerListLayout->addWidget(checkbox);

        connect(checkbox, &QCheckBox::stateChanged, this,
                &ManageCompilersDialog::updateInstallButtonState);

        m_compilerCheckboxes.append({compiler, checkbox});
    }

    m_compilerListLayout->addStretch();
}

void ManageCompilersDialog::onSelectAll() {
    for (auto &[name, checkbox] : m_compilerCheckboxes) {
        checkbox->setChecked(true);
    }
}

void ManageCompilersDialog::onDeselectAll() {
    for (auto &[name, checkbox] : m_compilerCheckboxes) {
        checkbox->setChecked(false);
    }
}

void ManageCompilersDialog::updateInstallButtonState() {
    bool anySelected = false;
    for (const auto &[name, checkbox] : m_compilerCheckboxes) {
        if (checkbox->isChecked()) {
            anySelected = true;
            break;
        }
    }

    m_installButton->setEnabled(anySelected);
}

void ManageCompilersDialog::onInstallSelected() {
    QStringList selectedPackages;

    for (const auto &[name, checkbox] : m_compilerCheckboxes) {
        if (checkbox->isChecked()) {
            selectedPackages.append(name);
        }
    }

    if (selectedPackages.isEmpty()) {
        QMessageBox::information(this, "Install Compilers",
                                 "Please select at least one compiler to install.");
        return;
    }

    installPackages(selectedPackages);
}

void ManageCompilersDialog::installPackages(const QStringList &packages) {
    QProgressDialog progress("Installing compilers...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setWindowTitle("Installing");
    progress.show();

    QApplication::processEvents();

    bool success = m_packageManager.installPackages(packages);

    progress.close();

    if (success) {
        QMessageBox::information(this, "Installation Complete",
                                 "The selected compilers have been installed successfully.");
    } else {
        QMessageBox::warning(this, "Installation Failed",
                             "There was an error installing the compilers. You may need to install "
                             "them manually or check your system permissions.");
    }
}

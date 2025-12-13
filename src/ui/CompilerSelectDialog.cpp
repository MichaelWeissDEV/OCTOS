# 1 "./src/ui/CompilerSelectDialog.cpp"
#include "CompilerSelectDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QVBoxLayout>

#include "../CompilerDetectorUtil.h"

CompilerSelectDialog::CompilerSelectDialog(QWidget* parent)
    : QDialog(parent), m_currentLanguage(Language::Cpp) {
    setWindowTitle("Select Compiler");
    setMinimumWidth(700);
    setMinimumHeight(500);
    setupUi();
}

CompilerSelectDialog::CompilerSelectDialog(Language language, QWidget* parent)
    : QDialog(parent), m_currentLanguage(language) {
    setWindowTitle("Select Compiler");
    setMinimumWidth(700);
    setMinimumHeight(500);
    setupUi();
    loadCompilersForLanguage(language);
}

void CompilerSelectDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* langLayout = new QHBoxLayout();
    QLabel* langLabel = new QLabel("Language:");
    m_languageCombo = new QComboBox();
    m_languageCombo->addItem("C++", QVariant::fromValue(Language::Cpp));
    m_languageCombo->addItem("C", QVariant::fromValue(Language::C));
    m_languageCombo->addItem("Python", QVariant::fromValue(Language::Python));
    m_languageCombo->addItem("Java", QVariant::fromValue(Language::Java));
    m_languageCombo->addItem("C#", QVariant::fromValue(Language::CSharp));
    m_languageCombo->addItem("Rust", QVariant::fromValue(Language::Rust));

    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CompilerSelectDialog::onLanguageChanged);

    langLayout->addWidget(langLabel);
    langLayout->addWidget(m_languageCombo);
    langLayout->addStretch();
    mainLayout->addLayout(langLayout);

    m_statusLabel = new QLabel("Available Compilers:");
    mainLayout->addWidget(m_statusLabel);

    m_compilerList = new QListWidget();
    m_compilerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_compilerList->setStyleSheet(
        "QListWidget { background-color: #2d2d2d; color: #ffffff; border: 1px solid #555; }"
        "QListWidget::item:selected { background-color: #007acc; }"
        "QListWidget::item:hover { background-color: #3e3e42; }");
    mainLayout->addWidget(m_compilerList);

    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_refreshButton = new QPushButton("🔄 Refresh");
    m_installMissingButton = new QPushButton("⬇️ Install Missing");
    m_selectButton = new QPushButton(" Select");
    m_cancelButton = new QPushButton(" Cancel");

    connect(m_refreshButton, &QPushButton::clicked, this, &CompilerSelectDialog::onRefresh);
    connect(m_installMissingButton, &QPushButton::clicked, this,
            &CompilerSelectDialog::onInstallMissing);
    connect(m_selectButton, &QPushButton::clicked, this, [this]() {
        if (!m_compilerList->selectedItems().isEmpty()) {
            m_selectedCompilerPath = m_compilerList->selectedItems()[0]->text().split(" - ")[0];
            accept();
        } else {
            QMessageBox::warning(this, "Selection", "Please select a compiler");
        }
    });
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_installMissingButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_selectButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    loadCompilersForLanguage(m_currentLanguage);
}

void CompilerSelectDialog::loadCompilersForLanguage(Language lang) {
    m_currentLanguage = lang;

    for (int i = 0; i < m_languageCombo->count(); ++i) {
        if (m_languageCombo->itemData(i).value<Language>() == lang) {
            m_languageCombo->setCurrentIndex(i);
            break;
        }
    }

    populateCompilerList();
}

void CompilerSelectDialog::populateCompilerList() {
    m_compilerList->clear();

    QStringList allCompilers = CompilerDetectorUtil::getAllCompilerVersions(m_currentLanguage);
    QStringList available = CompilerDetectorUtil::getAvailableCompilers(m_currentLanguage);

    for (const QString& compiler : allCompilers) {
        if (available.contains(compiler)) {
            QString version = CompilerDetectorUtil::getCompilerVersion(compiler);
            QString displayText = compiler;
            if (!version.isEmpty() && version != "unknown")
                displayText += " (" + version.split('\n').first() + ")";

            QListWidgetItem* item = new QListWidgetItem(" " + displayText);
            item->setBackground(QColor("#1a4d2e"));
            item->setForeground(QColor("#ffffff"));
            m_compilerList->addItem(item);
        }
    }

    for (const QString& compiler : allCompilers) {
        if (!available.contains(compiler)) {
            QListWidgetItem* item = new QListWidgetItem(" " + compiler + " (not installed)");
            item->setBackground(QColor("#4d1a1a"));
            item->setForeground(QColor("#cccccc"));
            m_compilerList->addItem(item);
        }
    }

    m_statusLabel->setText(
        QString("Available: %1 / %2 compilers").arg(available.size()).arg(allCompilers.size()));

    if (!available.isEmpty()) {
        m_compilerList->setCurrentRow(0);
    }
}
QString CompilerSelectDialog::selectedCompilerPath() const { return m_selectedCompilerPath; }

void CompilerSelectDialog::onRefresh() {
    populateCompilerList();
    QMessageBox::information(this, "Refresh", "Compiler list updated");
}

void CompilerSelectDialog::onInstallMissing() {
    QMessageBox::information(this, "Install Compilers",
                             "Use Tools → Manage Compilers to install missing compilers");
}

void CompilerSelectDialog::onLanguageChanged(int index) {
    Language lang = m_languageCombo->itemData(index).value<Language>();
    loadCompilersForLanguage(lang);
}

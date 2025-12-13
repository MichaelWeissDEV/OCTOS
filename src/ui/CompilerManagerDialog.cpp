#include "CompilerManagerDialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>

CompilerManagerDialog::CompilerManagerDialog(DockerCompilerManager* dockerManager, QWidget* parent)
    : QDialog(parent), m_dockerManager(dockerManager) {
    setWindowTitle("Compiler Manager");
    resize(900, 600);

    m_categoryNames[CompilerCategory::Cpp] = "C++";
    m_categoryNames[CompilerCategory::C] = "C";
    m_categoryNames[CompilerCategory::Clang] = "Clang";
    m_categoryNames[CompilerCategory::Java] = "Java";
    m_categoryNames[CompilerCategory::Python] = "Python";
    m_categoryNames[CompilerCategory::Rust] = "Rust";

    setupUi();

    connect(m_dockerManager, &DockerCompilerManager::tagsReceived, this,
            &CompilerManagerDialog::onTagsReceived);
    connect(m_dockerManager, &DockerCompilerManager::tagsFetchError, this,
            &CompilerManagerDialog::onTagsFetchError);
    connect(m_dockerManager, &DockerCompilerManager::pullProgress, this,
            &CompilerManagerDialog::onPullProgress);
    connect(m_dockerManager, &DockerCompilerManager::pullFinished, this,
            &CompilerManagerDialog::onPullFinished);

    populateCategories();
}

void CompilerManagerDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(8, 8, 8, 8);

    QLabel* categoryLabel = new QLabel("Categories");
    categoryLabel->setStyleSheet("font-weight: bold; font-size: 14px; padding: 4px;");

    m_categoryList = new QListWidget();
    m_categoryList->setMaximumWidth(200);
    m_categoryList->setStyleSheet(R"(
        QListWidget {
            background-color: #252526;
            border: 1px solid #3e3e42;
            border-radius: 4px;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #3e3e42;
        }
        QListWidget::item:selected {
            background-color: #094771;
        }
        QListWidget::item:hover {
            background-color: #2a2d2e;
        }
    )");

    leftLayout->addWidget(categoryLabel);
    leftLayout->addWidget(m_categoryList);

    connect(m_categoryList, &QListWidget::itemClicked, this,
            &CompilerManagerDialog::onCategorySelected);

    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(8, 8, 8, 8);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search compiler versions...");
    m_searchBox->setStyleSheet(R"(
        QLineEdit {
            padding: 6px;
            background-color: #3c3c3c;
            border: 1px solid #3e3e42;
            border-radius: 4px;
            color: #cccccc;
        }
    )");

    m_refreshButton = new QPushButton("Refresh");
    m_refreshButton->setStyleSheet(R"(
        QPushButton {
            padding: 6px 16px;
            background-color: #0e639c;
            border: none;
            border-radius: 4px;
            color: white;
        }
        QPushButton:hover {
            background-color: #1177bb;
        }
        QPushButton:pressed {
            background-color: #0d5a8f;
        }
    )");

    connect(m_searchBox, &QLineEdit::textChanged, this,
            &CompilerManagerDialog::onSearchTextChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &CompilerManagerDialog::onRefreshClicked);

    searchLayout->addWidget(m_searchBox);
    searchLayout->addWidget(m_refreshButton);

    m_imageTable = new QTableWidget();
    m_imageTable->setColumnCount(4);
    m_imageTable->setHorizontalHeaderLabels(QStringList()
                                            << "Version" << "Architecture" << "Size" << "Status");
    m_imageTable->horizontalHeader()->setStretchLastSection(false);
    m_imageTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_imageTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_imageTable->setColumnWidth(3, 130);
    m_imageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_imageTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_imageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_imageTable->verticalHeader()->setDefaultSectionSize(32);
    m_imageTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e1e;
            border: 1px solid #3e3e42;
            gridline-color: #3e3e42;
        }
        QTableWidget::item {
            padding: 6px;
        }
        QTableWidget::item:selected {
            background-color: #094771;
        }
        QHeaderView::section {
            background-color: #2d2d30;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #3e3e42;
            font-weight: bold;
        }
    )");

    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Select a category to view compilers");
    m_statusLabel->setStyleSheet("padding: 4px; color: #cccccc;");

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(300);
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid #3e3e42;
            border-radius: 4px;
            text-align: center;
            background-color: #252526;
        }
        QProgressBar::chunk {
            background-color: #0e639c;
            border-radius: 3px;
        }
    )");

    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_progressBar);

    rightLayout->addLayout(searchLayout);
    rightLayout->addWidget(m_imageTable);
    rightLayout->addLayout(statusLayout);

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(8, 8, 8, 8);

    QPushButton* closeButton = new QPushButton("Close");
    closeButton->setStyleSheet(R"(
        QPushButton {
            padding: 6px 20px;
            background-color: #3c3c3c;
            border: 1px solid #3e3e42;
            border-radius: 4px;
            color: white;
        }
        QPushButton:hover {
            background-color: #505050;
        }
    )");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}

void CompilerManagerDialog::populateCategories() {
    m_categoryList->clear();

    for (auto it = m_categoryNames.begin(); it != m_categoryNames.end(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(it.value());
        item->setData(Qt::UserRole, QVariant::fromValue(it.key()));
        m_categoryList->addItem(item);
    }

    if (m_categoryList->count() > 0) {
        m_categoryList->setCurrentRow(0);
        onCategorySelected(m_categoryList->item(0));
    }
}

void CompilerManagerDialog::onCategorySelected(QListWidgetItem* item) {
    if (!item) return;

    CompilerCategory category = item->data(Qt::UserRole).value<CompilerCategory>();

    m_statusLabel->setText("Fetching compiler versions...");
    m_imageTable->setRowCount(0);

    m_dockerManager->fetchAvailableTags(category);
}

void CompilerManagerDialog::onTagsReceived(CompilerCategory category,
                                           const QVector<DockerImage>& images) {
    if (category != getCurrentCategory()) return;

    m_currentImages = images;
    updateImageTable(images);

    m_statusLabel->setText(QString("Found %1 compiler versions").arg(images.size()));
}

void CompilerManagerDialog::onTagsFetchError(CompilerCategory category, const QString& error) {
    if (category != getCurrentCategory()) return;

    m_statusLabel->setText("Error: " + error);
    QMessageBox::warning(this, "Fetch Error", "Failed to fetch compiler versions:\n" + error);
}

void CompilerManagerDialog::updateImageTable(const QVector<DockerImage>& images) {
    m_imageTable->setRowCount(0);

    for (int i = 0; i < images.size(); ++i) {
        const DockerImage& image = images[i];

        int row = m_imageTable->rowCount();
        m_imageTable->insertRow(row);

        QTableWidgetItem* versionItem = new QTableWidgetItem(image.tag);
        m_imageTable->setItem(row, 0, versionItem);

        QTableWidgetItem* archItem = new QTableWidgetItem(image.architecture);
        m_imageTable->setItem(row, 1, archItem);

        QString sizeStr = image.size > 0 ? QString("%1 MB").arg(image.size / 1024 / 1024) : "N/A";
        QTableWidgetItem* sizeItem = new QTableWidgetItem(sizeStr);
        m_imageTable->setItem(row, 2, sizeItem);

        QWidget* statusWidget = new QWidget();
        QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
        statusLayout->setContentsMargins(2, 1, 2, 1);

        QPushButton* actionButton = new QPushButton();

        if (image.isInstalled) {
            actionButton->setText("Uninstall");
            actionButton->setStyleSheet(R"(
                QPushButton {
                    padding: 3px 8px;
                    min-width: 85px;
                    min-height: 20px;
                    max-height: 24px;
                    background-color: #d32f2f;
                    border: none;
                    border-radius: 3px;
                    color: white;
                    font-size: 11px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background-color: #e53935;
                }
                QPushButton:pressed {
                    background-color: #b71c1c;
                }
            )");
            connect(actionButton, &QPushButton::clicked, this,
                    &CompilerManagerDialog::onDeleteClicked);
        } else {
            actionButton->setText("Download");
            actionButton->setStyleSheet(R"(
                QPushButton {
                    padding: 3px 8px;
                    min-width: 85px;
                    min-height: 20px;
                    max-height: 24px;
                    background-color: #0e639c;
                    border: none;
                    border-radius: 3px;
                    color: white;
                    font-size: 11px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background-color: #1177bb;
                }
                QPushButton:pressed {
                    background-color: #0d5a8f;
                }
            )");
            connect(actionButton, &QPushButton::clicked, this,
                    &CompilerManagerDialog::onDownloadClicked);
        }

        actionButton->setProperty("imageName", image.fullName);
        actionButton->setProperty("row", row);

        statusLayout->addWidget(actionButton);
        statusLayout->addStretch();

        m_imageTable->setCellWidget(row, 3, statusWidget);
    }
}

void CompilerManagerDialog::onSearchTextChanged(const QString& text) {
    if (text.isEmpty()) {
        updateImageTable(m_currentImages);
        return;
    }

    QVector<DockerImage> filtered;
    for (const DockerImage& image : m_currentImages) {
        if (image.tag.contains(text, Qt::CaseInsensitive)) {
            filtered.append(image);
        }
    }

    updateImageTable(filtered);
}

void CompilerManagerDialog::onDownloadClicked() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString imageName = button->property("imageName").toString();
    m_currentPullImage = imageName;

    m_statusLabel->setText("Downloading " + imageName + "...");
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);

    button->setEnabled(false);
    button->setText("Downloading...");

    m_dockerManager->pullImage(imageName);
}

void CompilerManagerDialog::onDeleteClicked() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString imageName = button->property("imageName").toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Image", "Are you sure you want to delete " + imageName + "?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_dockerManager->removeImage(imageName);
        m_statusLabel->setText("Deleted " + imageName);

        if (m_categoryList->currentItem()) {
            onCategorySelected(m_categoryList->currentItem());
        }
    }
}

void CompilerManagerDialog::onRefreshClicked() {
    if (m_categoryList->currentItem()) {
        onCategorySelected(m_categoryList->currentItem());
    }
}

void CompilerManagerDialog::onPullProgress(const QString& imageName, int percentage) {
    if (imageName != m_currentPullImage) return;

    m_progressBar->setValue(percentage);
    m_statusLabel->setText(QString("Downloading %1... %2%").arg(imageName).arg(percentage));
}

void CompilerManagerDialog::onPullFinished(const QString& imageName, bool success,
                                           const QString& message) {
    if (imageName != m_currentPullImage) return;

    m_progressBar->setVisible(false);

    if (success) {
        m_statusLabel->setText("Successfully downloaded " + imageName);

        if (m_categoryList->currentItem()) {
            onCategorySelected(m_categoryList->currentItem());
        }
    } else {
        m_statusLabel->setText("Failed to download " + imageName);
        QMessageBox::warning(this, "Download Failed", message);
    }
}

CompilerCategory CompilerManagerDialog::getCurrentCategory() const {
    QListWidgetItem* item = m_categoryList->currentItem();
    if (!item) return CompilerCategory::Cpp;

    return item->data(Qt::UserRole).value<CompilerCategory>();
}

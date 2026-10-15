#include "CompilerManagerDialog.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>

#include "../core/CompilerRegistry.h"

CompilerManagerDialog::CompilerManagerDialog(DockerCompilerManager* dockerManager, QWidget* parent)
    : QDialog(parent), m_dockerManager(dockerManager) {
    setWindowTitle("Compiler Toolchain Manager");
    resize(980, 620);

    m_categoryNames[CompilerCategory::Cpp] = "C++";
    m_categoryNames[CompilerCategory::C] = "C";
    m_categoryNames[CompilerCategory::Clang] = "Clang";
    m_categoryNames[CompilerCategory::Java] = "Java";
    m_categoryNames[CompilerCategory::Python] = "Python";
    m_categoryNames[CompilerCategory::Rust] = "Rust";
    m_categoryNames[CompilerCategory::CSharp] = "C# (Experimental)";

    setupUi();

    connect(m_dockerManager, &DockerCompilerManager::tagsReceived, this,
            &CompilerManagerDialog::onTagsReceived);
    connect(m_dockerManager, &DockerCompilerManager::tagsFetchError, this,
            &CompilerManagerDialog::onTagsFetchError);
    connect(m_dockerManager, &DockerCompilerManager::pullProgress, this,
            &CompilerManagerDialog::onPullProgress);
    connect(m_dockerManager, &DockerCompilerManager::pullFinished, this,
            &CompilerManagerDialog::onPullFinished);
    connect(m_dockerManager, &DockerCompilerManager::dockerConnectionStatusChanged, this,
            &CompilerManagerDialog::onDockerConnectionStatusChanged);
    connect(m_dockerManager, &DockerCompilerManager::installedImagesRefreshed, this,
            &CompilerManagerDialog::onInstalledImagesRefreshed);
    connect(m_dockerManager, &DockerCompilerManager::imageRemoved, this,
            &CompilerManagerDialog::onImageRemoved);
    connect(m_dockerManager, &DockerCompilerManager::installedImagesRefreshFailed, this,
            &CompilerManagerDialog::onInstalledImagesRefreshFailed);
    connect(m_dockerManager, &DockerCompilerManager::toolchainProbed, this,
            &CompilerManagerDialog::onToolchainProbed);

    populateCategories();

    // Query connection status right away
    m_dockerManager->checkDockerConnectionAsync();
}

void CompilerManagerDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Docker Status Banner
    m_dockerStatusBanner = new QLabel(this);
    Theme::setRole(m_dockerStatusBanner, "toolbar");
    m_dockerStatusBanner->setText("Checking Docker daemon status...");
    mainLayout->addWidget(m_dockerStatusBanner);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(8, 8, 8, 8);

    QLabel* categoryLabel = new QLabel("Toolchains & Languages");
    Theme::setRole(categoryLabel, "title");

    m_categoryList = new QListWidget();
    m_categoryList->setMaximumWidth(210);


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


    m_refreshButton = new QPushButton("Refresh Local Images");
    Theme::setRole(m_refreshButton, "subtle");

    connect(m_searchBox, &QLineEdit::textChanged, this,
            &CompilerManagerDialog::onSearchTextChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &CompilerManagerDialog::onRefreshClicked);

    searchLayout->addWidget(m_searchBox);
    searchLayout->addWidget(m_refreshButton);

    m_imageTable = new QTableWidget();
    m_imageTable->setColumnCount(5);
    m_imageTable->setHorizontalHeaderLabels(QStringList()
                                            << "Image / Tag" << "Toolchain" << "Architecture" << "Compatibility" << "Action");
    m_imageTable->horizontalHeader()->setStretchLastSection(false);
    m_imageTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_imageTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_imageTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_imageTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_imageTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_imageTable->setColumnWidth(4, 120);
    m_imageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_imageTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_imageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_imageTable->verticalHeader()->setDefaultSectionSize(32);


    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Select a toolchain category to view compilers");
    Theme::setRole(m_statusLabel, "muted");

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(300);


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
    Theme::setRole(closeButton, "accent");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
}

void CompilerManagerDialog::onDockerConnectionStatusChanged(bool available, const QString& message) {
    if (available) {
        Theme::setRole(m_dockerStatusBanner, "noticeOk");
        m_dockerStatusBanner->setText("● Docker Connected: " + message);
    } else {
        Theme::setRole(m_dockerStatusBanner, "noticeError");
        m_dockerStatusBanner->setText("● Docker Offline: " + message + " (Local compilation requires running Docker daemon)");
    }
}

void CompilerManagerDialog::onInstalledImagesRefreshed(const QVector<DockerImage>& /*images*/) {
    updateImageTable();
}

void CompilerManagerDialog::onImageRemoved(const QString& imageName, bool success, const QString& message) {
    if (success) {
        m_statusLabel->setText("Successfully removed: " + imageName);
    } else {
        m_statusLabel->setText("Error removing image: " + imageName);
        QMessageBox::warning(this, "Remove Image Error", message);
    }
    updateImageTable();
}

void CompilerManagerDialog::populateCategories() {
    m_categoryList->clear();

    for (auto it = m_categoryNames.begin(); it != m_categoryNames.end(); ++it) {
        auto* item = new QListWidgetItem(it.value());
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
    QString categoryName = m_categoryNames.value(category);

    m_statusLabel->setText("Fetching available versions for " + categoryName + "...");

    m_currentImages = m_dockerManager->getFilteredTags(category);
    if (!m_currentImages.isEmpty()) {
        updateImageTable(m_currentImages);
        m_statusLabel->setText(QString("Showing %1 versions for %2").arg(m_currentImages.size()).arg(categoryName));
    } else {
        m_dockerManager->fetchAvailableTags(category);
    }
}

void CompilerManagerDialog::onTagsReceived(CompilerCategory category,
                                          const QVector<DockerImage>& images) {
    if (getCurrentCategory() != category) return;

    m_currentImages = images;
    updateImageTable(images);
    m_statusLabel->setText(
        QString("Found %1 available versions").arg(images.size()));
}

void CompilerManagerDialog::onTagsFetchError(CompilerCategory category, const QString& error) {
    if (getCurrentCategory() != category) return;

    m_statusLabel->setText("Error fetching tags: " + error);
}

void CompilerManagerDialog::updateImageTable() {
    updateImageTable(m_currentImages);
}

void CompilerManagerDialog::updateImageTable(const QVector<DockerImage>& images) {
    m_imageTable->setRowCount(0);

    for (int row = 0; row < images.size(); ++row) {
        const DockerImage& image = images[row];
        m_imageTable->insertRow(row);

        auto* itemTag = new QTableWidgetItem(image.tag.isEmpty() ? image.fullName : image.tag);
        auto* itemArch = new QTableWidgetItem(image.architecture.isEmpty() ? "amd64" : image.architecture);

        CompilerDefinition def = CompilerRegistry::instance().compilerForImage(image.fullName);
        auto* itemFamily = new QTableWidgetItem(compilerFamilyToString(def.family));

        // "Verified" only after a probe ran inside the image; a known image name or plausible
        // metadata is shown for what it is.
        const VerificationState vstate = def.verificationState();
        QString compatStr = verificationStateToString(vstate);
        if (vstate == VerificationState::Verified && !def.actualVersion().isEmpty())
            compatStr += " – " + def.actualVersion().left(40);
        auto* itemCompat = new QTableWidgetItem(compatStr);
        QString tip;
        switch (vstate) {
            case VerificationState::Verified:
                tip = QString("Checked %1 inside the image.").arg(def.probe.checkedAt.toLocalTime().toString());
                itemCompat->setForeground(Theme::currentTokens().success);
                break;
            case VerificationState::VerificationFailed:
                tip = def.probe.message;
                itemCompat->setForeground(Theme::currentTokens().error);
                break;
            case VerificationState::ToolchainPlausible:
                tip = "Image name and metadata suggest a usable toolchain. Not checked yet - use 'Verify'.";
                itemCompat->setForeground(Theme::currentTokens().warning);
                break;
            case VerificationState::ImageRecognized:
                tip = "Image name recognised. Its contents were not checked.";
                itemCompat->setForeground(Theme::currentTokens().textMuted);
                break;
        }
        if (def.isExperimental && !def.statusMessage.isEmpty()) tip += "\n" + def.statusMessage;
        itemCompat->setToolTip(tip);

        m_imageTable->setItem(row, 0, itemTag);
        m_imageTable->setItem(row, 1, itemFamily);
        m_imageTable->setItem(row, 2, itemArch);
        m_imageTable->setItem(row, 3, itemCompat);

        bool isInstalled = m_dockerManager->isImageInstalled(image.fullName);

        auto* statusWidget = new QWidget();
        auto* statusLayout = new QHBoxLayout(statusWidget);
        statusLayout->setContentsMargins(4, 2, 4, 2);

        auto* actionButton = new QPushButton();

        if (isInstalled) {
            actionButton->setText("Delete");
            Theme::setRole(actionButton, "danger");
            connect(actionButton, &QPushButton::clicked, this,
                    &CompilerManagerDialog::onDeleteClicked);
        } else {
            actionButton->setText("Download");
            Theme::setRole(actionButton, "accent");
            connect(actionButton, &QPushButton::clicked, this,
                    &CompilerManagerDialog::onDownloadClicked);
        }

        actionButton->setProperty("imageName", image.fullName);
        actionButton->setProperty("row", row);

        statusLayout->addWidget(actionButton);
        if (isInstalled) {
            auto* verifyButton = new QPushButton("Verify");
            verifyButton->setToolTip("Run a tool check inside the image (no network, nothing is downloaded)");
            verifyButton->setProperty("imageName", image.fullName);
            connect(verifyButton, &QPushButton::clicked, this, &CompilerManagerDialog::onVerifyClicked);
            statusLayout->addWidget(verifyButton);
        }
        statusLayout->addStretch();

        m_imageTable->setCellWidget(row, 4, statusWidget);
    }
}

void CompilerManagerDialog::onSearchTextChanged(const QString& text) {
    if (text.isEmpty()) {
        updateImageTable(m_currentImages);
        return;
    }

    QVector<DockerImage> filtered;
    for (const DockerImage& image : m_currentImages) {
        if (image.tag.contains(text, Qt::CaseInsensitive) ||
            image.fullName.contains(text, Qt::CaseInsensitive)) {
            filtered.append(image);
        }
    }

    updateImageTable(filtered);
}

void CompilerManagerDialog::onDownloadClicked() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString imageName = button->property("imageName").toString();

    // Explicit user confirmation requirement!
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Download Compiler Image",
        QString("Download Docker compiler image '%1'?\nThis will download the container image locally.")
            .arg(imageName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

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

    const QString imageName = button->property("imageName").toString();

    if (!CompilerRegistry::instance().isKnownCompilerImageName(imageName)) {
        QMessageBox::warning(this, "Cannot Delete",
                             "Image '" + imageName + "' is not a recognised compiler image. "
                             "OCTOS only offers to delete compiler images.");
        return;
    }

    // Deletion is always an explicit decision. Images OCTOS did not download may be used elsewhere.
    const bool owned = m_dockerManager->isOwnedByOctos(imageName);
    QString text = QString("Delete the local Docker image '%1'?\nThis action cannot be undone.").arg(imageName);
    if (!owned) {
        text = QString("The image '%1' was NOT downloaded by OCTOS.\n\nIt was already on this machine and "
                       "may be used by other projects or containers. Deleting it can break them.\n\n"
                       "Delete it anyway?").arg(imageName);
    }
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, owned ? "Delete Compiler Image" : "Delete Image Not Managed by OCTOS", text,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_dockerManager->removeImage(imageName, !owned);
        m_statusLabel->setText("Removing " + imageName + "...");
    }
}

void CompilerManagerDialog::onVerifyClicked() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    const QString imageName = button->property("imageName").toString();
    button->setEnabled(false);
    button->setText("Verifying...");
    m_statusLabel->setText("Checking the toolchain inside " + imageName + "...");
    m_dockerManager->probeToolchain(imageName);
}

void CompilerManagerDialog::onToolchainProbed(const QString& imageName, const ToolchainProbeResult& result) {
    m_statusLabel->setText(result.success
        ? QString("%1: toolchain verified (%2)").arg(imageName, result.version)
        : QString("%1: verification failed - %2").arg(imageName, result.message));
    updateImageTable();
}

void CompilerManagerDialog::onInstalledImagesRefreshFailed(const QString& message) {
    // A failed query is shown as an error, not silently treated as "no images".
    m_statusLabel->setText(message);
}

void CompilerManagerDialog::onRefreshClicked() {
    m_statusLabel->setText("Refreshing local installed images...");
    m_dockerManager->refreshInstalledImagesAsync();
    m_dockerManager->checkDockerConnectionAsync();
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
        updateImageTable();
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

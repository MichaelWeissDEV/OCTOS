#ifndef COMPILERMANAGERDIALOG_H
#define COMPILERMANAGERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "backend/DockerCompilerManager.h"

class CompilerManagerDialog : public QDialog {
    Q_OBJECT

   public:
    explicit CompilerManagerDialog(DockerCompilerManager* dockerManager, QWidget* parent = nullptr);

   private slots:
    void onCategorySelected(QListWidgetItem* item);
    void onSearchTextChanged(const QString& text);
    void onDownloadClicked();
    void onDeleteClicked();
    void onRefreshClicked();

    void onTagsReceived(CompilerCategory category, const QVector<DockerImage>& images);
    void onTagsFetchError(CompilerCategory category, const QString& error);
    void onPullProgress(const QString& imageName, int percentage);
    void onPullFinished(const QString& imageName, bool success, const QString& message);

   private:
    void setupUi();
    void populateCategories();
    void updateImageTable();
    void updateImageTable(const QVector<DockerImage>& images);
    CompilerCategory getCurrentCategory() const;

    DockerCompilerManager* m_dockerManager;

    QListWidget* m_categoryList;
    QLineEdit* m_searchBox;
    QTableWidget* m_imageTable;
    QPushButton* m_refreshButton;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;

    QMap<CompilerCategory, QString> m_categoryNames;
    QVector<DockerImage> m_currentImages;
    QString m_currentPullImage;
};

#endif  // COMPILERMANAGERDIALOG_H

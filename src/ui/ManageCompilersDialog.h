# 1 "./src/ui/ManageCompilersDialog.h"
#ifndef MANAGECOMPILERSDIALOG_H
#define MANAGECOMPILERSDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QList>
#include <memory>

#include "PackageManager.h"

class QVBoxLayout;
class QPushButton;
class QLabel;
class QProgressDialog;

class ManageCompilersDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ManageCompilersDialog(const PackageManager &packageManager, QWidget *parent = nullptr);
    ~ManageCompilersDialog() = default;

   private slots:
    void onInstallSelected();
    void onSelectAll();
    void onDeselectAll();
    void updateInstallButtonState();

   private:
    void setupUI();
    void loadAvailableCompilers();
    void installPackages(const QStringList &packages);

    const PackageManager &m_packageManager;
    QVBoxLayout *m_mainLayout;
    QVBoxLayout *m_compilerListLayout;
    QPushButton *m_installButton;
    QPushButton *m_selectAllButton;
    QPushButton *m_deselectAllButton;
    QLabel *m_statusLabel;

    QList<QPair<QString, QCheckBox *>> m_compilerCheckboxes;
};

#endif

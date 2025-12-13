# 1 "./src/ui/CompilerSelectDialog.h"
#ifndef COMPILERSELECTDIALOG_H
#define COMPILERSELECTDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

#include "../CompilerDriver.h"

class CompilerSelectDialog : public QDialog {
    Q_OBJECT

   public:
    explicit CompilerSelectDialog(QWidget *parent = nullptr);
    explicit CompilerSelectDialog(Language language, QWidget *parent = nullptr);

    QString selectedCompilerPath() const;

   private slots:
    void onRefresh();
    void onInstallMissing();
    void onLanguageChanged(int index);

   private:
    void setupUi();
    void loadCompilersForLanguage(Language lang);
    void populateCompilerList();

    QComboBox *m_languageCombo;
    QListWidget *m_compilerList;
    QLabel *m_statusLabel;
    QPushButton *m_installMissingButton;
    QPushButton *m_refreshButton;
    QPushButton *m_selectButton;
    QPushButton *m_cancelButton;

    Language m_currentLanguage;
    QString m_selectedCompilerPath;
};

#endif

# 1 "./src/ui/PythonInstallWizard.h"
#ifndef PYTHONINSTALLWIZARD_H
#define PYTHONINSTALLWIZARD_H

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QWizard>

#include "PackageManager.h"

class PythonInstallWizard : public QWizard {
    Q_OBJECT

   public:
    explicit PythonInstallWizard(const PackageManager &packageManager, QWidget *parent = nullptr);

   private slots:
    void onFinished();

   private:
    const PackageManager &m_packageManager;

    QWizardPage *createIntroPage();
    QWizardPage *createVersionPage();
    QWizardPage *createPackagesPage();
    QWizardPage *createSummaryPage();

    QComboBox *m_pythonVersionCombo;
    QCheckBox *m_jupyterCheckbox;
    QCheckBox *m_numpyCheckbox;
    QCheckBox *m_pandasCheckbox;
    QCheckBox *m_matplotlibCheckbox;
    QCheckBox *m_scipyCheckbox;
    QCheckBox *m_numbaCheckbox;
    QLabel *m_summaryLabel;
};

#endif

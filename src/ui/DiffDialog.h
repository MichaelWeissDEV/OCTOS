#ifndef DIFFDIALOG_H
#define DIFFDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

#include "utils/AssemblyDiffProcessor.h"

class DiffDialog : public QDialog {
    Q_OBJECT

public:
    explicit DiffDialog(const QString& nameA, const QString& asmA,
                        const QString& nameB, const QString& asmB,
                        QWidget* parent = nullptr);

private:
    void populateTable(const QString& nameA, const QString& asmA,
                       const QString& nameB, const QString& asmB);

    QTableWidget* m_table{nullptr};
    QLabel* m_summaryLabel{nullptr};
};

#endif // DIFFDIALOG_H

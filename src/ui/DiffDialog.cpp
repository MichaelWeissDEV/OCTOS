#include "DiffDialog.h"

DiffDialog::DiffDialog(const QString& nameA, const QString& asmA,
                       const QString& nameB, const QString& asmB,
                       QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QString("Assembly Diff - %1 vs %2").arg(nameA, nameB));
    resize(900, 600);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #d4d4d4; padding: 4px;");
    layout->addWidget(m_summaryLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({
        QString("Line (%1)").arg(nameA),
        QString("Assembly (%1)").arg(nameA),
        QString("Line (%2)").arg(nameB),
        QString("Assembly (%2)").arg(nameB)
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setShowGrid(false);
    m_table->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
            font-family: monospace;
            border: 1px solid #3e3e42;
        }
        QHeaderView::section {
            background-color: #2d2d30;
            color: #cccccc;
            padding: 4px;
            border: 1px solid #3e3e42;
            font-weight: bold;
        }
    )");
    layout->addWidget(m_table);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            padding: 6px 16px;
            background-color: #0e639c;
            border: none;
            border-radius: 3px;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1177bb;
        }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    populateTable(nameA, asmA, nameB, asmB);
}

void DiffDialog::populateTable(const QString& nameA, const QString& asmA,
                               const QString& nameB, const QString& asmB) {
    Q_UNUSED(nameA);
    Q_UNUSED(nameB);

    DiffResult diff = AssemblyDiffProcessor::computeDiff(asmA, asmB);

    m_summaryLabel->setText(
        QString("Additions: <span style='color: #4ec9b0;'>+%1</span> | "
                "Deletions: <span style='color: #f48771;'>-%2</span> | "
                "Unchanged: %3")
            .arg(diff.additions)
            .arg(diff.deletions)
            .arg(diff.unchanged));

    m_table->setRowCount(diff.entries.size());

    const QColor addedColor(40, 80, 50, 140);
    const QColor removedColor(80, 35, 35, 140);

    for (int i = 0; i < diff.entries.size(); ++i) {
        const DiffEntry& entry = diff.entries[i];

        auto* itemLineA = new QTableWidgetItem(entry.lineIndexA >= 0 ? QString::number(entry.lineIndexA + 1) : "");
        auto* itemTextA = new QTableWidgetItem(entry.textA);
        auto* itemLineB = new QTableWidgetItem(entry.lineIndexB >= 0 ? QString::number(entry.lineIndexB + 1) : "");
        auto* itemTextB = new QTableWidgetItem(entry.textB);

        itemLineA->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemLineB->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        if (entry.type == DiffLineType::Added) {
            itemLineB->setBackground(addedColor);
            itemTextB->setBackground(addedColor);
        } else if (entry.type == DiffLineType::Removed) {
            itemLineA->setBackground(removedColor);
            itemTextA->setBackground(removedColor);
        }

        m_table->setItem(i, 0, itemLineA);
        m_table->setItem(i, 1, itemTextA);
        m_table->setItem(i, 2, itemLineB);
        m_table->setItem(i, 3, itemTextB);
    }
}

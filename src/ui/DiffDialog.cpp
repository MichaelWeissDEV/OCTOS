#include "DiffDialog.h"

#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QTextStream>

DiffDialog::DiffDialog(const QVector<DiffPaneInfo>& panes,
                       int initialIndexA, int initialIndexB,
                       QWidget* parent)
    : QDialog(parent),
      m_panes(panes) {
    setWindowTitle("Assembly & Output Diff Viewer");
    resize(1100, 750);
    setupUi();

    if (m_comboA && m_comboB) {
        if (initialIndexA >= 0 && initialIndexA < m_comboA->count()) {
            m_comboA->setCurrentIndex(initialIndexA);
        }
        if (initialIndexB >= 0 && initialIndexB < m_comboB->count()) {
            m_comboB->setCurrentIndex(initialIndexB);
        }
    }

    onRecomputeDiff();
}

DiffDialog::DiffDialog(const QString& nameA, const QString& asmA,
                       const QString& nameB, const QString& asmB,
                       QWidget* parent)
    : QDialog(parent) {
    DiffPaneInfo pA;
    pA.title = nameA;
    pA.compilerName = nameA;
    pA.content = asmA;
    pA.artifactType = ArtifactType::Assembly;

    DiffPaneInfo pB;
    pB.title = nameB;
    pB.compilerName = nameB;
    pB.content = asmB;
    pB.artifactType = ArtifactType::Assembly;

    m_panes.append(pA);
    m_panes.append(pB);

    setWindowTitle("Assembly & Output Diff Viewer");
    resize(1100, 750);
    setupUi();
    onRecomputeDiff();
}

void DiffDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // 1. Panel Selection Header
    auto* headerLayout = new QHBoxLayout();

    auto* labelA = new QLabel("<b>Pane A:</b>", this);
    m_comboA = new QComboBox(this);
    for (int i = 0; i < m_panes.size(); ++i) {
        m_comboA->addItem(m_panes[i].title.isEmpty() ? QString("Pane %1").arg(i + 1) : m_panes[i].title);
    }
    connect(m_comboA, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DiffDialog::onPaneSelectionChanged);

    auto* vsLabel = new QLabel("<b>vs</b>", this);
    vsLabel->setStyleSheet("color: #0e639c; font-weight: bold; padding: 0 4px;");

    auto* labelB = new QLabel("<b>Pane B:</b>", this);
    m_comboB = new QComboBox(this);
    for (int i = 0; i < m_panes.size(); ++i) {
        m_comboB->addItem(m_panes[i].title.isEmpty() ? QString("Pane %1").arg(i + 1) : m_panes[i].title);
    }
    if (m_comboB->count() > 1) {
        m_comboB->setCurrentIndex(1);
    }
    connect(m_comboB, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DiffDialog::onPaneSelectionChanged);

    headerLayout->addWidget(labelA);
    headerLayout->addWidget(m_comboA, 1);
    headerLayout->addWidget(vsLabel);
    headerLayout->addWidget(labelB);
    headerLayout->addWidget(m_comboB, 1);
    mainLayout->addLayout(headerLayout);

    // 2. Info / Configuration Details & Warning
    auto* infoLayout = new QHBoxLayout();
    m_infoLabelA = new QLabel(this);
    m_infoLabelA->setStyleSheet("font-size: 11px; color: #9cdcfe;");
    m_infoLabelB = new QLabel(this);
    m_infoLabelB->setStyleSheet("font-size: 11px; color: #9cdcfe;");
    infoLayout->addWidget(m_infoLabelA, 1);
    infoLayout->addWidget(m_infoLabelB, 1);
    mainLayout->addLayout(infoLayout);

    m_typeWarningLabel = new QLabel(this);
    m_typeWarningLabel->setStyleSheet("background-color: #593e00; color: #ffd700; padding: 4px 8px; border-radius: 3px; font-weight: bold; font-size: 11px;");
    m_typeWarningLabel->setVisible(false);
    mainLayout->addWidget(m_typeWarningLabel);

    // 3. Options & Normalization Toolbar
    auto* optionsLayout = new QHBoxLayout();
    m_hideUnchangedCheck = new QCheckBox("Show Only Differences", this);
    connect(m_hideUnchangedCheck, &QCheckBox::toggled, this, &DiffDialog::updateTableVisibility);

    m_ignoreWhitespaceCheck = new QCheckBox("Ignore Whitespace", this);
    connect(m_ignoreWhitespaceCheck, &QCheckBox::toggled, this, &DiffDialog::onRecomputeDiff);

    m_ignoreCommentsCheck = new QCheckBox("Ignore Comments", this);
    connect(m_ignoreCommentsCheck, &QCheckBox::toggled, this, &DiffDialog::onRecomputeDiff);

    m_normalizeLabelsCheck = new QCheckBox("Normalize Local Labels", this);
    connect(m_normalizeLabelsCheck, &QCheckBox::toggled, this, &DiffDialog::onRecomputeDiff);

    optionsLayout->addWidget(m_hideUnchangedCheck);
    optionsLayout->addWidget(m_ignoreWhitespaceCheck);
    optionsLayout->addWidget(m_ignoreCommentsCheck);
    optionsLayout->addWidget(m_normalizeLabelsCheck);
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);

    // 4. Search and Diff Navigation Toolbar
    auto* navLayout = new QHBoxLayout();

    auto* searchLabel = new QLabel("Find:", this);
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Search in diff...");
    m_searchInput->setMaximumWidth(220);
    connect(m_searchInput, &QLineEdit::textChanged, this, &DiffDialog::onFindText);

    m_searchStatusLabel = new QLabel(this);
    m_searchStatusLabel->setStyleSheet("font-size: 11px; color: #858585; min-width: 60px;");

    m_prevSearchBtn = new QPushButton("▲", this);
    m_prevSearchBtn->setToolTip("Find Previous");
    m_prevSearchBtn->setFixedWidth(28);
    connect(m_prevSearchBtn, &QPushButton::clicked, this, &DiffDialog::onFindPrev);

    m_nextSearchBtn = new QPushButton("▼", this);
    m_nextSearchBtn->setToolTip("Find Next");
    m_nextSearchBtn->setFixedWidth(28);
    connect(m_nextSearchBtn, &QPushButton::clicked, this, &DiffDialog::onFindNext);

    navLayout->addWidget(searchLabel);
    navLayout->addWidget(m_searchInput);
    navLayout->addWidget(m_prevSearchBtn);
    navLayout->addWidget(m_nextSearchBtn);
    navLayout->addWidget(m_searchStatusLabel);

    navLayout->addSpacing(20);

    m_prevDiffBtn = new QPushButton("← Prev Change", this);
    connect(m_prevDiffBtn, &QPushButton::clicked, this, &DiffDialog::onPrevDiff);

    m_nextDiffBtn = new QPushButton("Next Change →", this);
    connect(m_nextDiffBtn, &QPushButton::clicked, this, &DiffDialog::onNextDiff);

    navLayout->addWidget(m_prevDiffBtn);
    navLayout->addWidget(m_nextDiffBtn);
    navLayout->addStretch();

    m_copyBtn = new QPushButton("Copy Patch", this);
    connect(m_copyBtn, &QPushButton::clicked, this, &DiffDialog::onCopyUnifiedDiff);

    m_exportBtn = new QPushButton("Export Patch...", this);
    connect(m_exportBtn, &QPushButton::clicked, this, &DiffDialog::onExportUnifiedDiff);

    navLayout->addWidget(m_copyBtn);
    navLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(navLayout);

    // 5. Summary Label
    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #d4d4d4; padding: 4px;");
    mainLayout->addWidget(m_summaryLabel);

    // 6. Main Diff Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"Line A", "Assembly / Output (A)", "Line B", "Assembly / Output (B)"});
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
    mainLayout->addWidget(m_table, 1);

    // 7. Close Button
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
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
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);
}

void DiffDialog::updatePaneInfoLabels() {
    int idxA = m_comboA ? m_comboA->currentIndex() : 0;
    int idxB = m_comboB ? m_comboB->currentIndex() : 1;

    if (idxA >= 0 && idxA < m_panes.size()) {
        const DiffPaneInfo& pA = m_panes[idxA];
        QString infoA = QString("Compiler: %1 | Flags: %2 | Type: %3")
                            .arg(pA.compilerName.isEmpty() ? "N/A" : pA.compilerName,
                                 pA.compilerFlags.isEmpty() ? "(none)" : pA.compilerFlags,
                                 artifactTypeToString(pA.artifactType));
        m_infoLabelA->setText(infoA);
    } else {
        m_infoLabelA->setText("");
    }

    if (idxB >= 0 && idxB < m_panes.size()) {
        const DiffPaneInfo& pB = m_panes[idxB];
        QString infoB = QString("Compiler: %1 | Flags: %2 | Type: %3")
                            .arg(pB.compilerName.isEmpty() ? "N/A" : pB.compilerName,
                                 pB.compilerFlags.isEmpty() ? "(none)" : pB.compilerFlags,
                                 artifactTypeToString(pB.artifactType));
        m_infoLabelB->setText(infoB);
    } else {
        m_infoLabelB->setText("");
    }

    // Check for artifact type disparity
    if (idxA >= 0 && idxA < m_panes.size() && idxB >= 0 && idxB < m_panes.size()) {
        if (m_panes[idxA].artifactType != m_panes[idxB].artifactType) {
            m_typeWarningLabel->setText(
                QString("⚠️ Warning: Comparing different artifact formats (%1 vs %2). Line-by-line machine instruction comparison may not be semantically equivalent.")
                    .arg(artifactTypeToString(m_panes[idxA].artifactType),
                         artifactTypeToString(m_panes[idxB].artifactType)));
            m_typeWarningLabel->setVisible(true);
        } else {
            m_typeWarningLabel->setVisible(false);
        }
    }
}

void DiffDialog::onPaneSelectionChanged() {
    updatePaneInfoLabels();
    onRecomputeDiff();
}

void DiffDialog::onRecomputeDiff() {
    int idxA = m_comboA ? m_comboA->currentIndex() : 0;
    int idxB = m_comboB ? m_comboB->currentIndex() : 1;

    QString asmA = (idxA >= 0 && idxA < m_panes.size()) ? m_panes[idxA].content : "";
    QString asmB = (idxB >= 0 && idxB < m_panes.size()) ? m_panes[idxB].content : "";

    DiffOptions options;
    options.ignoreWhitespace = m_ignoreWhitespaceCheck ? m_ignoreWhitespaceCheck->isChecked() : false;
    options.ignoreComments = m_ignoreCommentsCheck ? m_ignoreCommentsCheck->isChecked() : false;
    options.normalizeLabels = m_normalizeLabelsCheck ? m_normalizeLabelsCheck->isChecked() : false;

    m_currentDiff = AssemblyDiffProcessor::computeDiff(asmA, asmB, options);
    populateTable();

    if (m_searchInput && !m_searchInput->text().isEmpty()) {
        onFindText(m_searchInput->text());
    }
}

void DiffDialog::populateTable() {
    m_diffRows.clear();
    m_currentDiffIndex = -1;

    QString summary = QString("Additions: <span style='color: #4ec9b0;'>+%1</span> | "
                              "Deletions: <span style='color: #f48771;'>-%2</span> | "
                              "Unchanged: %3 | Time: %4 ms")
                          .arg(m_currentDiff.additions)
                          .arg(m_currentDiff.deletions)
                          .arg(m_currentDiff.unchanged)
                          .arg(m_currentDiff.durationMs);

    if (m_currentDiff.isTruncated) {
        summary += " | <span style='color: #e5c07b;'>[Truncated: " + m_currentDiff.statusMessage + "]</span>";
    }
    m_summaryLabel->setText(summary);

    m_table->setRowCount(m_currentDiff.entries.size());

    const QColor addedColor(40, 80, 50, 140);
    const QColor removedColor(80, 35, 35, 140);

    for (int i = 0; i < m_currentDiff.entries.size(); ++i) {
        const DiffEntry& entry = m_currentDiff.entries[i];

        if (entry.type != DiffLineType::Unchanged) {
            m_diffRows.append(i);
        }

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

    updateTableVisibility();
}

void DiffDialog::updateTableVisibility() {
    bool hideUnchanged = m_hideUnchangedCheck && m_hideUnchangedCheck->isChecked();

    for (int i = 0; i < m_currentDiff.entries.size(); ++i) {
        bool isUnchanged = (m_currentDiff.entries[i].type == DiffLineType::Unchanged);
        m_table->setRowHidden(i, hideUnchanged && isUnchanged);
    }
}

void DiffDialog::onFindText(const QString& query) {
    m_searchMatchingRows.clear();
    m_currentSearchIndex = -1;

    if (query.trimmed().isEmpty()) {
        m_searchStatusLabel->setText("");
        highlightSearchResults();
        return;
    }

    QString lowerQuery = query.toLower();
    for (int i = 0; i < m_currentDiff.entries.size(); ++i) {
        const auto& entry = m_currentDiff.entries[i];
        if (entry.textA.toLower().contains(lowerQuery) || entry.textB.toLower().contains(lowerQuery)) {
            m_searchMatchingRows.append(i);
        }
    }

    if (m_searchMatchingRows.isEmpty()) {
        m_searchStatusLabel->setText("0 matches");
    } else {
        m_currentSearchIndex = 0;
        m_searchStatusLabel->setText(QString("1 of %1").arg(m_searchMatchingRows.size()));
        scrollToRow(m_searchMatchingRows[0]);
    }

    highlightSearchResults();
}

void DiffDialog::onFindNext() {
    if (m_searchMatchingRows.isEmpty()) return;
    m_currentSearchIndex = (m_currentSearchIndex + 1) % m_searchMatchingRows.size();
    m_searchStatusLabel->setText(QString("%1 of %2").arg(m_currentSearchIndex + 1).arg(m_searchMatchingRows.size()));
    scrollToRow(m_searchMatchingRows[m_currentSearchIndex]);
}

void DiffDialog::onFindPrev() {
    if (m_searchMatchingRows.isEmpty()) return;
    m_currentSearchIndex = (m_currentSearchIndex - 1 + m_searchMatchingRows.size()) % m_searchMatchingRows.size();
    m_searchStatusLabel->setText(QString("%1 of %2").arg(m_currentSearchIndex + 1).arg(m_searchMatchingRows.size()));
    scrollToRow(m_searchMatchingRows[m_currentSearchIndex]);
}

void DiffDialog::onNextDiff() {
    if (m_diffRows.isEmpty()) return;
    m_currentDiffIndex = (m_currentDiffIndex + 1) % m_diffRows.size();
    scrollToRow(m_diffRows[m_currentDiffIndex]);
}

void DiffDialog::onPrevDiff() {
    if (m_diffRows.isEmpty()) return;
    m_currentDiffIndex = (m_currentDiffIndex - 1 + m_diffRows.size()) % m_diffRows.size();
    scrollToRow(m_diffRows[m_currentDiffIndex]);
}

void DiffDialog::scrollToRow(int row) {
    if (row >= 0 && row < m_table->rowCount()) {
        m_table->setRowHidden(row, false); // Make sure it's visible even if unchanged
        m_table->selectRow(row);
        m_table->scrollToItem(m_table->item(row, 1), QAbstractItemView::PositionAtCenter);
    }
}

void DiffDialog::highlightSearchResults() {
    // Reset background highlights
    const QColor addedColor(40, 80, 50, 140);
    const QColor removedColor(80, 35, 35, 140);
    const QColor searchMatchColor(100, 90, 30, 180);

    for (int i = 0; i < m_table->rowCount(); ++i) {
        DiffLineType type = (i < m_currentDiff.entries.size()) ? m_currentDiff.entries[i].type : DiffLineType::Unchanged;
        bool isMatch = m_searchMatchingRows.contains(i);

        for (int c = 0; c < 4; ++c) {
            auto* item = m_table->item(i, c);
            if (item) {
                if (isMatch) {
                    item->setBackground(searchMatchColor);
                } else if (type == DiffLineType::Added && (c == 2 || c == 3)) {
                    item->setBackground(addedColor);
                } else if (type == DiffLineType::Removed && (c == 0 || c == 1)) {
                    item->setBackground(removedColor);
                } else {
                    item->setBackground(QBrush());
                }
            }
        }
    }
}

void DiffDialog::onCopyUnifiedDiff() {
    QString labelA = m_comboA ? m_comboA->currentText() : "Pane A";
    QString labelB = m_comboB ? m_comboB->currentText() : "Pane B";
    QString patch = AssemblyDiffProcessor::toUnifiedDiff(m_currentDiff, labelA, labelB);

    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(patch);

    QMessageBox::information(this, "Diff Copied", "Unified diff has been copied to your clipboard.");
}

void DiffDialog::onExportUnifiedDiff() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Unified Diff", "", "Patch Files (*.patch *.diff);;All Files (*)");

    if (fileName.isEmpty()) return;

    QString labelA = m_comboA ? m_comboA->currentText() : "Pane A";
    QString labelB = m_comboB ? m_comboB->currentText() : "Pane B";
    QString patch = AssemblyDiffProcessor::toUnifiedDiff(m_currentDiff, labelA, labelB);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Error", "Could not write to file: " + fileName);
        return;
    }

    QTextStream out(&file);
    out << patch;
    file.close();

    QMessageBox::information(this, "Diff Exported", "Unified diff successfully saved to:\n" + fileName);
}

#ifndef DIFFDIALOG_H
#define DIFFDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QVector>

#include "../core/CompilationArtifact.h"
#include "../core/CompilerEnums.h"
#include "../utils/AssemblyDiffProcessor.h"

struct DiffPaneInfo {
    quint64 paneId{0};
    QString title;
    QString compilerName;
    QString compilerFlags;
    Language language{Language::Cpp};
    SyntaxFlavor syntaxFlavor{SyntaxFlavor::Intel};
    QString content;
    ArtifactType artifactType{ArtifactType::Assembly};
};

class DiffDialog : public QDialog {
    Q_OBJECT

public:
    explicit DiffDialog(const QVector<DiffPaneInfo>& panes,
                        int initialIndexA = 0, int initialIndexB = 1,
                        QWidget* parent = nullptr);

    // Backwards-compatible constructor
    explicit DiffDialog(const QString& nameA, const QString& asmA,
                        const QString& nameB, const QString& asmB,
                        QWidget* parent = nullptr);

    ~DiffDialog() override = default;

private slots:
    void onPaneSelectionChanged();
    void onRecomputeDiff();
    void onFindText(const QString& query);
    void onFindNext();
    void onFindPrev();
    void onNextDiff();
    void onPrevDiff();
    void onExportUnifiedDiff();
    void onCopyUnifiedDiff();

private:
    void setupUi();
    void updatePaneInfoLabels();
    void populateTable();
    void updateTableVisibility();
    void highlightSearchResults();
    void scrollToRow(int row);

    QVector<DiffPaneInfo> m_panes;
    DiffResult m_currentDiff;

    QComboBox* m_comboA{nullptr};
    QComboBox* m_comboB{nullptr};
    QLabel* m_infoLabelA{nullptr};
    QLabel* m_infoLabelB{nullptr};
    QLabel* m_typeWarningLabel{nullptr};

    QCheckBox* m_hideUnchangedCheck{nullptr};
    QCheckBox* m_ignoreWhitespaceCheck{nullptr};
    QCheckBox* m_ignoreCommentsCheck{nullptr};
    QCheckBox* m_normalizeLabelsCheck{nullptr};

    QLineEdit* m_searchInput{nullptr};
    QLabel* m_searchStatusLabel{nullptr};
    QPushButton* m_prevSearchBtn{nullptr};
    QPushButton* m_nextSearchBtn{nullptr};

    QPushButton* m_prevDiffBtn{nullptr};
    QPushButton* m_nextDiffBtn{nullptr};
    QPushButton* m_exportBtn{nullptr};
    QPushButton* m_copyBtn{nullptr};

    QTableWidget* m_table{nullptr};
    QLabel* m_summaryLabel{nullptr};

    QVector<int> m_diffRows;
    QVector<int> m_searchMatchingRows;
    int m_currentSearchIndex{-1};
    int m_currentDiffIndex{-1};
};

#endif // DIFFDIALOG_H

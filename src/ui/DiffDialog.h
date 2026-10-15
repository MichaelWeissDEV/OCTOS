#ifndef DIFFDIALOG_H
#define DIFFDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QTableView>
#include <QThreadPool>
#include <QVector>
#include <memory>

#include "../core/CompilationArtifact.h"
#include "../core/CompilerEnums.h"
#include "../utils/AssemblyDiffProcessor.h"
#include "DiffTableModel.h"

struct DiffPaneInfo {
    quint64 paneId{0};
    QString title;
    QString compilerName;
    QString compilerFlags;
    Language language{Language::Cpp};
    SyntaxFlavor syntaxFlavor{SyntaxFlavor::Intel};
    QString content;
    ArtifactType artifactType{ArtifactType::Assembly};
    QString architecture;  // e.g. "x86_64", "aarch64"; empty = detect from content
};

/**
 * Diff viewer. The comparison runs on a worker thread (QPromise/QFutureWatcher), the table is a
 * virtual model. Every option change cancels the running computation and starts a new one; a
 * result is only applied if it belongs to the newest request.
 */
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

    ~DiffDialog() override;

    bool isComputing() const { return m_activeJob != 0; }
    const DiffResult* currentResult() const { return m_model ? m_model->result() : nullptr; }

public slots:
    void done(int r) override;

private slots:
    void onPaneSelectionChanged();
    void onRecomputeDiff();
    void onCancelCompute();
    void onFindText(const QString& query);
    void onFindNext();
    void onFindPrev();
    void onNextDiff();
    void onPrevDiff();
    void onExportUnifiedDiff();
    void onCopyUnifiedDiff();
    void onCopySelection();

private:
    struct Job {
        quint64 id{0};
        QFutureWatcher<DiffResult>* watcher{nullptr};
    };

    void setupUi();
    void updatePaneInfoLabels();
    void updateControlsEnabled();
    void showResult(DiffResult result);
    void cancelAllJobs();
    void onJobFinished(quint64 jobId);
    QString patchOrAsk(bool* ok);
    void scrollToSourceRow(int sourceRow);
    void setSummary(const DiffResult& r);
    AsmDialect dialectFor(const DiffPaneInfo& pa, const DiffPaneInfo& pb) const;

    QVector<DiffPaneInfo> m_panes;

    // Computation state
    QThreadPool m_pool;
    QVector<Job> m_jobs;
    quint64 m_nextJobId{1};
    quint64 m_activeJob{0};  // newest request; 0 = nothing running
    bool m_closing{false};

    QComboBox* m_comboA{nullptr};
    QComboBox* m_comboB{nullptr};
    QLabel* m_infoLabelA{nullptr};
    QLabel* m_infoLabelB{nullptr};
    QLabel* m_typeWarningLabel{nullptr};
    QLabel* m_statusLabel{nullptr};
    QProgressBar* m_progress{nullptr};
    QPushButton* m_cancelBtn{nullptr};

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

    QTableView* m_table{nullptr};
    DiffTableModel* m_model{nullptr};
    DiffFilterProxy* m_proxy{nullptr};
    QLabel* m_summaryLabel{nullptr};

    QVector<int> m_diffRows;           // source rows
    QVector<int> m_searchMatchingRows; // source rows
    int m_currentSearchIndex{-1};
    int m_currentDiffIndex{-1};
};

#endif // DIFFDIALOG_H

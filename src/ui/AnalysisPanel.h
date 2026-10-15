#ifndef ANALYSISPANEL_H
#define ANALYSISPANEL_H

#include <QComboBox>
#include <QFutureWatcher>
#include <QLabel>
#include <QMap>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QWidget>

#include "../parser/AssemblyModel.h"

// Structured view of an assembly listing built from the AssemblyModel (one parser, no extra regexes):
// function list, basic blocks with their control-flow successors, instruction statistics, metrics.
class AnalysisPanel : public QWidget {
    Q_OBJECT

public:
    explicit AnalysisPanel(QWidget* parent = nullptr);
    ~AnalysisPanel() override;

    // Registers (or replaces) the assembly of an output pane. The first registered pane is shown.
    void setPaneAssembly(quint64 paneId, const QString& title, const QString& rawAssembly);
    void removePane(quint64 paneId);
    void clear();

    bool isAnalyzing() const { return m_activeJob != 0; }
    const AssemblyModule* currentModule() const { return m_module.get(); }
    QTreeWidget* functionTree() const { return m_functions; }
    QTreeWidget* blockTree() const { return m_blocks; }

signals:
    // 1-based line of the ORIGINAL (unfiltered) assembly text, and the pane it belongs to.
    void assemblyLineActivated(quint64 paneId, int line);
    // 1-based line of the source file as referenced by .loc, 0 if unknown.
    void sourceLineActivated(int line);

private slots:
    void onPaneSelected(int index);
    void onFunctionSelected();
    void onFunctionActivated(QTreeWidgetItem* item, int column);
    void onBlockActivated(QTreeWidgetItem* item, int column);

private:
    struct Entry { QString title; QString text; };

    void startAnalysis();
    void showModule(std::shared_ptr<AssemblyModule> module);
    void populateMetrics();
    void cancelJob();

    QComboBox* m_paneCombo{nullptr};
    QLabel* m_summary{nullptr};
    QLabel* m_status{nullptr};
    QTreeWidget* m_functions{nullptr};
    QTreeWidget* m_blocks{nullptr};
    QTableWidget* m_frequencies{nullptr};
    QLabel* m_metrics{nullptr};

    QMap<quint64, Entry> m_entries;
    quint64 m_currentPane{0};
    std::shared_ptr<AssemblyModule> m_module;

    QFutureWatcher<std::shared_ptr<AssemblyModule>>* m_watcher{nullptr};
    quint64 m_nextJob{1};
    quint64 m_activeJob{0};
};

#endif // ANALYSISPANEL_H

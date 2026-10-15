#ifndef DIFFTABLEMODEL_H
#define DIFFTABLEMODEL_H

#include <QAbstractTableModel>
#include <QSet>
#include <QSortFilterProxyModel>
#include <memory>
#include "../utils/AssemblyDiffProcessor.h"

// Virtual model over a DiffResult: rows are produced on demand by the view, so a 100k-entry
// diff costs no per-cell objects (the previous QTableWidget created 4 items per row).
class DiffTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { LineA = 0, TextA, LineB, TextB, ColumnCount };

    explicit DiffTableModel(QObject* parent = nullptr);

    void setResult(std::shared_ptr<const DiffResult> result);
    const DiffResult* result() const { return m_result.get(); }
    const DiffEntry* entryAt(int row) const;

    void setSearchMatches(const QSet<int>& rows);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    /// Tab-separated text of one row (used for copy).
    QString rowText(int row) const;

private:
    std::shared_ptr<const DiffResult> m_result;
    QSet<int> m_matches;
};

class DiffFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    void setOnlyDifferences(bool on);
    bool onlyDifferences() const { return m_only; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& parent) const override;

private:
    bool m_only{false};
};

#endif // DIFFTABLEMODEL_H

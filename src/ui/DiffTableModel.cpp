#include "DiffTableModel.h"
#include <QBrush>
#include <QColor>
#include <QFontDatabase>

#include "Theme.h"

DiffTableModel::DiffTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void DiffTableModel::setResult(std::shared_ptr<const DiffResult> result) {
    beginResetModel();
    m_result = std::move(result);
    m_matches.clear();
    endResetModel();
}

const DiffEntry* DiffTableModel::entryAt(int row) const {
    if (!m_result || row < 0 || row >= m_result->entries.size()) return nullptr;
    return &m_result->entries[row];
}

void DiffTableModel::setSearchMatches(const QSet<int>& rows) {
    if (rows == m_matches) return;
    QSet<int> changed = m_matches;
    changed.unite(rows);
    m_matches = rows;
    for (int r : std::as_const(changed)) {
        emit dataChanged(index(r, 0), index(r, ColumnCount - 1), {Qt::BackgroundRole});
    }
}

int DiffTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() || !m_result ? 0 : m_result->entries.size();
}

int DiffTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant DiffTableModel::data(const QModelIndex& idx, int role) const {
    const DiffEntry* e = entryAt(idx.row());
    if (!e) return {};
    switch (role) {
        case Qt::DisplayRole:
            switch (idx.column()) {
                case LineA: return e->lineIndexA >= 0 ? QVariant(e->lineIndexA + 1) : QVariant(QString());
                case TextA: return e->type == DiffLineType::Added ? QString() : e->textA;
                case LineB: return e->lineIndexB >= 0 ? QVariant(e->lineIndexB + 1) : QVariant(QString());
                case TextB: return e->type == DiffLineType::Removed ? QString() : e->textB;
            }
            break;
        case Qt::TextAlignmentRole:
            if (idx.column() == LineA || idx.column() == LineB) return int(Qt::AlignRight | Qt::AlignVCenter);
            break;
        case Qt::FontRole:
            return QFontDatabase::systemFont(QFontDatabase::FixedFont);
        case Qt::BackgroundRole:
            if (m_matches.contains(idx.row())) return QBrush(Theme::currentTokens().searchMatch);
            if (e->type == DiffLineType::Added && (idx.column() == LineB || idx.column() == TextB)) return QBrush(Theme::currentTokens().diffAdded);
            if (e->type == DiffLineType::Removed && (idx.column() == LineA || idx.column() == TextA)) return QBrush(Theme::currentTokens().diffRemoved);
            break;
    }
    return {};
}

QVariant DiffTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    static const char* names[] = {"Line A", "Assembly / Output (A)", "Line B", "Assembly / Output (B)"};
    return section >= 0 && section < ColumnCount ? QString(names[section]) : QString();
}

QString DiffTableModel::rowText(int row) const {
    const DiffEntry* e = entryAt(row);
    if (!e) return {};
    return QString("%1\t%2\t%3\t%4")
        .arg(e->lineIndexA >= 0 ? QString::number(e->lineIndexA + 1) : QString(),
             e->type == DiffLineType::Added ? QString() : e->textA,
             e->lineIndexB >= 0 ? QString::number(e->lineIndexB + 1) : QString(),
             e->type == DiffLineType::Removed ? QString() : e->textB);
}

void DiffFilterProxy::setOnlyDifferences(bool on) {
    if (m_only == on) return;
    m_only = on;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    beginFilterChange();
    endFilterChange();
#else
    invalidateFilter();
#endif
}

bool DiffFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex&) const {
    if (!m_only) return true;
    auto* m = qobject_cast<DiffTableModel*>(sourceModel());
    const DiffEntry* e = m ? m->entryAt(sourceRow) : nullptr;
    return !e || e->type != DiffLineType::Unchanged;
}

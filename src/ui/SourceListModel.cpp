// Tankoban 3 - see SourceListModel.h.
#include "ui/SourceListModel.h"
#include "ui/StreamRowPaint.h"

#include <QSet>

namespace tankoban {

SourceListModel::SourceListModel(QObject* parent) : QAbstractListModel(parent) {}

int SourceListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant SourceListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& r = m_rows[index.row()];
    if (role == StreamRole)
        return QVariant::fromValue(r.stream);
    if (role == RankRole)
        return r.rank;
    return {};
}

ScoredStream SourceListModel::at(int row) const
{
    return (row >= 0 && row < m_rows.size()) ? m_rows[row].stream : ScoredStream{};
}

int SourceListModel::rankAt(int row) const
{
    return (row >= 0 && row < m_rows.size()) ? m_rows[row].rank : -1;
}

void SourceListModel::reindex()
{
    m_indexByKey.clear();
    for (int i = 0; i < m_rows.size(); ++i)
        m_indexByKey.insert(streamRowKey(m_rows[i].stream), i);
}

void SourceListModel::setStreams(const QVector<ScoredStream>& streams)
{
    // 1) Dedupe incoming by key (keep the first/highest-ranked occurrence) and stamp
    //    each kept stream with rank = its position in the ranked input.
    QVector<ScoredStream> ranked;
    QSet<QString> seen;
    ranked.reserve(streams.size());
    for (const ScoredStream& s : streams) {
        const QString key = streamRowKey(s);
        if (seen.contains(key))
            continue;
        seen.insert(key);
        ranked.push_back(s);
    }

    // 2) Update existing rows in place (stream + rank); collect genuinely-new ones.
    QSet<QString> incomingKeys;
    incomingKeys.reserve(ranked.size());
    QVector<Row> fresh;
    for (int rank = 0; rank < ranked.size(); ++rank) {
        const ScoredStream& s = ranked[rank];
        const QString key = streamRowKey(s);
        incomingKeys.insert(key);
        const auto it = m_indexByKey.constFind(key);
        if (it != m_indexByKey.constEnd()) {
            Row& row = m_rows[it.value()];
            row.stream = s;
            row.rank = rank;
            const QModelIndex idx = index(it.value());
            emit dataChanged(idx, idx, {StreamRole, RankRole});
        } else {
            fresh.push_back({s, rank});
        }
    }

    // 3) Remove rows whose key vanished (back to front so indices stay valid).
    for (int i = m_rows.size() - 1; i >= 0; --i) {
        if (!incomingKeys.contains(streamRowKey(m_rows[i].stream))) {
            beginRemoveRows(QModelIndex(), i, i);
            m_rows.remove(i);
            endRemoveRows();
        }
    }

    // 4) Append the fresh rows.
    if (!fresh.isEmpty()) {
        const int first = m_rows.size();
        beginInsertRows(QModelIndex(), first, first + fresh.size() - 1);
        m_rows += fresh;
        endInsertRows();
    }

    reindex();
}

void SourceListModel::clear()
{
    if (m_rows.isEmpty())
        return;
    beginResetModel();
    m_rows.clear();
    m_indexByKey.clear();
    endResetModel();
}

} // namespace tankoban

// Tankoban 3 - see SourceFilterProxy.h.
#include "ui/SourceFilterProxy.h"
#include "ui/SourceListModel.h"
#include "ui/StreamRowPaint.h"

namespace tankoban {

SourceFilterProxy::SourceFilterProxy(QObject* parent) : QSortFilterProxyModel(parent)
{
    setSortRole(SourceListModel::RankRole);   // sort by buildPicker rank, NOT score-only
    setDynamicSortFilter(true);
    sort(0, Qt::AscendingOrder);              // lower rank = better -> earlier
}

void SourceFilterProxy::setAddonFilter(const QString& key)
{
    if (m_addon == key)
        return;
    m_addon = key;
    invalidateFilter();
}

void SourceFilterProxy::setQualityFilter(const QString& group)
{
    if (m_quality == group)
        return;
    m_quality = group;
    invalidateFilter();
}

bool SourceFilterProxy::lessThan(const QModelIndex& l, const QModelIndex& r) const
{
    return l.data(SourceListModel::RankRole).toInt() < r.data(SourceListModel::RankRole).toInt();
}

bool SourceFilterProxy::filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const
{
    const QModelIndex idx = sourceModel()->index(srcRow, 0, srcParent);
    const auto s = idx.data(SourceListModel::StreamRole).value<ScoredStream>();
    if (m_addon != QStringLiteral("all") && addonInstanceKey(s) != m_addon)
        return false;
    if (m_quality != QStringLiteral("all") && qualityGroupOf(s) != m_quality)
        return false;
    return true;
}

} // namespace tankoban

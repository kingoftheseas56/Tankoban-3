// Tankoban 3 - see SourceListView.h.
#include "ui/SourceListView.h"
#include "ui/SourceListModel.h"
#include "ui/SourceRowDelegate.h"
#include "ui/StreamRowPaint.h"

#include <QMouseEvent>

namespace tankoban {

SourceListView::SourceListView(QWidget* parent) : QListView(parent)
{
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    setSelectionMode(QAbstractItemView::NoSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setUniformItemSizes(true);   // fixed row height -> fast virtualization
    setSpacing(8);
    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setResizeMode(QListView::Adjust);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored); // never expand to all 500 rows
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(420);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

SourceRowDelegate* SourceListView::rowDelegate() const
{
    return qobject_cast<SourceRowDelegate*>(itemDelegate());
}

void SourceListView::mouseMoveEvent(QMouseEvent* e)
{
    const QModelIndex idx = indexAt(e->pos());
    // Hover key derived from the stream (model exposes StreamRole/RankRole, no KeyRole);
    // delegate compares against streamRowKey() so hover never jumps row under live updates.
    const auto s = idx.isValid() ? idx.data(SourceListModel::StreamRole).value<ScoredStream>() : ScoredStream{};
    if (auto* d = rowDelegate()) { d->setHoveredKey(idx.isValid() ? streamRowKey(s) : QString()); viewport()->update(); }
    const QRect row = visualRect(idx);
    const bool playable = idx.isValid() && isPlayable(s);
    const bool copyable = idx.isValid()
        && ((!s.url.isEmpty() && s.url != QStringLiteral("#")) || !s.externalUrl.isEmpty());
    const bool overBtn = idx.isValid()
        && ((playable && SourceRowDelegate::playRect(row).contains(e->pos()))
            || (copyable && SourceRowDelegate::copyRect(row).contains(e->pos())));
    setCursor(overBtn ? Qt::PointingHandCursor : Qt::ArrowCursor);
    QListView::mouseMoveEvent(e);
}

void SourceListView::mouseReleaseEvent(QMouseEvent* e)
{
    const QModelIndex idx = indexAt(e->pos());
    if (idx.isValid()) {
        const QRect row = visualRect(idx);
        const auto s = idx.data(SourceListModel::StreamRole).value<ScoredStream>();
        const bool playable = isPlayable(s);
        const bool copyable = ((!s.url.isEmpty() && s.url != QStringLiteral("#")) || !s.externalUrl.isEmpty());
        if (playable && SourceRowDelegate::playRect(row).contains(e->pos())) { emit playActivated(idx); return; }
        if (copyable && SourceRowDelegate::copyRect(row).contains(e->pos())) { emit copyActivated(idx); return; }
    }
    QListView::mouseReleaseEvent(e);
}

void SourceListView::leaveEvent(QEvent* e)
{
    if (auto* d = rowDelegate()) { d->setHoveredKey(QString()); viewport()->update(); }
    QListView::leaveEvent(e);
}

} // namespace tankoban

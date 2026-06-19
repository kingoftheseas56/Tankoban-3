// Tankoban 3 - ranks + filters the SourceListModel for the view. Sorts by RankRole
// (preserves MainWindow::buildPicker() ordering, NOT score-only) and filters by addon
// + quality. Filter changes are O(visible) repaints, not widget rebuilds.
#pragma once

#include <QSortFilterProxyModel>
#include <QString>

namespace tankoban {

class SourceFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SourceFilterProxy(QObject* parent = nullptr);

    void setAddonFilter(const QString& key);     // "all" or an addonInstanceKey
    void setQualityFilter(const QString& group); // "all" / "4K" / "1080p" / "720p" / "SD"

protected:
    bool lessThan(const QModelIndex& l, const QModelIndex& r) const override;
    bool filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const override;

private:
    QString m_addon = QStringLiteral("all");
    QString m_quality = QStringLiteral("all");
};

} // namespace tankoban

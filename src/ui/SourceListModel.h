// Tankoban 3 - source rows as DATA (no widgets). Incremental merge by stream key so
// streaming partials never rebuild the view. ScoredStream is exposed via StreamRole;
// the incoming buildPicker() ranking is preserved per-row via RankRole (the proxy
// sorts by RankRole ascending, NOT score-only) so respectAddonOrder/scorer nuance hold.
#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

#include "core/StreamModels.h"

namespace tankoban {

class SourceListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { StreamRole = Qt::UserRole + 1, RankRole };

    explicit SourceListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    // Merge the full RANKED set: dedupe incoming by key, assign rank = order, insert
    // new keys, update changed rows in place, remove keys no longer present. Never a
    // blanket reset (preserves view state); emits dataChanged({StreamRole, RankRole}).
    void setStreams(const QVector<ScoredStream>& streams);
    void clear();

    ScoredStream at(int row) const;   // convenience for delegate/tests
    int rankAt(int row) const;

private:
    struct Row { ScoredStream stream; int rank = 0; };
    QVector<Row> m_rows;                 // arrival order; the proxy sorts by RankRole
    QHash<QString, int> m_indexByKey;    // streamRowKey -> row index
    void reindex();
};

} // namespace tankoban

// NOTE: no Q_DECLARE_METATYPE(ScoredStream) — Qt6 auto-registers the metatype from its
// use as a signal parameter (StreamList::streamActivated), so an explicit specialization
// here collides ("QMetaTypeId already instantiated", C2908). QVariant::fromValue works
// without it in Qt6.

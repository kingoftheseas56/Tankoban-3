// Tankoban 3 — DiscoverPage (main sidebar Discover route).
//
// Harbor's Discover view (src/views/discover.tsx) is NOT the Movies page: it has no
// CinemaHero masthead — its top widget is the FeaturedBanner, which Tankoban 3 v1 defers
// (no-key scope) — so Discover is a pure, padded stack of lazy poster shelves. We recreate
// the no-key path faithfully: Harbor's themes.ts fallbackShelves() = Cinemeta Top Movies +
// Top Series + five daily-seeded genre shelves (movie/top/genre=<g>), deduped across rows
// (use-deduped-rows.ts MIN_ROW backfill) so genre rows that overlap Top Movies stay full.
// Reuses CatalogRow + PosterCard + the View-all -> GridPage wiring + BackToTop, 1:1 with the
// Anime/Movies routes. Harbor's TMDB/taste/Featured/CriticsPick/Queue/Award machinery is out
// of v1 scope and intentionally absent (not faked).
//
// Spacing is Harbor discover.tsx exactly: px-12 / pt-20 / pb-20 (48 / 80 / 80) + gap-14 (56).

#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

namespace tankoban {

class AddonClient;
class CatalogRow;

class DiscoverPage : public QWidget {
    Q_OBJECT
public:
    explicit DiscoverPage(QWidget* parent = nullptr);

signals:
    void openDetailRequested(const MetaItem& meta);
    void openGridRequested(const QString& title, const QVector<MetaItem>& items);

private:
    struct RowDef {
        QString key;
        QString title;
        QString base;
        QString type;
        QString catalogId;
    };

    // Harbor fallbackShelves(): Top Movies + Top Series + 5 daily-seeded genres (movie).
    static QVector<RowDef> buildShelves();
    void render(); // dedup in route order (+ MIN_ROW backfill); populate once all rows arrive

    QVector<RowDef> m_defs;
    AddonClient* m_addons = nullptr;
    QVector<CatalogRow*> m_rowWidgets;          // parallel to m_defs
    QHash<QString, QVector<MetaItem>> m_items;  // RowDef.key -> fetched metas (poster-filtered)
    int m_pending = 0;
};

} // namespace tankoban

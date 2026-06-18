// Tankoban 3 — RowPage (Screen Build Order #5: Catalog Aggregation).
//
// A route-parameterized catalogue page for the sidebar content routes
// (Movies / Shows / Anime / Discover). It is the row stack from HomePage, generalized:
// a featured hero over a vertical stack of lazy poster shelves. Only the ROW SET differs
// per route — the rendering reuses HomePage's CatalogRow + PosterCard + lazy load 1:1.
//
// Fidelity: Tankoban 3 cuts TMDB (ROADMAP "Locked Product Decisions"), so each route
// follows Harbor's no-TMDB-key branch verbatim — exactly what Harbor itself renders with
// no key present. Movies = Cinemeta Top Movies + genre shelves (harbor-ref movies.tsx);
// Shows = the series equivalent (shows.tsx). Rows dedup in route order (seeded by the
// hero), and a row that falls below 4 items after dedup is omitted — matching Harbor's
// restRows useMemo. Anime / Discover row sets land in later increments.

#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

namespace tankoban {

class AddonClient;
class CatalogRow;
class FeaturedHero;

class RowPage : public QWidget {
    Q_OBJECT
public:
    // routeId: "movies" | "shows" | "anime" | "discover".
    explicit RowPage(const QString& routeId, QWidget* parent = nullptr);

signals:
    void openDetailRequested(const MetaItem& meta);
    // A row's "View all" was clicked: open a full-category grid for this title + list.
    void openGridRequested(const QString& title, const QVector<MetaItem>& items);

private:
    struct RowDef {
        QString key;        // unique tag (AddonClient rowKey + item-map key)
        QString title;      // shelf heading
        QString base;       // addon base URL (no trailing slash)
        QString type;       // "movie" | "series"
        QString catalogId;  // e.g. "top" or "top/genre=Action"
    };

    static QVector<RowDef> rowsForRoute(const QString& routeId);
    // Populate the hero + shelves once every fetch has resolved (Harbor renders after a
    // single Promise.all, with cross-row dedup) — render() runs exactly once.
    void render();

    QString m_routeId;
    AddonClient* m_addons = nullptr;
    FeaturedHero* m_hero = nullptr;
    QVector<RowDef> m_defs;
    QVector<CatalogRow*> m_rowWidgets;          // parallel to m_defs (by index)
    QHash<QString, QVector<MetaItem>> m_items;  // RowDef.key -> fetched metas
    int m_pending = 0;
    int m_heroTarget = 5;                        // Harbor HERO_POOL_TARGET (movies 5 / shows 6)
    QString m_heroType = QStringLiteral("movie");
};

} // namespace tankoban

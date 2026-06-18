// Tankoban 3 — ShowsPage (Shows sidebar route).
//
// Harbor's Shows view is deliberately NOT Movies: a PageMast (time-of-day copy) over a
// PeekHero landscape carousel, then portrait series rows. Faithful to harbor-ref
// src/views/shows.tsx under the no-TMDB-key path (the only path Tankoban 3 runs, since
// TMDB is cut): Cinemeta `topSeries` + genre shelves; hero = top series with backdrops;
// masthead copy from shows/hero-curation.ts `bucketCopy()` (data-source independent).
//
// Reuses CatalogRow + PosterCard + the View-all → GridPage wiring. Deferred vs Harbor:
// Top 10 Series (needs a trending row — absent in the no-key path, as in Harbor),
// Continue Watching (no local history yet), BackToTop.

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
class PeekHero;

class ShowsPage : public QWidget {
    Q_OBJECT
public:
    explicit ShowsPage(QWidget* parent = nullptr);

signals:
    void openDetailRequested(const MetaItem& meta);
    void playRequested(const MetaItem& meta);
    void openGridRequested(const QString& title, const QVector<MetaItem>& items);

private:
    struct RowDef {
        QString key;
        QString title;
        QString base;
        QString type;
        QString catalogId;
    };

    void render(); // dedup in route order + populate once all rows arrive (runs once)

    AddonClient* m_addons = nullptr;
    PeekHero* m_hero = nullptr;
    QVector<RowDef> m_defs;
    QVector<CatalogRow*> m_rowWidgets;          // parallel to m_defs
    QHash<QString, QVector<MetaItem>> m_items;  // RowDef.key -> fetched metas
    int m_pending = 0;
};

} // namespace tankoban

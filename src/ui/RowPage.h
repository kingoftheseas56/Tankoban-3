// Tankoban 3 — RowPage (Movies route).
//
// Harbor's Movies view: a FULL-BLEED CinemaHero masthead, then a centered padded stack of
// lazy poster shelves (Harbor px-12 / pt-12 / gap-12). Faithful to harbor-ref movies.tsx
// under the no-TMDB-key path (the only path Tankoban 3 runs): Cinemeta Top Movies + genre
// shelves; hero = top movies with backdrops. Reuses CatalogRow + PosterCard + the
// View-all -> GridPage wiring. (Shows has its own ShowsPage/PeekHero.)

#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

class QResizeEvent;

namespace tankoban {

class AddonClient;
class CatalogRow;
class CinemaHero;

class RowPage : public QWidget {
    Q_OBJECT
public:
    explicit RowPage(const QString& routeId, QWidget* parent = nullptr);

signals:
    void openDetailRequested(const MetaItem& meta);
    void playRequested(const MetaItem& meta);
    void openGridRequested(const QString& title, const QVector<MetaItem>& items);

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    struct RowDef {
        QString key;
        QString title;
        QString base;
        QString type;
        QString catalogId;
    };

    static QVector<RowDef> rowsForRoute(const QString& routeId);
    void render(); // dedup in route order + populate once all rows arrive (runs once)

    QString m_routeId;
    AddonClient* m_addons = nullptr;
    CinemaHero* m_hero = nullptr;
    QVector<RowDef> m_defs;
    QVector<CatalogRow*> m_rowWidgets;          // parallel to m_defs
    QHash<QString, QVector<MetaItem>> m_items;  // RowDef.key -> fetched metas
    int m_pending = 0;
    int m_heroTarget = 5; // Harbor movies HERO_POOL_TARGET
};

} // namespace tankoban

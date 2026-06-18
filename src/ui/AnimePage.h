// Tankoban 3 — AnimePage (Anime route).
//
// Harbor's Anime view (harbor-ref views/anime.tsx + anime/anime-rows.tsx): an AnimeHero
// masthead over a stack of Jikan-backed poster shelves. Built-in rows come from Jikan
// (free MyAnimeList API, no key) via JikanClient — the no-TMDB anime path. Rows fill
// progressively (Jikan is throttled), hero-feeding rows first. Reuses CatalogRow +
// PosterCard + the View-all -> GridPage wiring.
//
// v1 (V1A): built-in Jikan rows + AnimeHero only. Deferred: installed addon anime rows
// (V1B), Anime Kitsu episode meta (V1C), ARM mal->kitsu mapping, awards/AniList/Simkl.

#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

class QResizeEvent;

namespace tankoban {

class JikanClient;
class CatalogRow;
class AnimeHero;

class AnimePage : public QWidget {
    Q_OBJECT
public:
    explicit AnimePage(QWidget* parent = nullptr);

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
        QString path; // Jikan path after base, e.g. "/seasons/now?page=1"
        bool hero;    // feeds the AnimeHero pool
    };

    void onRow(const QString& key, const QVector<MetaItem>& items);

    AnimeHero* m_hero = nullptr;
    JikanClient* m_jikan = nullptr;
    QVector<RowDef> m_defs;
    QVector<CatalogRow*> m_rowWidgets;          // parallel to m_defs
    QHash<QString, QVector<MetaItem>> m_items;   // key -> fetched metas
    QVector<MetaItem> m_heroPool;
    bool m_heroFull = false;
};

} // namespace tankoban

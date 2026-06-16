// Tankoban 3 — MetaDetail (Detail path).
//
// Full detail metadata from a Stremio addon's /meta endpoint (richer than the
// catalog MetaItem preview): synopsis, genres, and — for series — the episode list.

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace tankoban {

struct EpisodeItem {
    QString id;          // Stremio video id, e.g. "tt123:1:2"
    int season = 0;
    int episode = 0;     // episode-or-number
    QString name;
    QString overview;
    QString thumbnail;   // still image url
    QString released;    // ISO date
};

struct MetaDetail {
    QString id;
    QString type;        // movie | series
    QString name;
    QString poster;
    QString background;
    QString logo;
    QString description;
    QString releaseInfo; // year, or range for series
    QString imdbRating;
    QString runtime;
    QStringList genres;
    QVector<EpisodeItem> videos;
    bool loaded = false; // false until /meta returns
};

} // namespace tankoban

// Tankoban 3 — MetaItem (Step 3).
//
// One catalog/detail item from the Stremio addon protocol (the `metas[]` entry).
// Minimal for now; grows as detail/stream steps need more fields.

#pragma once

#include <QString>
#include <QStringList>

namespace tankoban {

struct MetaItem {
    QString id;          // e.g. "tt1375666"
    QString type;        // "movie" | "series"
    QString name;
    QString poster;      // full-res poster URL
    QString background;  // wide backdrop URL (the hero)
    QString description; // synopsis (the hero)
    QString releaseInfo; // year, e.g. "2026" (the hero stat line)
    QString imdbRating;  // e.g. "8.1" (the hero stat line)
    QString runtime;     // e.g. "141 min" (the hero stat line)
    QStringList genres;  // e.g. ["Drama","Horror"] (the hero meta row)
};

} // namespace tankoban

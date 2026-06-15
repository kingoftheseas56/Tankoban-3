// Tankoban 3 — MetaItem (Step 3).
//
// One catalog/detail item from the Stremio addon protocol (the `metas[]` entry).
// Minimal for now; grows as detail/stream steps need more fields.

#pragma once

#include <QString>

namespace tankoban {

struct MetaItem {
    QString id;          // e.g. "tt1375666"
    QString type;        // "movie" | "series"
    QString name;
    QString poster;      // full-res poster URL
    QString background;  // wide backdrop URL (for the hero, later)
};

} // namespace tankoban

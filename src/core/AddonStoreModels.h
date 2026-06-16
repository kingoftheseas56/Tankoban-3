// Tankoban 3 — AddonStoreModels (Addons marketplace).
//
// A marketplace addon entry from stremio-addons.net (Harbor's Browse/Discover source).
// This is a preview for the store grid — installing uses manifestUrl through the registry,
// which re-fetches the full Stremio manifest.

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace tankoban {

struct StoreAddon {
    QString uuid;
    QString slug;
    QString manifestUrl; // install target (transport URL)
    QString name;
    QString description;
    QString logo;
    int stars = 0;
    QStringList categories;
    bool adult = false;
    bool configurable = false;
};

struct StorePage {
    QVector<StoreAddon> addons;
    int page = 1;
    int totalPages = 1;
    bool hasNext = false;
};

struct StoreCategory {
    QString name;
    QString slug;
};

} // namespace tankoban

// Tankoban 3 — AddonModels (Addons path).
//
// The Stremio addon manifest surface, recreated from Harbor's src/lib/addons.ts.
// A manifest declares which resources (catalog/meta/stream/subtitles) an addon serves
// and for which types/id-prefixes — the registry uses that to route queries.

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace tankoban {

// A manifest resource: either a plain string ("stream") = wildcard over the manifest's
// own types/idPrefixes, or an object with its own types/idPrefixes.
struct AddonResource {
    QString name;          // "catalog" | "meta" | "stream" | "subtitles"
    QStringList types;
    QStringList idPrefixes;
    bool wildcard = false; // true when the manifest listed it as a bare string
};

struct CatalogDef {
    QString id;
    QString type;
    QString name;
    bool hasRequiredExtra = false; // not a Home row until the UI can supply the extra
};

struct Manifest {
    QString id;
    QString name;
    QString version;
    QString description;
    QString logo;
    QString background;
    QStringList types;
    QStringList idPrefixes;
    QVector<AddonResource> resources;
    QVector<CatalogDef> catalogs;
    bool adult = false;
    bool configurable = false;
};

struct InstalledAddon {
    QString manifestUrl; // transport url (…/manifest.json)
    QString baseUrl;     // manifestUrl without the trailing /manifest.json
    Manifest manifest;
    bool enabled = true;
    int order = 0;
};

} // namespace tankoban

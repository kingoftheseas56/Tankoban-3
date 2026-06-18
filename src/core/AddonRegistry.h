// Tankoban 3 — AddonRegistry (Addons path).
//
// The single source of truth for installed Stremio addons: install-by-URL (fetch +
// validate manifest), persist across launches (QSettings), enable/disable, remove, and
// the capability query other services use — forResource("stream", type, id) — recreated
// from Harbor's addonAccepts() in src/lib/addons.ts.

#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "core/AddonModels.h"

class QNetworkAccessManager;

namespace tankoban {

class AddonRegistry : public QObject {
    Q_OBJECT
public:
    explicit AddonRegistry(QObject* parent = nullptr);

    QVector<InstalledAddon> installed() const { return m_addons; }
    QVector<InstalledAddon> enabledAddons() const; // enabled, in user order

    // Enabled addons (in user order) that serve `resource` for this type/id.
    QVector<InstalledAddon> forResource(const QString& resource, const QString& type,
                                        const QString& id) const;

    void load();                                // restore from QSettings
    void seedDefaultsIfNeeded();                // first run: install the curated default addons
    void installFromUrl(const QString& rawUrl); // fetch + validate + install/replace
    void uninstall(const QString& manifestUrl);
    void setEnabled(const QString& manifestUrl, bool enabled);

signals:
    void addonsChanged();
    void installSucceeded(const QString& name, bool replaced);
    void installFailed(const QString& rawUrl, const QString& error);

private:
    void persist() const;

    QVector<InstalledAddon> m_addons;
    QNetworkAccessManager* m_nam = nullptr;
};

} // namespace tankoban

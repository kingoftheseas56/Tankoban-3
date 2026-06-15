// Tankoban 3 — AddonClient (Step 3).
//
// The seed of the addon engine: fetches a Stremio addon catalog over HTTP and parses
// the metas[] into MetaItem[]. Faithful to the Stremio addon protocol
// ({base}/catalog/{type}/{id}.json -> { metas: [...] }). For Step 3 it powers the
// Home rows from Cinemeta; it generalizes to any installed addon + the meta/stream
// endpoints in later steps.

#pragma once

#include <QObject>
#include <QVector>

#include "core/MetaItem.h"

class QNetworkAccessManager;

namespace tankoban {

class AddonClient : public QObject {
    Q_OBJECT
public:
    explicit AddonClient(QObject* parent = nullptr);

    // rowKey is an opaque caller tag echoed back in the result signals so one client
    // can serve several rows. base = addon base URL with no trailing slash.
    void fetchCatalog(const QString& rowKey, const QString& base,
                      const QString& type, const QString& id);

signals:
    void catalogReady(const QString& rowKey, const QVector<tankoban::MetaItem>& items);
    void catalogFailed(const QString& rowKey, const QString& error);

private:
    QNetworkAccessManager* m_nam;
};

} // namespace tankoban

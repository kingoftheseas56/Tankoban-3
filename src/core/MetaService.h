// Tankoban 3 — MetaService (Detail path).
//
// Fetches full detail metadata from a Stremio addon's /meta endpoint (Cinemeta for
// tt/IMDB ids). Faithful to Harbor src/lib/cinemeta.ts meta().

#pragma once

#include <QObject>
#include <QString>

#include "core/MetaDetail.h"

class QNetworkAccessManager;

namespace tankoban {

class MetaService : public QObject {
    Q_OBJECT
public:
    explicit MetaService(QObject* parent = nullptr);

    void fetchDetail(const QString& type, const QString& id);

signals:
    void detailReady(const QString& id, const MetaDetail& detail);
    void detailFailed(const QString& id, const QString& error);

private:
    QNetworkAccessManager* m_nam = nullptr;
};

} // namespace tankoban

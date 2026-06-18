// Tankoban 3 — JikanClient (Anime provider).
//
// The first non-Stremio data source: the free, no-key MyAnimeList API via Jikan
// (https://api.jikan.moe/v4). Anime is not in Cinemeta and Jikan is not a Stremio addon,
// so the Anime route's built-in rows come through here (faithful to harbor-ref
// src/lib/providers/jikan.ts). Jikan rate-limits aggressively, so requests are queued and
// spaced (~400ms) with 429 backoff. Each row fetch maps Jikan `data[]` -> MetaItem with
// `mal:<id>` ids (ARM mal->kitsu mapping deferred to a later phase).

#pragma once

#include <QObject>
#include <QQueue>
#include <QString>
#include <QVector>

#include "core/MetaItem.h"

class QNetworkAccessManager;

namespace tankoban {

class JikanClient : public QObject {
    Q_OBJECT
public:
    explicit JikanClient(QObject* parent = nullptr);

    // path = the Jikan path after the base, e.g. "/seasons/now?page=1".
    void fetchRow(const QString& rowKey, const QString& path);

    // Hidden Gems (Harbor jikanUnderratedGems): gathers two raw pages, filters by member
    // ceiling / scored-by floor / sequel exclusion, sorts by score, then maps. Emits rowReady.
    void fetchGems(const QString& rowKey);

signals:
    void rowReady(const QString& rowKey, const QVector<tankoban::MetaItem>& items);
    void rowFailed(const QString& rowKey, const QString& error);

private:
    struct Pending {
        QString rowKey;
        QString path;
        int attempt = 0;
        int gemsPage = 0; // 0 = normal row; 1/2 = the two raw pages of a Hidden Gems fetch
    };

    void pump();
    void doRequest(const Pending& job);

    QQueue<Pending> m_queue;
    bool m_busy = false;
    QNetworkAccessManager* m_nam = nullptr;
};

} // namespace tankoban

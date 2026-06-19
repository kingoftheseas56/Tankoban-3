// Tankoban 3 - StreamService (Slice 5).
// Queries enabled Stremio addons for playable streams.
// Harbor-inspired adapter for src/lib/streams/stream-ids.ts + addons.ts.
// Current MetaDetail/EpisodeItem lacks Harbor's defaultVideoId, mapped IMDb,
// Kitsu stream id, and per-episode videoId fields, so buildStreamIds preserves
// the safe minimum candidate IDs available in the Tankoban 3 model.
#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "core/AddonModels.h"
#include "core/MetaDetail.h"
#include "core/StreamModels.h"

class QNetworkAccessManager;

namespace tankoban {

class AddonRegistry;

class StreamService : public QObject {
    Q_OBJECT
public:
    explicit StreamService(AddonRegistry* registry,
                           QNetworkAccessManager* nam,
                           QObject* parent = nullptr);

    // -- pure function adapted from Harbor's buildStreamIds --
    // metaId: MetaDetail::id (e.g. "tt1375666", "kitsu:123")
    // episode: the selected episode, or nullptr for movies
    // imdbId: optional fallback IMDB id (may be same as metaId or different)
    // defaultVideoId: MetaDetail default video id (from Stremio meta)
    // Returns deduplicated candidate ID list in priority order.
    static QStringList buildStreamIds(const QString& metaId,
                                      const EpisodeItem* episode,
                                      const QString& imdbId,
                                      const QString& defaultVideoId);

    // -- async adapter for Harbor's fetchAddonStreams --
    // Queries all enabled addons that support "stream" for this type/id.
    // Emits streamsPartial after each addon responds.
    // Emits streamsReady when all addons have responded or timed out.
    // Emits streamsFailed if no addons support streams.
    void fetchStreams(const MetaDetail& meta,
                      const EpisodeItem* episode);

signals:
    // Emitted after each addon responds (partial results accumulate).
    // The QVector is ALL streams received so far, not just the latest addon's.
    void streamsPartial(QVector<Stream> streams);

    // Emitted once - all addons have responded or timed out.
    // Streams are deduplicated.
    void streamsReady(QVector<Stream> streams);

    // Emitted if no enabled addon supports "stream" for this type/id.
    void streamsFailed(const QString& error);

private:
    void fetchFromAddon(const InstalledAddon& addon,
                        const QString& type,
                        const QString& id,
                        int generation);
    void finishOneRequest(int generation);

    AddonRegistry* m_registry = nullptr;
    QNetworkAccessManager* m_nam = nullptr;

    // tracked per fetchStreams call - generation guards against stale replies
    int m_generation = 0;
    int m_pendingRequests = 0;
    QVector<Stream> m_accumulated;

    // Partial-emission throttle (Harbor pipeline.ts emitPartial: drop partials that
    // land within ~250ms of the last). streamsReady always delivers the full final
    // set, so dropping intermediate partials never loses streams. m_partialClock is
    // an app-lifetime monotonic source; m_lastPartialMs resets per fetchStreams.
    QElapsedTimer m_partialClock;
    qint64 m_lastPartialMs = 0;
};

} // namespace tankoban

#include "core/StreamService.h"

#include <algorithm>

#include <QDebug>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QTimer>
#include <QUrl>

#include "core/AddonRegistry.h"

namespace tankoban {

namespace {

QStringList toStringList(const QJsonArray& array)
{
    QStringList values;
    values.reserve(array.size());
    for (const QJsonValue& value : array)
        values.push_back(value.toString());
    return values;
}

QHash<QString, QString> toStringHash(const QJsonObject& object)
{
    QHash<QString, QString> values;
    for (auto it = object.begin(); it != object.end(); ++it)
        values.insert(it.key(), it.value().toString());
    return values;
}

bool hasPrefix(const QString& value, const QString& prefix)
{
    return value.startsWith(prefix, Qt::CaseInsensitive);
}

bool isAnimeMetaId(const QString& metaId)
{
    return hasPrefix(metaId, QStringLiteral("kitsu:"))
        || hasPrefix(metaId, QStringLiteral("mal:"))
        || hasPrefix(metaId, QStringLiteral("anilist:"))
        || hasPrefix(metaId, QStringLiteral("anidb:"));
}

QString episodicId(const QString& baseId, const EpisodeItem* episode)
{
    if (!episode)
        return baseId;
    return QStringLiteral("%1:%2:%3")
        .arg(baseId)
        .arg(episode->season)
        .arg(episode->episode);
}

void appendUnique(QStringList& ids, QSet<QString>& seen, const QString& id)
{
    if (id.isEmpty() || seen.contains(id))
        return;
    seen.insert(id);
    ids.push_back(id);
}

bool hasStreamResource(const InstalledAddon& addon)
{
    for (const AddonResource& resource : addon.manifest.resources) {
        if (resource.name == QStringLiteral("stream"))
            return true;
    }
    return false;
}

int prefixPriority(const QString& id)
{
    if (hasPrefix(id, QStringLiteral("kitsu:")))
        return 0;
    if (hasPrefix(id, QStringLiteral("mal:")))
        return 1;
    if (hasPrefix(id, QStringLiteral("anidb:")))
        return 2;
    if (hasPrefix(id, QStringLiteral("anilist:")))
        return 3;
    if (hasPrefix(id, QStringLiteral("tt")))
        return 4;
    if (hasPrefix(id, QStringLiteral("tmdb:")))
        return 5;
    return 999;
}

bool idMatchesPrefixes(const QString& id, const QStringList& prefixes)
{
    if (prefixes.isEmpty())
        return true;
    for (const QString& prefix : prefixes) {
        if (id.startsWith(prefix))
            return true;
    }
    return false;
}

bool addonAcceptsId(const InstalledAddon& addon, const QString& type, const QString& id)
{
    for (const AddonResource& resource : addon.manifest.resources) {
        if (resource.name != QStringLiteral("stream") || resource.wildcard)
            continue;
        if (resource.types.contains(type) && idMatchesPrefixes(id, resource.idPrefixes))
            return true;
    }

    for (const AddonResource& resource : addon.manifest.resources) {
        if (resource.name != QStringLiteral("stream") || !resource.wildcard)
            continue;
        if (addon.manifest.types.contains(type)
            && idMatchesPrefixes(id, addon.manifest.idPrefixes))
            return true;
    }

    return false;
}

QString pickId(const InstalledAddon& addon, const QString& type, const QStringList& ids)
{
    struct Candidate {
        QString id;
        int index = 0;
    };

    QVector<Candidate> sorted;
    sorted.reserve(ids.size());
    for (int i = 0; i < ids.size(); ++i)
        sorted.push_back(Candidate{ids[i], i});

    std::stable_sort(sorted.begin(), sorted.end(), [](const Candidate& lhs, const Candidate& rhs) {
        const int leftPriority = prefixPriority(lhs.id);
        const int rightPriority = prefixPriority(rhs.id);
        if (leftPriority != rightPriority)
            return leftPriority < rightPriority;
        return lhs.index < rhs.index;
    });

    for (const Candidate& candidate : sorted) {
        if (addonAcceptsId(addon, type, candidate.id))
            return candidate.id;
    }

    return QString();
}

Stream mapStream(const QJsonObject& object, const InstalledAddon& addon)
{
    Stream stream;
    stream.name = object.value(QStringLiteral("name")).toString();
    stream.title = object.value(QStringLiteral("title")).toString();
    stream.description = object.value(QStringLiteral("description")).toString();
    stream.infoHash = object.value(QStringLiteral("infoHash")).toString().toLower();
    stream.fileIdx = object.value(QStringLiteral("fileIdx")).toInt(-1);
    stream.fileMustInclude = object.value(QStringLiteral("fileMustInclude")).toString();
    stream.url = object.value(QStringLiteral("url")).toString();
    stream.ytId = object.value(QStringLiteral("ytId")).toString();
    stream.externalUrl = object.value(QStringLiteral("externalUrl")).toString();
    stream.nzbUrl = object.value(QStringLiteral("nzbUrl")).toString();
    stream.sources = toStringList(object.value(QStringLiteral("sources")).toArray());
    stream.servers = toStringList(object.value(QStringLiteral("servers")).toArray());
    stream.rarUrls = toStringList(object.value(QStringLiteral("rarUrls")).toArray());
    stream.zipUrls = toStringList(object.value(QStringLiteral("zipUrls")).toArray());
    stream.tarUrls = toStringList(object.value(QStringLiteral("tarUrls")).toArray());
    stream.tgzUrls = toStringList(object.value(QStringLiteral("tgzUrls")).toArray());
    stream.sevenZipUrls = toStringList(object.value(QStringLiteral("sevenZipUrls")).toArray());

    const QJsonArray subtitles = object.value(QStringLiteral("subtitles")).toArray();
    stream.subtitles.reserve(subtitles.size());
    for (const QJsonValue& value : subtitles) {
        const QJsonObject subtitleObject = value.toObject();
        StreamSubtitle subtitle;
        subtitle.id = subtitleObject.value(QStringLiteral("id")).toString();
        subtitle.url = subtitleObject.value(QStringLiteral("url")).toString();
        subtitle.lang = subtitleObject.value(QStringLiteral("lang")).toString();
        subtitle.m = subtitleObject.value(QStringLiteral("m")).toString();
        stream.subtitles.push_back(subtitle);
    }

    const QJsonObject behaviorHints = object.value(QStringLiteral("behaviorHints")).toObject();
    stream.bingeGroup = behaviorHints.value(QStringLiteral("bingeGroup")).toString();
    stream.videoHash = behaviorHints.value(QStringLiteral("videoHash")).toString();
    stream.videoSize = behaviorHints.value(QStringLiteral("videoSize")).toVariant().toLongLong();
    stream.filename = behaviorHints.value(QStringLiteral("filename")).toString();
    stream.fileName = behaviorHints.value(QStringLiteral("fileName")).toString();
    stream.countryWhitelist = toStringList(
        behaviorHints.value(QStringLiteral("countryWhitelist")).toArray());
    stream.notWebReady = behaviorHints.value(QStringLiteral("notWebReady")).toBool();

    const QJsonObject proxyHeaders = behaviorHints.value(QStringLiteral("proxyHeaders")).toObject();
    stream.proxyHeaders.request = toStringHash(proxyHeaders.value(QStringLiteral("request")).toObject());
    stream.proxyHeaders.response
        = toStringHash(proxyHeaders.value(QStringLiteral("response")).toObject());
    stream.headers = toStringHash(behaviorHints.value(QStringLiteral("headers")).toObject());

    const QJsonArray contributors = object.value(QStringLiteral("contributors")).toArray();
    stream.contributors.reserve(contributors.size());
    for (const QJsonValue& value : contributors) {
        const QJsonObject contributorObject = value.toObject();
        Contributor contributor;
        contributor.id = contributorObject.value(QStringLiteral("id")).toString();
        contributor.name = contributorObject.value(QStringLiteral("name")).toString();
        stream.contributors.push_back(contributor);
    }

    stream.availability = object.value(QStringLiteral("availability")).toInt();
    stream.liveStreamCheck = object.value(QStringLiteral("liveStreamCheck")).toBool();

    stream.addonId = addon.manifest.id;
    stream.addonName = addon.manifest.name;
    stream.addonUrl = QUrl(addon.manifestUrl);
    // v1: addon-ranked detection is deferred until we have Harbor's helper surface.
    stream.addonRanked = false;
    stream.addonPriority = addon.order;

    return stream;
}

QVector<Stream> dedupeStreams(const QVector<Stream>& streams)
{
    QVector<Stream> deduped;
    deduped.reserve(streams.size());
    QHash<QString, int> byKey;

    for (const Stream& stream : streams) {
        const QString identity = !stream.infoHash.isEmpty()
            ? QStringLiteral("hash:%1:%2").arg(stream.infoHash).arg(stream.fileIdx)
            : QStringLiteral("url:%1")
                  .arg(stream.url.isEmpty()
                           ? (stream.title.isEmpty() ? stream.name : stream.title)
                           : stream.url);
        const QString key = stream.addonId + QLatin1Char(':') + identity;
        const auto it = byKey.constFind(key);
        if (it == byKey.constEnd()) {
            byKey.insert(key, deduped.size());
            deduped.push_back(stream);
            continue;
        }

        Stream& existing = deduped[*it];
        QSet<QString> merged(existing.sources.begin(), existing.sources.end());
        for (const QString& source : stream.sources)
            merged.insert(source);
        existing.sources = merged.values();
    }

    return deduped;
}

} // namespace

StreamService::StreamService(AddonRegistry* registry,
                             QNetworkAccessManager* nam,
                             QObject* parent)
    : QObject(parent)
    , m_registry(registry)
    , m_nam(nam)
{
}

QStringList StreamService::buildStreamIds(const QString& metaId,
                                          const EpisodeItem* episode,
                                          const QString& imdbId,
                                          const QString& defaultVideoId)
{
    QStringList ids;
    QSet<QString> seen;

    if (episode && !episode->id.isEmpty())
        appendUnique(ids, seen, episode->id);

    if (!episode && !defaultVideoId.isEmpty())
        appendUnique(ids, seen, defaultVideoId);

    const bool animeMeta = isAnimeMetaId(metaId);
    if (animeMeta) {
        if (!episode) {
            appendUnique(ids, seen, metaId);
        } else if (episode->id.isEmpty()) {
            // v1: anime episode fallback uses the fields we actually have today.
            appendUnique(ids, seen, episodicId(metaId, episode));
        }
    }

    if (hasPrefix(metaId, QStringLiteral("tt"))) {
        appendUnique(ids, seen, episodicId(metaId, episode));
    } else if (hasPrefix(metaId, QStringLiteral("tmdb:"))) {
        appendUnique(ids, seen, episodicId(metaId, episode));
    } else {
        appendUnique(ids, seen, episodicId(metaId, episode));
    }

    if (!animeMeta && hasPrefix(imdbId, QStringLiteral("tt"))
        && imdbId.compare(metaId, Qt::CaseInsensitive) != 0) {
        appendUnique(ids, seen, episodicId(imdbId, episode));
    }

    return ids;
}

void StreamService::fetchStreams(const MetaDetail& meta, const EpisodeItem* episode)
{
    ++m_generation;
    const int generation = m_generation;
    m_pendingRequests = 0;
    m_accumulated.clear();

    const QStringList ids = buildStreamIds(meta.id, episode, meta.id, QString());
    const QString type = meta.type;

    for (const InstalledAddon& addon : m_registry->enabledAddons()) {
        // v1: only skip addons with zero stream resources; status-only detection is deferred.
        if (!hasStreamResource(addon))
            continue;

        const QString matchedId = pickId(addon, type, ids);
        if (matchedId.isEmpty())
            continue;

        ++m_pendingRequests;
        fetchFromAddon(addon, type, matchedId, generation);
    }

    if (m_pendingRequests == 0) {
        emit streamsFailed(QStringLiteral("No stream addons configured"));
        return;
    }
}

void StreamService::fetchFromAddon(const InstalledAddon& addon,
                                   const QString& type,
                                   const QString& id,
                                   int generation)
{
    QString baseUrl = addon.baseUrl;
    if (!baseUrl.endsWith(QLatin1Char('/')))
        baseUrl += QLatin1Char('/');
    const QUrl url(baseUrl + QStringLiteral("stream/") + type + QLatin1Char('/') + id
                   + QStringLiteral(".json"));

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("User-Agent",
                         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

    QNetworkReply* reply = m_nam->get(request);
    QTimer* timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->setInterval(8000); // v1: flat 8s timeout for all addons.
    connect(timer, &QTimer::timeout, reply, [reply]() { reply->abort(); });
    timer->start();

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, addon, generation]() {
        timer->stop();
        timer->deleteLater();

        const auto cleanupReply = [reply]() { reply->deleteLater(); };

        if (generation != m_generation) {
            cleanupReply();
            return;
        }

        const int statusCode
            = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool statusOk = statusCode >= 200 && statusCode < 300;
        if (reply->error() != QNetworkReply::NoError || !statusOk) {
            qWarning() << "[StreamService]" << addon.manifest.name << "returned"
                       << reply->errorString();
            cleanupReply();
            finishOneRequest(generation);
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        if (!document.isObject()) {
            qWarning() << "[StreamService]" << addon.manifest.name
                       << "returned invalid stream JSON";
            cleanupReply();
            finishOneRequest(generation);
            return;
        }

        const QJsonArray streams = document.object().value(QStringLiteral("streams")).toArray();
        for (const QJsonValue& value : streams)
            m_accumulated.push_back(mapStream(value.toObject(), addon));

        emit streamsPartial(m_accumulated);

        cleanupReply();
        finishOneRequest(generation);
    });
}

void StreamService::finishOneRequest(int generation)
{
    if (generation != m_generation)
        return;

    if (m_pendingRequests > 0)
        --m_pendingRequests;

    if (m_pendingRequests == 0)
        emit streamsReady(dedupeStreams(m_accumulated));
}

} // namespace tankoban

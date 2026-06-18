// Tankoban 3 — JikanClient. See JikanClient.h.

#include "core/JikanClient.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <algorithm>

namespace tankoban {

namespace {
const QString kBase = QStringLiteral("https://api.jikan.moe/v4");
constexpr int kMinIntervalMs = 400; // Harbor MIN_INTERVAL_MS — Jikan throttle
constexpr int kMaxAttempts = 4;     // Harbor jikanQuery retry loop (attempt < 4)

QString pickImage(const QJsonObject& images)
{
    const QJsonObject webp = images.value(QStringLiteral("webp")).toObject();
    const QJsonObject jpg = images.value(QStringLiteral("jpg")).toObject();
    for (const QJsonObject& set : {webp, jpg}) {
        const QString large = set.value(QStringLiteral("large_image_url")).toString();
        if (!large.isEmpty())
            return large;
        const QString normal = set.value(QStringLiteral("image_url")).toString();
        if (!normal.isEmpty())
            return normal;
    }
    return {};
}

MetaItem toMeta(const QJsonObject& o)
{
    MetaItem m;
    const qint64 malId = qint64(o.value(QStringLiteral("mal_id")).toDouble());
    m.id = QStringLiteral("mal:") + QString::number(malId);

    const QString english = o.value(QStringLiteral("title_english")).toString();
    const QString title = o.value(QStringLiteral("title")).toString();
    const QString japanese = o.value(QStringLiteral("title_japanese")).toString();
    m.name = !english.isEmpty() ? english : (!title.isEmpty() ? title : japanese);

    const QString type = o.value(QStringLiteral("type")).toString();
    m.type = (type == QLatin1String("Movie")) ? QStringLiteral("movie") : QStringLiteral("series");

    m.poster = pickImage(o.value(QStringLiteral("images")).toObject());
    m.background = m.poster; // Jikan has no wide art; poster doubles as backdrop (v1)

    m.description = o.value(QStringLiteral("synopsis")).toString();

    const QJsonValue yearVal = o.value(QStringLiteral("year"));
    if (yearVal.isDouble() && yearVal.toInt() > 0) {
        m.releaseInfo = QString::number(yearVal.toInt());
    } else {
        const QString from = o.value(QStringLiteral("aired")).toObject()
                                 .value(QStringLiteral("from")).toString();
        if (from.size() >= 4)
            m.releaseInfo = from.left(4);
    }

    const double score = o.value(QStringLiteral("score")).toDouble();
    if (score > 0.0)
        m.imdbRating = QString::number(score, 'f', 1); // MAL score (UI may omit it)

    const QJsonArray genres = o.value(QStringLiteral("genres")).toArray();
    for (const QJsonValue& g : genres) {
        const QString name = g.toObject().value(QStringLiteral("name")).toString();
        if (!name.isEmpty())
            m.genres << name;
    }
    return m;
}

// ---- Harbor jikan.ts cache (lines 229-283), 1:1 ------------------------------------
// Two-tier so the Anime page loads instantly: an in-memory map (module singleton)
// hydrated once at first use from a JSON file on disk, with a 6h TTL. A fresh cache
// hit returns metas with ZERO network — that is Harbor's "instant" load. Harbor uses
// localStorage; the faithful Qt equivalent is a JSON file under the app cache dir
// (matches TB3's existing CacheLocation convention for image/disk caches). The persist
// is debounced 1s and keeps only the newest CATALOG_MAX shelves, trimming long synopses.
// The 400ms throttle + 429 backoff below are kept exactly — they only bite on a MISS.
constexpr qint64 kCacheTtlMs = 6LL * 60 * 60 * 1000; // Harbor CACHE_TTL (6h)
constexpr int kCatalogMax = 40;                       // Harbor CATALOG_MAX
constexpr int kPersistDescMax = 500;                  // Harbor PERSIST_DESC_MAX
constexpr int kPersistDebounceMs = 1000;              // Harbor catalogFlushTimer

struct CacheEntry {
    QVector<MetaItem> metas;
    qint64 t = 0;
};

QHash<QString, CacheEntry>& catalogCache()
{
    static QHash<QString, CacheEntry> cache; // module singleton (Harbor `cache` Map)
    return cache;
}

qint64 nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

QString catalogFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
           + QStringLiteral("/jikan_catalog.json"); // Harbor localStorage "harbor.jikancatalog"
}

QJsonObject metaToJson(const MetaItem& m)
{
    QJsonObject o;
    o.insert(QStringLiteral("id"), m.id);
    o.insert(QStringLiteral("type"), m.type);
    o.insert(QStringLiteral("name"), m.name);
    o.insert(QStringLiteral("poster"), m.poster);
    o.insert(QStringLiteral("background"), m.background);
    o.insert(QStringLiteral("description"), m.description);
    o.insert(QStringLiteral("releaseInfo"), m.releaseInfo);
    o.insert(QStringLiteral("imdbRating"), m.imdbRating);
    o.insert(QStringLiteral("runtime"), m.runtime);
    QJsonArray genres;
    for (const QString& g : m.genres)
        genres.append(g);
    o.insert(QStringLiteral("genres"), genres);
    return o;
}

MetaItem metaFromJson(const QJsonObject& o)
{
    MetaItem m;
    m.id = o.value(QStringLiteral("id")).toString();
    m.type = o.value(QStringLiteral("type")).toString();
    m.name = o.value(QStringLiteral("name")).toString();
    m.poster = o.value(QStringLiteral("poster")).toString();
    m.background = o.value(QStringLiteral("background")).toString();
    m.description = o.value(QStringLiteral("description")).toString();
    m.releaseInfo = o.value(QStringLiteral("releaseInfo")).toString();
    m.imdbRating = o.value(QStringLiteral("imdbRating")).toString();
    m.runtime = o.value(QStringLiteral("runtime")).toString();
    const QJsonArray genres = o.value(QStringLiteral("genres")).toArray();
    for (const QJsonValue& g : genres)
        m.genres << g.toString();
    return m;
}

// Hydrate the in-memory cache from disk once (Harbor's module-load IIFE), dropping any
// shelf already past the 6h TTL.
void loadCatalogOnce()
{
    static bool loaded = false;
    if (loaded)
        return;
    loaded = true;

    QFile f(catalogFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject())
        return;

    const qint64 now = nowMs();
    const QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        const QJsonObject e = it.value().toObject();
        const qint64 t = qint64(e.value(QStringLiteral("t")).toDouble());
        if (now - t >= kCacheTtlMs)
            continue;
        const QJsonArray arr = e.value(QStringLiteral("metas")).toArray();
        if (arr.isEmpty())
            continue;
        CacheEntry entry;
        entry.t = t;
        entry.metas.reserve(arr.size());
        for (const QJsonValue& v : arr)
            entry.metas.push_back(metaFromJson(v.toObject()));
        catalogCache().insert(it.key(), entry);
    }
}

// Write the newest kCatalogMax shelves to disk, trimming long synopses (Harbor
// persistCatalog). Called debounced via schedulePersist().
void writeCatalogNow()
{
    QHash<QString, CacheEntry>& cache = catalogCache();
    QList<QString> keys = cache.keys();
    std::sort(keys.begin(), keys.end(), [&cache](const QString& a, const QString& b) {
        return cache.value(a).t > cache.value(b).t; // newest first
    });
    if (keys.size() > kCatalogMax)
        keys = keys.mid(0, kCatalogMax);

    QJsonObject root;
    for (const QString& key : keys) {
        const CacheEntry& entry = cache.value(key);
        QJsonArray arr;
        for (const MetaItem& m : entry.metas) {
            MetaItem trimmed = m;
            if (trimmed.description.size() > kPersistDescMax)
                trimmed.description =
                    trimmed.description.left(kPersistDescMax) + QStringLiteral("...");
            arr.append(metaToJson(trimmed));
        }
        QJsonObject e;
        e.insert(QStringLiteral("t"), double(entry.t));
        e.insert(QStringLiteral("metas"), arr);
        root.insert(key, e);
    }

    const QString path = catalogFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        f.close();
    }
}

void schedulePersist()
{
    static QTimer* timer = nullptr;
    if (!timer) {
        timer = new QTimer; // app-lifetime singleton, GUI thread
        timer->setSingleShot(true);
        timer->setInterval(kPersistDebounceMs);
        QObject::connect(timer, &QTimer::timeout, timer, []() { writeCatalogNow(); });
    }
    timer->start(); // restart resets the countdown -> debounce
}
} // namespace

JikanClient::JikanClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void JikanClient::fetchRow(const QString& rowKey, const QString& path)
{
    loadCatalogOnce();

    // Cache hit (fresh) -> emit with ZERO network, off the throttle queue (Harbor
    // jikanQuery early return, line 315-316). Emit on the next event-loop tick so a
    // caller that connects rowReady immediately before calling this still receives it
    // (mirrors awaiting a resolved promise).
    const auto it = catalogCache().constFind(path);
    if (it != catalogCache().constEnd() && nowMs() - it->t < kCacheTtlMs) {
        const QVector<MetaItem> metas = it->metas;
        QPointer<JikanClient> self(this);
        QTimer::singleShot(0, this, [self, rowKey, metas]() {
            if (self)
                emit self->rowReady(rowKey, metas);
        });
        return;
    }

    m_queue.enqueue({rowKey, path, 0});
    pump();
}

void JikanClient::pump()
{
    if (m_busy || m_queue.isEmpty())
        return;
    m_busy = true;
    doRequest(m_queue.dequeue());
}

void JikanClient::doRequest(const Pending& job)
{
    QNetworkRequest req((QUrl(kBase + job.path)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));

    QNetworkReply* reply = m_nam->get(req);
    QPointer<JikanClient> self(this);
    connect(reply, &QNetworkReply::finished, reply, [self, reply, job]() {
        reply->deleteLater();
        if (!self)
            return;

        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        auto scheduleNext = [self]() {
            self->m_busy = false;
            QTimer::singleShot(kMinIntervalMs, self, [self]() {
                if (self)
                    self->pump();
            });
        };

        // 429 -> backoff + retry (re-queue at the front). Harbor jikanQuery (line 328)
        // backs off ONLY on 429, with an exponential 2000 * 2^attempt over 4 attempts;
        // every other non-ok status returns empty immediately (`if (!r.ok) return []`).
        // We match that: a 429'd row recovers (then caches, instant thereafter), but a
        // 5xx/4xx (e.g. Jikan's 504 on heavy query endpoints) fails fast below instead of
        // stalling the shared queue ~14s — affordable because the cold path runs once / 6h.
        if (status == 429) {
            if (job.attempt + 1 < kMaxAttempts) {
                Pending retry = job;
                retry.attempt += 1;
                self->m_queue.prepend(retry);
                self->m_busy = false;
                QTimer::singleShot(2000 * (1 << job.attempt), self, [self]() { // Harbor backoff
                    if (self)
                        self->pump();
                });
                return;
            }
            emit self->rowFailed(job.rowKey, QStringLiteral("Jikan throttled"));
            scheduleNext();
            return;
        }

        // Harbor `if (!r.ok) return []` — never cache a non-2xx, so a transient 504 isn't
        // frozen as an empty shelf for 6h; it simply retries on the next cold pass.
        if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
            emit self->rowFailed(job.rowKey, reply->errorString());
            scheduleNext();
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray data = doc.object().value(QStringLiteral("data")).toArray();
        QVector<MetaItem> items;
        items.reserve(data.size());
        for (const QJsonValue& v : data) {
            MetaItem m = toMeta(v.toObject());
            if (!m.poster.isEmpty() && !m.id.isEmpty())
                items.push_back(m);
        }
        catalogCache().insert(job.path, CacheEntry{items, nowMs()}); // remember (Harbor cache.set)
        schedulePersist();                                           // persist to disk, debounced
        emit self->rowReady(job.rowKey, items);
        scheduleNext();
    });
}

} // namespace tankoban

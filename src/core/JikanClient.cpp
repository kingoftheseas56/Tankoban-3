// Tankoban 3 — JikanClient. See JikanClient.h.

#include "core/JikanClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>
#include <QUrl>

namespace tankoban {

namespace {
const QString kBase = QStringLiteral("https://api.jikan.moe/v4");
constexpr int kMinIntervalMs = 400; // Harbor MIN_INTERVAL_MS — Jikan throttle
constexpr int kMaxAttempts = 3;

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
} // namespace

JikanClient::JikanClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void JikanClient::fetchRow(const QString& rowKey, const QString& path)
{
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

        // 429 / 5xx -> backoff + retry (re-queue at the front).
        if (status == 429 || (status >= 500 && status < 600)) {
            if (job.attempt + 1 < kMaxAttempts) {
                Pending retry = job;
                retry.attempt += 1;
                self->m_queue.prepend(retry);
                self->m_busy = false;
                QTimer::singleShot(1200 * (job.attempt + 1), self, [self]() {
                    if (self)
                        self->pump();
                });
                return;
            }
            emit self->rowFailed(job.rowKey, QStringLiteral("Jikan throttled"));
            scheduleNext();
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
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
        emit self->rowReady(job.rowKey, items);
        scheduleNext();
    });
}

} // namespace tankoban

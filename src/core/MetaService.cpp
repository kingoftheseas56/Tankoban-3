// Tankoban 3 — MetaService (Detail path). See MetaService.h.

#include "core/MetaService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QUrl>

namespace tankoban {

namespace {
const QString kCinemeta = QStringLiteral("https://v3-cinemeta.strem.io");

QString narrowType(const QString& t)
{
    return t == QLatin1String("series") ? QStringLiteral("series") : QStringLiteral("movie");
}
} // namespace

MetaService::MetaService(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void MetaService::fetchDetail(const QString& type, const QString& id)
{
    const QString t = narrowType(type);
    const QUrl url(kCinemeta + QStringLiteral("/meta/") + t + QStringLiteral("/") + id
                   + QStringLiteral(".json"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));

    QNetworkReply* reply = m_nam->get(req);
    QPointer<MetaService> self(this);
    connect(reply, &QNetworkReply::finished, this, [self, reply, id]() {
        reply->deleteLater();
        if (!self)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            emit self->detailFailed(id, reply->errorString());
            return;
        }
        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonObject o = root.value(QStringLiteral("meta")).toObject();

        MetaDetail d;
        d.id = o.value(QStringLiteral("id")).toString();
        d.type = o.value(QStringLiteral("type")).toString();
        d.name = o.value(QStringLiteral("name")).toString();
        d.poster = o.value(QStringLiteral("poster")).toString();
        d.background = o.value(QStringLiteral("background")).toString();
        d.logo = o.value(QStringLiteral("logo")).toString();
        d.description = o.value(QStringLiteral("description")).toString();
        d.releaseInfo = o.value(QStringLiteral("releaseInfo")).toString();
        d.imdbRating = o.value(QStringLiteral("imdbRating")).toString();
        d.runtime = o.value(QStringLiteral("runtime")).toString();
        for (const QJsonValue& g : o.value(QStringLiteral("genres")).toArray())
            d.genres << g.toString();

        for (const QJsonValue& vv : o.value(QStringLiteral("videos")).toArray()) {
            const QJsonObject v = vv.toObject();
            EpisodeItem e;
            e.id = v.value(QStringLiteral("id")).toString();
            e.season = v.value(QStringLiteral("season")).toInt();
            e.episode = v.contains(QStringLiteral("episode"))
                ? v.value(QStringLiteral("episode")).toInt()
                : v.value(QStringLiteral("number")).toInt();
            e.name = v.value(QStringLiteral("name")).toString();
            if (e.name.isEmpty())
                e.name = v.value(QStringLiteral("title")).toString();
            e.overview = v.value(QStringLiteral("overview")).toString();
            if (e.overview.isEmpty())
                e.overview = v.value(QStringLiteral("description")).toString();
            e.thumbnail = v.value(QStringLiteral("thumbnail")).toString();
            e.released = v.value(QStringLiteral("released")).toString();
            if (e.released.isEmpty())
                e.released = v.value(QStringLiteral("firstAired")).toString();
            d.videos.push_back(e);
        }
        d.loaded = true;
        emit self->detailReady(id, d);
    });
}

} // namespace tankoban

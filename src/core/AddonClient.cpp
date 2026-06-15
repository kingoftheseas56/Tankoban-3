// Tankoban 3 — AddonClient (Step 3). See AddonClient.h.

#include "core/AddonClient.h"

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
// metahub posters are served at /small|/medium|/large — force the largest so cards
// are sharp (the Tankoban 2 blur lesson, baked in from the start).
QString upgradePoster(QString url)
{
    // Right-size to /medium/ (~110KB, sharp at up to 2x on a ~150px card) instead of
    // /large/ (~238KB): the fix for "posters load painfully slow". /large/ was
    // overkill for the card size and roughly doubled the data per cover.
    url.replace(QStringLiteral("/poster/small/"), QStringLiteral("/poster/medium/"));
    url.replace(QStringLiteral("/poster/large/"), QStringLiteral("/poster/medium/"));
    return url;
}
} // namespace

AddonClient::AddonClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void AddonClient::fetchCatalog(const QString& rowKey, const QString& base,
                               const QString& type, const QString& id)
{
    const QUrl url(base + QStringLiteral("/catalog/") + type + QStringLiteral("/") + id
                   + QStringLiteral(".json"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));

    QNetworkReply* reply = m_nam->get(req);
    QPointer<AddonClient> self(this);
    connect(reply, &QNetworkReply::finished, this, [self, reply, rowKey]() {
        reply->deleteLater();
        if (!self)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            emit self->catalogFailed(rowKey, reply->errorString());
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray metas = doc.object().value(QStringLiteral("metas")).toArray();
        QVector<MetaItem> items;
        items.reserve(metas.size());
        for (const QJsonValue& v : metas) {
            const QJsonObject o = v.toObject();
            MetaItem m;
            m.id = o.value(QStringLiteral("id")).toString();
            m.type = o.value(QStringLiteral("type")).toString();
            m.name = o.value(QStringLiteral("name")).toString();
            m.poster = upgradePoster(o.value(QStringLiteral("poster")).toString());
            m.background = o.value(QStringLiteral("background")).toString();
            m.description = o.value(QStringLiteral("description")).toString();
            m.releaseInfo = o.value(QStringLiteral("releaseInfo")).toString();
            m.imdbRating = o.value(QStringLiteral("imdbRating")).toString();
            m.runtime = o.value(QStringLiteral("runtime")).toString();
            if (!m.poster.isEmpty())
                items.push_back(m);
        }
        emit self->catalogReady(rowKey, items);
    });
}

} // namespace tankoban

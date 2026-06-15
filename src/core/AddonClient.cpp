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
    url.replace(QStringLiteral("/poster/small/"), QStringLiteral("/poster/large/"));
    url.replace(QStringLiteral("/poster/medium/"), QStringLiteral("/poster/large/"));
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
            if (!m.poster.isEmpty())
                items.push_back(m);
        }
        emit self->catalogReady(rowKey, items);
    });
}

} // namespace tankoban

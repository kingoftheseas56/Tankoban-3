// Tankoban 3 — StremioAddonsProvider (Addons marketplace). See StremioAddonsProvider.h.

#include "core/StremioAddonsProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QUrl>
#include <QUrlQuery>

namespace tankoban {

namespace {
const QString kApi = QStringLiteral("https://stremio-addons.net/api/v0");

StoreAddon parseStoreAddon(const QJsonObject& a)
{
    StoreAddon s;
    s.uuid = a.value(QStringLiteral("uuid")).toString();
    s.slug = a.value(QStringLiteral("slug")).toString();
    s.manifestUrl = a.value(QStringLiteral("manifestUrl")).toString();
    if (s.manifestUrl.isEmpty())
        s.manifestUrl = a.value(QStringLiteral("url")).toString();
    s.stars = a.value(QStringLiteral("stars")).toInt();
    const QJsonObject m = a.value(QStringLiteral("manifest")).toObject();
    s.name = m.value(QStringLiteral("name")).toString();
    s.description = m.value(QStringLiteral("description")).toString();
    s.logo = m.value(QStringLiteral("logo")).toString();
    const QJsonObject bh = m.value(QStringLiteral("behaviorHints")).toObject();
    s.adult = bh.value(QStringLiteral("adult")).toBool();
    s.configurable = bh.value(QStringLiteral("configurable")).toBool()
        || bh.value(QStringLiteral("configurationRequired")).toBool();
    for (const QJsonValue& c : a.value(QStringLiteral("categories")).toArray())
        s.categories << c.toObject().value(QStringLiteral("name")).toString();
    return s;
}

bool matchesSearch(const StoreAddon& s, const QString& q)
{
    return s.name.toLower().contains(q) || s.description.toLower().contains(q)
        || s.slug.toLower().contains(q);
}
} // namespace

StremioAddonsProvider::StremioAddonsProvider(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void StremioAddonsProvider::list(const QString& requestKey, const QString& mode,
                                 const QString& category, const QString& search, int page,
                                 bool includeAdult)
{
    const bool rising = (mode == QLatin1String("rising"));
    QUrl url;
    if (rising) {
        url = QUrl(kApi + QStringLiteral("/rising"));
    } else {
        QUrl u(kApi + QStringLiteral("/addons"));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("page"), QString::number(qMax(1, page)));
        q.addQueryItem(QStringLiteral("limit"), QStringLiteral("30"));
        if (mode == QLatin1String("new")) {
            q.addQueryItem(QStringLiteral("sort_by"), QStringLiteral("createdAt"));
            q.addQueryItem(QStringLiteral("order"), QStringLiteral("desc"));
        } else {
            q.addQueryItem(QStringLiteral("sort_by"), QStringLiteral("stars"));
            q.addQueryItem(QStringLiteral("order"), QStringLiteral("desc"));
        }
        if (!includeAdult)
            q.addQueryItem(QStringLiteral("nsfw"), QStringLiteral("exclude"));
        if (!category.isEmpty())
            q.addQueryItem(QStringLiteral("category"), category);
        if (!search.isEmpty())
            q.addQueryItem(QStringLiteral("search"), search);
        u.setQuery(q);
        url = u;
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    QNetworkReply* reply = m_nam->get(req);
    QPointer<StremioAddonsProvider> self(this);
    const QString sLower = search.toLower();
    connect(reply, &QNetworkReply::finished, this,
            [self, reply, requestKey, rising, includeAdult, sLower]() {
                reply->deleteLater();
                if (!self)
                    return;
                if (reply->error() != QNetworkReply::NoError) {
                    emit self->listFailed(requestKey, reply->errorString());
                    return;
                }
                const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
                StorePage sp;
                for (const QJsonValue& v : obj.value(QStringLiteral("addons")).toArray()) {
                    StoreAddon s = parseStoreAddon(v.toObject());
                    if (s.manifestUrl.isEmpty() || s.name.isEmpty())
                        continue;
                    if (rising) { // /rising isn't filtered server-side
                        if (!includeAdult && s.adult)
                            continue;
                        if (!sLower.isEmpty() && !matchesSearch(s, sLower))
                            continue;
                    }
                    sp.addons.push_back(s);
                }
                if (rising) {
                    sp.page = 1;
                    sp.totalPages = 1;
                    sp.hasNext = false;
                } else {
                    const QJsonObject pg = obj.value(QStringLiteral("pagination")).toObject();
                    sp.page = pg.value(QStringLiteral("page")).toInt(1);
                    sp.totalPages = pg.value(QStringLiteral("totalPages")).toInt(1);
                    sp.hasNext = pg.value(QStringLiteral("hasNextPage")).toBool();
                }
                emit self->listReady(requestKey, sp);
            });
}

void StremioAddonsProvider::fetchCategories()
{
    QNetworkRequest req(QUrl(kApi + QStringLiteral("/categories")));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    QNetworkReply* reply = m_nam->get(req);
    QPointer<StremioAddonsProvider> self(this);
    connect(reply, &QNetworkReply::finished, this, [self, reply]() {
        reply->deleteLater();
        if (!self || reply->error() != QNetworkReply::NoError)
            return;
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        QVector<StoreCategory> cats;
        for (const QJsonValue& v : obj.value(QStringLiteral("categories")).toArray()) {
            const QJsonObject c = v.toObject();
            StoreCategory sc;
            sc.name = c.value(QStringLiteral("name")).toString();
            sc.slug = c.value(QStringLiteral("slug")).toString();
            if (!sc.slug.isEmpty())
                cats.push_back(sc);
        }
        emit self->categoriesReady(cats);
    });
}

} // namespace tankoban

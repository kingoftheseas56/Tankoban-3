// Tankoban 3 — AddonRegistry (Addons path). See AddonRegistry.h.

#include "core/AddonRegistry.h"

#include <algorithm>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

namespace tankoban {

namespace {

QSettings appSettings()
{
    return QSettings(QStringLiteral("Tankoban"), QStringLiteral("Tankoban3"));
}

// Accept https/http/stremio, a /configure page, or a bare base URL → a manifest URL.
QString normalizeUrl(QString raw)
{
    raw = raw.trimmed();
    if (raw.startsWith(QLatin1String("stremio://"), Qt::CaseInsensitive))
        raw = QStringLiteral("https://") + raw.mid(QStringLiteral("stremio://").size());
    raw.remove(QRegularExpression(QStringLiteral("[#/]*configure/?$")));
    if (raw.isEmpty())
        return raw;
    if (!raw.endsWith(QLatin1String("manifest.json"), Qt::CaseInsensitive)) {
        if (!raw.endsWith(QLatin1Char('/')))
            raw += QLatin1Char('/');
        raw += QStringLiteral("manifest.json");
    }
    return raw;
}

QStringList toStringList(const QJsonArray& a)
{
    QStringList out;
    for (const QJsonValue& v : a)
        out << v.toString();
    return out;
}

// Parse a freshly-fetched Stremio manifest object.
Manifest parseManifest(const QJsonObject& o)
{
    Manifest m;
    m.id = o.value(QStringLiteral("id")).toString();
    m.name = o.value(QStringLiteral("name")).toString();
    m.version = o.value(QStringLiteral("version")).toString();
    m.description = o.value(QStringLiteral("description")).toString();
    m.logo = o.value(QStringLiteral("logo")).toString();
    m.background = o.value(QStringLiteral("background")).toString();
    m.types = toStringList(o.value(QStringLiteral("types")).toArray());
    m.idPrefixes = toStringList(o.value(QStringLiteral("idPrefixes")).toArray());

    for (const QJsonValue& v : o.value(QStringLiteral("resources")).toArray()) {
        AddonResource r;
        if (v.isString()) {
            r.name = v.toString();
            r.wildcard = true;
        } else {
            const QJsonObject ro = v.toObject();
            r.name = ro.value(QStringLiteral("name")).toString();
            r.types = toStringList(ro.value(QStringLiteral("types")).toArray());
            r.idPrefixes = toStringList(ro.value(QStringLiteral("idPrefixes")).toArray());
        }
        if (!r.name.isEmpty())
            m.resources.push_back(r);
    }

    for (const QJsonValue& v : o.value(QStringLiteral("catalogs")).toArray()) {
        const QJsonObject co = v.toObject();
        CatalogDef c;
        c.id = co.value(QStringLiteral("id")).toString();
        c.type = co.value(QStringLiteral("type")).toString();
        c.name = co.value(QStringLiteral("name")).toString();
        for (const QJsonValue& e : co.value(QStringLiteral("extra")).toArray()) {
            const QJsonObject eo = e.toObject();
            if (eo.value(QStringLiteral("isRequired")).toBool()
                || eo.value(QStringLiteral("required")).toBool())
                c.hasRequiredExtra = true;
        }
        m.catalogs.push_back(c);
    }

    const QJsonObject bh = o.value(QStringLiteral("behaviorHints")).toObject();
    m.adult = bh.value(QStringLiteral("adult")).toBool();
    m.configurable = bh.value(QStringLiteral("configurable")).toBool()
        || bh.value(QStringLiteral("configurationRequired")).toBool();
    return m;
}

// Round-trip a Manifest through our own compact persistence layout.
QJsonObject manifestToJson(const Manifest& m)
{
    QJsonObject o;
    o[QStringLiteral("id")] = m.id;
    o[QStringLiteral("name")] = m.name;
    o[QStringLiteral("version")] = m.version;
    o[QStringLiteral("description")] = m.description;
    o[QStringLiteral("logo")] = m.logo;
    o[QStringLiteral("background")] = m.background;
    o[QStringLiteral("types")] = QJsonArray::fromStringList(m.types);
    o[QStringLiteral("idPrefixes")] = QJsonArray::fromStringList(m.idPrefixes);
    o[QStringLiteral("adult")] = m.adult;
    o[QStringLiteral("configurable")] = m.configurable;
    QJsonArray res;
    for (const AddonResource& r : m.resources) {
        QJsonObject ro;
        ro[QStringLiteral("name")] = r.name;
        ro[QStringLiteral("types")] = QJsonArray::fromStringList(r.types);
        ro[QStringLiteral("idPrefixes")] = QJsonArray::fromStringList(r.idPrefixes);
        ro[QStringLiteral("wildcard")] = r.wildcard;
        res.append(ro);
    }
    o[QStringLiteral("resources")] = res;
    QJsonArray cats;
    for (const CatalogDef& c : m.catalogs) {
        QJsonObject co;
        co[QStringLiteral("id")] = c.id;
        co[QStringLiteral("type")] = c.type;
        co[QStringLiteral("name")] = c.name;
        co[QStringLiteral("req")] = c.hasRequiredExtra;
        cats.append(co);
    }
    o[QStringLiteral("catalogs")] = cats;
    return o;
}

Manifest manifestFromJson(const QJsonObject& o)
{
    Manifest m;
    m.id = o.value(QStringLiteral("id")).toString();
    m.name = o.value(QStringLiteral("name")).toString();
    m.version = o.value(QStringLiteral("version")).toString();
    m.description = o.value(QStringLiteral("description")).toString();
    m.logo = o.value(QStringLiteral("logo")).toString();
    m.background = o.value(QStringLiteral("background")).toString();
    m.types = toStringList(o.value(QStringLiteral("types")).toArray());
    m.idPrefixes = toStringList(o.value(QStringLiteral("idPrefixes")).toArray());
    m.adult = o.value(QStringLiteral("adult")).toBool();
    m.configurable = o.value(QStringLiteral("configurable")).toBool();
    for (const QJsonValue& v : o.value(QStringLiteral("resources")).toArray()) {
        const QJsonObject ro = v.toObject();
        AddonResource r;
        r.name = ro.value(QStringLiteral("name")).toString();
        r.types = toStringList(ro.value(QStringLiteral("types")).toArray());
        r.idPrefixes = toStringList(ro.value(QStringLiteral("idPrefixes")).toArray());
        r.wildcard = ro.value(QStringLiteral("wildcard")).toBool();
        if (!r.name.isEmpty())
            m.resources.push_back(r);
    }
    for (const QJsonValue& v : o.value(QStringLiteral("catalogs")).toArray()) {
        const QJsonObject co = v.toObject();
        CatalogDef c;
        c.id = co.value(QStringLiteral("id")).toString();
        c.type = co.value(QStringLiteral("type")).toString();
        c.name = co.value(QStringLiteral("name")).toString();
        c.hasRequiredExtra = co.value(QStringLiteral("req")).toBool();
        m.catalogs.push_back(c);
    }
    return m;
}

// Harbor's addonAccepts(): does this addon serve `resource` for this type/id?
bool accepts(const InstalledAddon& a, const QString& resource, const QString& type,
             const QString& id)
{
    const Manifest& m = a.manifest;
    bool sawSpecific = false;
    for (const AddonResource& r : m.resources) {
        if (r.wildcard || r.name != resource)
            continue;
        sawSpecific = true;
        const bool typeOk = r.types.contains(type);
        const bool idOk = r.idPrefixes.isEmpty()
            || std::any_of(r.idPrefixes.cbegin(), r.idPrefixes.cend(),
                           [&](const QString& p) { return id.startsWith(p); });
        if (typeOk && idOk)
            return true;
    }
    if (sawSpecific)
        return false;
    bool hasWildcard = false;
    for (const AddonResource& r : m.resources)
        if (r.wildcard && r.name == resource)
            hasWildcard = true;
    if (!hasWildcard)
        return false;
    if (!m.types.contains(type))
        return false;
    if (!m.idPrefixes.isEmpty()
        && !std::any_of(m.idPrefixes.cbegin(), m.idPrefixes.cend(),
                        [&](const QString& p) { return id.startsWith(p); }))
        return false;
    return true;
}

QString baseFromManifestUrl(const QString& url)
{
    QString base = url;
    if (base.endsWith(QLatin1String("/manifest.json")))
        base.chop(QStringLiteral("/manifest.json").size());
    return base;
}

} // namespace

AddonRegistry::AddonRegistry(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

QVector<InstalledAddon> AddonRegistry::enabledAddons() const
{
    QVector<InstalledAddon> out;
    for (const InstalledAddon& a : m_addons)
        if (a.enabled)
            out.push_back(a);
    std::sort(out.begin(), out.end(),
              [](const InstalledAddon& x, const InstalledAddon& y) { return x.order < y.order; });
    return out;
}

QVector<InstalledAddon> AddonRegistry::forResource(const QString& resource, const QString& type,
                                                   const QString& id) const
{
    QVector<InstalledAddon> out;
    for (const InstalledAddon& a : enabledAddons())
        if (accepts(a, resource, type, id))
            out.push_back(a);
    return out;
}

void AddonRegistry::load()
{
    m_addons.clear();
    const QString raw = appSettings().value(QStringLiteral("addons")).toString();
    if (raw.isEmpty())
        return;
    const QJsonArray arr = QJsonDocument::fromJson(raw.toUtf8()).array();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        InstalledAddon a;
        a.manifestUrl = o.value(QStringLiteral("url")).toString();
        a.baseUrl = baseFromManifestUrl(a.manifestUrl);
        a.enabled = o.value(QStringLiteral("enabled")).toBool(true);
        a.order = o.value(QStringLiteral("order")).toInt();
        a.manifest = manifestFromJson(o.value(QStringLiteral("manifest")).toObject());
        if (!a.manifestUrl.isEmpty() && !a.manifest.id.isEmpty())
            m_addons.push_back(a);
    }
    std::sort(m_addons.begin(), m_addons.end(),
              [](const InstalledAddon& x, const InstalledAddon& y) { return x.order < y.order; });
}

void AddonRegistry::persist() const
{
    QJsonArray arr;
    for (const InstalledAddon& a : m_addons) {
        QJsonObject o;
        o[QStringLiteral("url")] = a.manifestUrl;
        o[QStringLiteral("enabled")] = a.enabled;
        o[QStringLiteral("order")] = a.order;
        o[QStringLiteral("manifest")] = manifestToJson(a.manifest);
        arr.append(o);
    }
    appSettings().setValue(QStringLiteral("addons"),
                           QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

void AddonRegistry::installFromUrl(const QString& rawUrl)
{
    const QString url = normalizeUrl(rawUrl);
    if (url.isEmpty()
        || !(url.startsWith(QLatin1String("http://")) || url.startsWith(QLatin1String("https://")))) {
        emit installFailed(rawUrl, QStringLiteral("That doesn't look like an addon URL."));
        return;
    }
    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    QNetworkReply* reply = m_nam->get(req);
    QPointer<AddonRegistry> self(this);
    connect(reply, &QNetworkReply::finished, this, [self, reply, rawUrl, url]() {
        reply->deleteLater();
        if (!self)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            emit self->installFailed(rawUrl, reply->errorString());
            return;
        }
        QJsonParseError pe{};
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
            emit self->installFailed(rawUrl, QStringLiteral("The manifest isn't valid JSON."));
            return;
        }
        const Manifest m = parseManifest(doc.object());
        if (m.id.isEmpty() || m.name.isEmpty()) {
            emit self->installFailed(rawUrl, QStringLiteral("Manifest is missing an id or name."));
            return;
        }
        InstalledAddon a;
        a.manifestUrl = url;
        a.baseUrl = baseFromManifestUrl(url);
        a.manifest = m;
        a.enabled = true;

        bool replaced = false;
        for (int i = 0; i < self->m_addons.size(); ++i) {
            if (self->m_addons[i].manifest.id == m.id || self->m_addons[i].manifestUrl == url) {
                a.order = self->m_addons[i].order;
                a.enabled = self->m_addons[i].enabled;
                self->m_addons[i] = a;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            a.order = int(self->m_addons.size());
            self->m_addons.push_back(a);
        }
        self->persist();
        emit self->installSucceeded(m.name, replaced);
        emit self->addonsChanged();
    });
}

void AddonRegistry::uninstall(const QString& manifestUrl)
{
    const int before = m_addons.size();
    m_addons.erase(std::remove_if(m_addons.begin(), m_addons.end(),
                                  [&](const InstalledAddon& a) {
                                      return a.manifestUrl == manifestUrl;
                                  }),
                   m_addons.end());
    if (m_addons.size() != before) {
        persist();
        emit addonsChanged();
    }
}

void AddonRegistry::setEnabled(const QString& manifestUrl, bool enabled)
{
    for (InstalledAddon& a : m_addons) {
        if (a.manifestUrl == manifestUrl && a.enabled != enabled) {
            a.enabled = enabled;
            persist();
            emit addonsChanged();
            return;
        }
    }
}

} // namespace tankoban

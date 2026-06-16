// Tankoban 3 — AddonCard (Addons marketplace). See AddonCard.h.

#include "ui/AddonCard.h"

#include <QEnterEvent>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QStandardPaths>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace tankoban {

namespace {
QNetworkAccessManager* logoNam()
{
    static QNetworkAccessManager* nam = [] {
        auto* m = new QNetworkAccessManager();
        auto* cache = new QNetworkDiskCache(m);
        cache->setCacheDirectory(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + QStringLiteral("/addon-logo-cache"));
        cache->setMaximumCacheSize(qint64(48) * 1024 * 1024);
        m->setCache(cache);
        return m;
    }();
    return nam;
}
} // namespace

AddonCard::AddonCard(const StoreAddon& addon, bool installed, QWidget* parent)
    : QWidget(parent)
    , m_addon(addon)
    , m_installed(installed)
{
    setObjectName(QStringLiteral("StoreCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(kCardW, kCardH);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(16, 14, 16, 14);
    col->setSpacing(8);

    auto* top = new QHBoxLayout();
    top->setSpacing(12);
    m_logo = new QLabel(this);
    m_logo->setObjectName(QStringLiteral("StoreCardLogo"));
    m_logo->setFixedSize(44, 44);
    m_logo->setAlignment(Qt::AlignCenter);
    m_logo->setText(m_addon.name.left(1).toUpper());
    top->addWidget(m_logo);

    auto* idCol = new QVBoxLayout();
    idCol->setContentsMargins(0, 0, 0, 0);
    idCol->setSpacing(2);
    m_name = new QLabel(m_addon.name, this);
    m_name->setObjectName(QStringLiteral("StoreCardName"));
    m_stars = new QLabel(QStringLiteral("★ %L1").arg(m_addon.stars), this);
    m_stars->setObjectName(QStringLiteral("StoreCardStars"));
    idCol->addWidget(m_name);
    idCol->addWidget(m_stars);
    top->addLayout(idCol, 1);
    col->addLayout(top);

    QString desc = m_addon.description;
    if (desc.size() > 100)
        desc = desc.left(99).trimmed() + QStringLiteral("…");
    m_desc = new QLabel(desc, this);
    m_desc->setObjectName(QStringLiteral("StoreCardDesc"));
    m_desc->setWordWrap(true);
    m_desc->setFixedHeight(34);
    col->addWidget(m_desc);

    col->addStretch();

    auto* bottom = new QHBoxLayout();
    bottom->setSpacing(8);
    m_cat = new QLabel(m_addon.categories.isEmpty() ? QString() : m_addon.categories.first(), this);
    m_cat->setObjectName(QStringLiteral("StoreCardCat"));
    bottom->addWidget(m_cat);
    bottom->addStretch();
    m_install = new QPushButton(this);
    m_install->setObjectName(QStringLiteral("StoreCardInstall"));
    m_install->setCursor(Qt::PointingHandCursor);
    m_install->setFixedHeight(30);
    connect(m_install, &QPushButton::clicked, this, [this]() {
        if (!m_installed)
            emit installRequested(m_addon);
        else
            emit openRequested(m_addon);
    });
    bottom->addWidget(m_install);
    col->addLayout(bottom);

    setInstalled(installed);

    if (!m_addon.logo.isEmpty())
        loadLogo(m_addon.logo);
}

void AddonCard::setInstalled(bool installed)
{
    m_installed = installed;
    m_install->setText(installed ? QStringLiteral("Installed") : QStringLiteral("Install"));
    m_install->setProperty("installed", installed);
    m_install->style()->unpolish(m_install);
    m_install->style()->polish(m_install);
}

void AddonCard::loadLogo(const QString& url)
{
    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    QNetworkReply* reply = logoNam()->get(req);
    QPointer<AddonCard> self(this);
    connect(reply, &QNetworkReply::finished, reply, [self, reply]() {
        reply->deleteLater();
        if (!self)
            return;
        if (reply->error() != QNetworkReply::NoError)
            return;
        QImage img;
        if (!img.loadFromData(reply->readAll()))
            return;
        const qreal dpr = self->devicePixelRatioF();
        QImage scaled = img.scaled(int(44 * dpr), int(44 * dpr), Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        QPixmap pm = QPixmap::fromImage(scaled);
        pm.setDevicePixelRatio(dpr);
        self->m_logo->setPixmap(pm);
    });
}

void AddonCard::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
        emit openRequested(m_addon);
}

void AddonCard::enterEvent(QEnterEvent*)
{
    setProperty("hover", true);
    style()->unpolish(this);
    style()->polish(this);
}

void AddonCard::leaveEvent(QEvent*)
{
    setProperty("hover", false);
    style()->unpolish(this);
    style()->polish(this);
}

} // namespace tankoban

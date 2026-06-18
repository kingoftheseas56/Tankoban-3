// Tankoban 3 — PosterCard (Step 3 / lazy). See PosterCard.h.

#include "ui/PosterCard.h"

#include <QEasingCurve>
#include <QEnterEvent>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QPoint>
#include <QPropertyAnimation>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPointer>
#include <QStandardPaths>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace tankoban {

namespace {
// App-lifetime shared manager for cover images, with an on-disk cache so re-launches
// are instant (the data path metahub allows via cache headers).
QNetworkAccessManager* imageNam()
{
    static QNetworkAccessManager* nam = [] {
        auto* m = new QNetworkAccessManager();
        auto* cache = new QNetworkDiskCache(m);
        cache->setCacheDirectory(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + QStringLiteral("/poster-cache"));
        cache->setMaximumCacheSize(qint64(256) * 1024 * 1024); // 256 MB
        m->setCache(cache);
        return m;
    }();
    return nam;
}
} // namespace

PosterCard::PosterCard(const MetaItem& item, QWidget* parent)
    : QWidget(parent)
    , m_url(item.poster)
    , m_item(item)
{
    setObjectName(QStringLiteral("PosterCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(kPosterW);
    setCursor(Qt::PointingHandCursor);

    auto* col = new QVBoxLayout(this);
    // Top padding = the hover lift, so the poster has room to rise to the card's top edge
    // on hover without being clipped by the card (Qt clips children to the parent rect).
    col->setContentsMargins(0, kHoverLift, 0, 0);
    col->setSpacing(8);

    m_poster = new QLabel(this);
    m_poster->setObjectName(QStringLiteral("Poster"));
    m_poster->setFixedSize(kPosterW, kPosterH);
    m_poster->setAlignment(Qt::AlignCenter);
    col->addWidget(m_poster);

    m_title = new QLabel(item.name, this);
    m_title->setObjectName(QStringLiteral("PosterTitle"));
    m_title->setFixedWidth(kPosterW);
    m_title->setWordWrap(false);
    col->addWidget(m_title);

    // Hover lift (Harbor PickCard group-hover:-translate-y-2): the poster glides up ~8px
    // on hover and settles back on leave. Only the poster moves; the title stays put.
    m_liftAnim = new QPropertyAnimation(m_poster, "pos", this);
    m_liftAnim->setDuration(260);
    m_liftAnim->setEasingCurve(QEasingCurve::OutCubic);
}

void PosterCard::ensureLoaded()
{
    if (m_loadRequested || m_url.isEmpty())
        return;
    m_loadRequested = true;
    loadPoster(m_url);
}

void PosterCard::loadPoster(const QString& url)
{
    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                     QNetworkRequest::PreferCache); // posters are immutable per imdb id

    QNetworkReply* reply = imageNam()->get(req);
    QPointer<PosterCard> self(this);
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
        const QSize physical(int(kPosterW * dpr), int(kPosterH * dpr));
        QImage scaled = img.scaled(physical, Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation);
        const int x = qMax(0, (scaled.width() - physical.width()) / 2);
        const int y = qMax(0, (scaled.height() - physical.height()) / 2);
        scaled = scaled.copy(x, y, qMin(physical.width(), scaled.width()),
                             qMin(physical.height(), scaled.height()));

        QPixmap pm = QPixmap::fromImage(scaled);
        pm.setDevicePixelRatio(dpr);
        self->m_poster->setPixmap(pm);
    });
}

void PosterCard::enterEvent(QEnterEvent*)
{
    setProperty("hover", true); // drives the gold ring via Theme QSS
    style()->unpolish(this);
    style()->polish(this);
    if (m_liftAnim) {
        m_liftAnim->stop();
        m_liftAnim->setStartValue(m_poster->pos());
        m_liftAnim->setEndValue(QPoint(0, 0)); // rise to the card's top edge
        m_liftAnim->start();
    }
}

void PosterCard::leaveEvent(QEvent*)
{
    setProperty("hover", false);
    style()->unpolish(this);
    style()->polish(this);
    if (m_liftAnim) {
        m_liftAnim->stop();
        m_liftAnim->setStartValue(m_poster->pos());
        m_liftAnim->setEndValue(QPoint(0, kHoverLift)); // settle back to rest
        m_liftAnim->start();
    }
}

void PosterCard::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
        emit activated(m_item);
}

} // namespace tankoban

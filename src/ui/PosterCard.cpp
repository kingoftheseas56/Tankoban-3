// Tankoban 3 — PosterCard (Step 3 / lazy). See PosterCard.h.

#include "ui/PosterCard.h"

#include "ui/Icons.h"

#include <QColor>
#include <QHBoxLayout>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
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
    col->setSpacing(10); // Harbor gap-2.5

    m_poster = new QLabel(this);
    m_poster->setObjectName(QStringLiteral("Poster"));
    m_poster->setFixedSize(kPosterW, kPosterH);
    m_poster->setAlignment(Qt::AlignCenter);
    col->addWidget(m_poster);

    m_title = new QLabel(item.name, this);
    m_title->setObjectName(QStringLiteral("PosterTitle"));
    m_title->setFixedWidth(kPosterW);
    m_title->setWordWrap(true); // Harbor line-clamp-2
    m_title->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    {
        // Clamp to exactly two lines (height from a 13px medium font, robust vs QSS timing).
        QFont tf;
        tf.setPixelSize(13);
        tf.setWeight(QFont::Medium);
        m_title->setFixedHeight(QFontMetrics(tf).lineSpacing() * 2);
    }
    col->addWidget(m_title);

    // Hover lift (Harbor PickCard group-hover:-translate-y-2): the poster glides up ~8px
    // on hover and settles back on leave. Only the poster moves; the title stays put.
    m_liftAnim = new QPropertyAnimation(m_poster, "pos", this);
    m_liftAnim->setDuration(260);
    m_liftAnim->setEasingCurve(QEasingCurve::OutCubic);

    // Harbor's hover shadow (deeper drop shadow under the lifted poster). Kept DISABLED at
    // rest — a disabled QGraphicsEffect renders nothing and costs nothing, so only the
    // hovered card (1-2 at a time) pays for it (safe on the UHD 620).
    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(28);
    m_shadow->setOffset(0, 12);
    m_shadow->setColor(QColor(0, 0, 0, 165));
    m_shadow->setEnabled(false);
    m_poster->setGraphicsEffect(m_shadow);

    // MAL score badge (Harbor ScoreStack, pick-card.tsx:163-235) — anime cards only, so
    // Movies/Shows stay clean (their IMDb badges aren't app-wide yet). A child of the poster
    // so it rides the hover-lift; bottom-end inset like Harbor's bottom-1.5 end-1.5.
    const bool isAnimeId = m_item.id.startsWith(QLatin1String("mal:"))
                           || m_item.id.startsWith(QLatin1String("kitsu:"))
                           || m_item.id.startsWith(QLatin1String("anilist:"))
                           || m_item.id.startsWith(QLatin1String("anidb:"));
    if (isAnimeId && !m_item.imdbRating.isEmpty()) {
        m_badge = new QWidget(m_poster);
        m_badge->setObjectName(QStringLiteral("ScoreBadge"));
        m_badge->setStyleSheet(QStringLiteral(
            "#ScoreBadge{background:rgba(18,19,23,0.95);border-radius:6px;}"));
        auto* brow = new QHBoxLayout(m_badge);
        brow->setContentsMargins(6, 2, 6, 2); // Harbor px-1.5 py-0.5
        brow->setSpacing(4);
        auto* logo = new QLabel(m_badge);
        logo->setPixmap(malLogoPixmap(QColor(QStringLiteral("#aab1bd")), 11)); // h-[11px]
        brow->addWidget(logo);
        auto* score = new QLabel(m_item.imdbRating, m_badge);
        score->setStyleSheet(QStringLiteral("color:#f3f1ea;font-size:10px;font-weight:600;"));
        brow->addWidget(score);
        m_badge->adjustSize();
        m_badge->move(m_cardW - m_badge->width() - 6, m_cardH - m_badge->height() - 6);
        m_badge->raise();
    }
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
        self->m_srcImage = img; // retain for crisp re-crop when the grid resizes the card
        self->applyPoster();
    });
}

void PosterCard::applyPoster()
{
    if (m_srcImage.isNull())
        return;
    const qreal dpr = devicePixelRatioF();
    const QSize physical(int(m_cardW * dpr), int(m_cardH * dpr));
    QImage scaled = m_srcImage.scaled(physical, Qt::KeepAspectRatioByExpanding,
                                      Qt::SmoothTransformation);
    const int x = qMax(0, (scaled.width() - physical.width()) / 2);
    const int y = qMax(0, (scaled.height() - physical.height()) / 2);
    scaled = scaled.copy(x, y, qMin(physical.width(), scaled.width()),
                         qMin(physical.height(), scaled.height()));
    QPixmap pm = QPixmap::fromImage(scaled);
    pm.setDevicePixelRatio(dpr);
    m_poster->setPixmap(pm);
}

void PosterCard::setCardWidth(int width)
{
    if (width <= 0 || width == m_cardW)
        return;
    m_cardW = width;
    m_cardH = width * 3 / 2; // 2:3 portrait
    setFixedWidth(m_cardW);
    m_poster->setFixedSize(m_cardW, m_cardH);
    m_title->setFixedWidth(m_cardW);
    applyPoster(); // re-crop the retained source to the new size (crisp, no upscale blur)
    if (m_badge)
        m_badge->move(m_cardW - m_badge->width() - 6, m_cardH - m_badge->height() - 6);
    updateGeometry();
}

void PosterCard::enterEvent(QEnterEvent*)
{
    setProperty("hover", true); // firms the inset ring via Theme QSS
    style()->unpolish(this);
    style()->polish(this);
    if (m_shadow)
        m_shadow->setEnabled(true);
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
    if (m_shadow)
        m_shadow->setEnabled(false);
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

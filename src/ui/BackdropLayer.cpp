// Tankoban 3 - Play Picker backdrop layer (M5). See BackdropLayer.h.

#include "ui/BackdropLayer.h"

#include <QImage>
#include <QLinearGradient>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>

namespace tankoban {

namespace {
const QColor kCanvas(0x12, 0x13, 0x17);

QNetworkAccessManager* backdropNam()
{
    static QNetworkAccessManager* nam = [] {
        auto* manager = new QNetworkAccessManager();
        auto* cache = new QNetworkDiskCache(manager);
        // Share DetailHero/FeaturedHero's disk cache so a backdrop already seen on
        // the detail page can paint quickly when the picker opens.
        cache->setCacheDirectory(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + QStringLiteral("/hero-cache"));
        cache->setMaximumCacheSize(qint64(128) * 1024 * 1024);
        manager->setCache(cache);
        return manager;
    }();
    return nam;
}

QString upgradeArt(QString url)
{
    url.replace(QStringLiteral("/background/small/"), QStringLiteral("/background/large/"));
    url.replace(QStringLiteral("/background/medium/"), QStringLiteral("/background/large/"));
    return url;
}
} // namespace

BackdropLayer::BackdropLayer(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void BackdropLayer::setSource(const QString& imageUrl)
{
    const QString upgraded = upgradeArt(imageUrl);
    if (upgraded == m_source)
        return;

    m_source = upgraded;
    m_cachedPixmap = QPixmap();
    m_scaledCache = QPixmap();   // invalidate the scaled cache for the new source
    m_scaledForSize = QSize();
    update();

    if (m_source.isEmpty())
        return;

    QNetworkRequest req((QUrl(m_source)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    QNetworkReply* reply = backdropNam()->get(req);
    QPointer<BackdropLayer> self(this);
    const QString expected = m_source;
    connect(reply, &QNetworkReply::finished, reply, [self, reply, expected]() {
        reply->deleteLater();
        if (!self || self->m_source != expected)
            return;
        if (reply->error() != QNetworkReply::NoError)
            return;
        QImage image;
        if (!image.loadFromData(reply->readAll()))
            return;
        self->m_cachedPixmap = QPixmap::fromImage(image);
        self->m_scaledCache = QPixmap();   // force one rescale at the next paint
        self->m_scaledForSize = QSize();
        self->update();
    });
}

void BackdropLayer::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    const QRect r = rect();
    p.fillRect(r, kCanvas);

    if (!m_cachedPixmap.isNull()) {
        const QSize target(qRound(r.width() * 1.05), qRound(r.height() * 1.05));
        // Rescale only when the target size changed; otherwise reuse the cached
        // bitmap. This turns every repaint (each scroll frame) from an expensive
        // full-image bicubic scale into a cheap blit.
        if (m_scaledCache.isNull() || m_scaledForSize != target) {
            m_scaledCache = m_cachedPixmap.scaled(target, Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation);
            m_scaledForSize = target;
        }
        const QPoint topLeft(r.center().x() - m_scaledCache.width() / 2,
                             r.center().y() - m_scaledCache.height() / 2);
        p.setOpacity(0.50);
        p.drawPixmap(topLeft, m_scaledCache);
        p.setOpacity(1.0);
    }

    QLinearGradient vertical(0, r.top(), 0, r.bottom());
    vertical.setColorAt(0.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 77));
    vertical.setColorAt(0.45, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 217));
    vertical.setColorAt(1.0, kCanvas);
    p.fillRect(r, vertical);

    QLinearGradient horizontal(r.left(), 0, r.right(), 0);
    horizontal.setColorAt(0.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 140));
    horizontal.setColorAt(0.5, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 0));
    horizontal.setColorAt(1.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 140));
    p.fillRect(r, horizontal);
}

} // namespace tankoban

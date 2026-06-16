// Tankoban 3 — DetailHero (Detail path). See DetailHero.h.

#include "ui/DetailHero.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace tankoban {

namespace {
const QColor kCanvas(0x12, 0x13, 0x17);

QNetworkAccessManager* artNam()
{
    static QNetworkAccessManager* nam = [] {
        auto* m = new QNetworkAccessManager();
        auto* cache = new QNetworkDiskCache(m);
        cache->setCacheDirectory(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + QStringLiteral("/hero-cache"));
        cache->setMaximumCacheSize(qint64(128) * 1024 * 1024);
        m->setCache(cache);
        return m;
    }();
    return nam;
}

QString upgradeArt(QString url)
{
    url.replace(QStringLiteral("/background/small/"), QStringLiteral("/background/large/"));
    url.replace(QStringLiteral("/background/medium/"), QStringLiteral("/background/large/"));
    return url;
}

QLabel* makePill(QWidget* parent, const QString& text)
{
    auto* p = new QLabel(text, parent);
    p->setObjectName(QStringLiteral("DetailPill"));
    p->setTextFormat(Qt::RichText);
    p->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    return p;
}
} // namespace

DetailHero::DetailHero(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("DetailHero"));
    setFixedHeight(kHeroH);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(48, 48, 48, 56); // Harbor px-12 pb-14
    col->addStretch(1);                       // anchor content to the bottom

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("DetailTitle"));
    m_title->setWordWrap(true);
    m_title->setMaximumWidth(820);
    col->addWidget(m_title);

    m_pillsRow = new QWidget(this);
    m_pillsRow->setAttribute(Qt::WA_TranslucentBackground, true);
    m_pillsLayout = new QHBoxLayout(m_pillsRow);
    m_pillsLayout->setContentsMargins(0, 0, 0, 0);
    m_pillsLayout->setSpacing(12);
    m_pillsLayout->addStretch();
    col->addSpacing(20);
    col->addWidget(m_pillsRow);

    auto* btns = new QWidget(this);
    btns->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* brow = new QHBoxLayout(btns);
    brow->setContentsMargins(0, 0, 0, 0);
    brow->setSpacing(12);
    m_play = new QPushButton(QStringLiteral("▶  Play"), btns);
    m_play->setObjectName(QStringLiteral("HeroPlayBtn")); // reuse hero button styling
    m_play->setCursor(Qt::PointingHandCursor);
    m_play->setFixedHeight(48);
    m_add = new QPushButton(QStringLiteral("＋  Add to Watchlist"), btns);
    m_add->setObjectName(QStringLiteral("HeroAddBtn"));
    m_add->setCursor(Qt::PointingHandCursor);
    m_add->setFixedHeight(48);
    brow->addWidget(m_play);
    brow->addWidget(m_add);
    brow->addStretch();
    col->addSpacing(28);
    col->addWidget(btns);

    connect(m_play, &QPushButton::clicked, this, [this]() { emit playClicked(); });
    connect(m_add, &QPushButton::clicked, this, [this]() {
        const bool now = !m_add->property("inwatch").toBool();
        m_add->setProperty("inwatch", now);
        m_add->setText(now ? QStringLiteral("✓  In Watchlist")
                           : QStringLiteral("＋  Add to Watchlist"));
        m_add->style()->unpolish(m_add);
        m_add->style()->polish(m_add);
    });
}

void DetailHero::setPreview(const MetaItem& preview)
{
    m_title->setText(preview.name);
    m_year = preview.releaseInfo;
    m_rating = preview.imdbRating;
    m_runtime = preview.runtime;
    m_genres.clear();
    rebuildPills();
    setBackdropUrl(upgradeArt(preview.background.isEmpty() ? preview.poster : preview.background));
}

void DetailHero::applyDetail(const MetaDetail& d)
{
    if (!d.name.isEmpty())
        m_title->setText(d.name);
    if (!d.releaseInfo.isEmpty())
        m_year = d.releaseInfo;
    if (!d.imdbRating.isEmpty())
        m_rating = d.imdbRating;
    if (!d.runtime.isEmpty())
        m_runtime = d.runtime;
    m_genres = d.genres;
    rebuildPills();
    if (!d.background.isEmpty())
        setBackdropUrl(upgradeArt(d.background));
}

void DetailHero::rebuildPills()
{
    // clear existing pills (keep the trailing stretch)
    while (m_pillsLayout->count() > 1) {
        QLayoutItem* it = m_pillsLayout->takeAt(0);
        if (it->widget())
            it->widget()->deleteLater();
        delete it;
    }
    auto add = [this](const QString& html) {
        m_pillsLayout->insertWidget(m_pillsLayout->count() - 1, makePill(m_pillsRow, html));
    };
    if (!m_year.isEmpty())
        add(m_year.toHtmlEscaped());
    if (!m_rating.isEmpty())
        add(QStringLiteral("<span style='color:#e8b923;font-weight:700'>IMDb</span>&nbsp;"
                           "<span style='color:#f3f1ea;font-weight:600'>%1</span>")
                .arg(m_rating.toHtmlEscaped()));
    if (!m_runtime.isEmpty())
        add(m_runtime.toHtmlEscaped());
    for (int i = 0; i < m_genres.size() && i < 3; ++i)
        add(m_genres.at(i).toHtmlEscaped());
}

void DetailHero::setBackdropUrl(const QString& url)
{
    if (url == m_bgUrl || url.isEmpty())
        return;
    m_bgUrl = url;
    // Drop the previous title's art immediately so it can't linger on this page — the
    // cached scaled pixmap would otherwise survive (window size unchanged) and show the
    // last title's backdrop until the new one arrives (the off-by-one bug).
    m_bgRaw = QImage();
    m_bgPix = QPixmap();
    update();

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    QNetworkReply* reply = artNam()->get(req);
    QPointer<DetailHero> self(this);
    connect(reply, &QNetworkReply::finished, reply, [self, reply, url]() {
        reply->deleteLater();
        if (!self || self->m_bgUrl != url)
            return;
        if (reply->error() != QNetworkReply::NoError)
            return;
        QImage img;
        if (!img.loadFromData(reply->readAll()))
            return;
        self->m_bgRaw = img;
        self->m_bgPix = QPixmap(); // force re-scale from the new art (window size unchanged)
        self->update();
    });
}

void DetailHero::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    m_bgPix = QPixmap(); // force re-scale on next paint
}

void DetailHero::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    const QRect r = rect();
    p.fillRect(r, kCanvas);

    if (!m_bgRaw.isNull()) {
        if (m_bgPix.isNull() || m_bgPix.size() != r.size() * devicePixelRatioF()) {
            const qreal dpr = devicePixelRatioF();
            const QSize phys(int(r.width() * dpr), int(r.height() * dpr));
            QImage scaled =
                m_bgRaw.scaled(phys, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            const int x = qMax(0, (scaled.width() - phys.width()) / 2);
            const int y = qMax(0, (scaled.height() - phys.height()) / 2);
            scaled = scaled.copy(x, y, qMin(phys.width(), scaled.width()),
                                 qMin(phys.height(), scaled.height()));
            m_bgPix = QPixmap::fromImage(scaled);
            m_bgPix.setDevicePixelRatio(dpr);
        }
        p.drawPixmap(QPoint(0, 0), m_bgPix);
    }

    // Bottom-up melt: from-canvas via-canvas/55 (45%) to-transparent.
    QLinearGradient vg(0, r.bottom(), 0, r.top());
    vg.setColorAt(0.0, kCanvas);
    vg.setColorAt(0.45, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 140));
    vg.setColorAt(1.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 0));
    p.fillRect(r, vg);

    // Left melt: from-canvas/85 via-canvas/35 to-transparent.
    QLinearGradient hg(r.left(), 0, r.right(), 0);
    hg.setColorAt(0.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 217));
    hg.setColorAt(0.5, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 89));
    hg.setColorAt(1.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 0));
    p.fillRect(r, hg);
}

} // namespace tankoban

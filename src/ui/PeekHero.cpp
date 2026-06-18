// Tankoban 3 — PeekHero. See PeekHero.h.

#include "ui/PeekHero.h"

#include <QColor>
#include <QEnterEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace tankoban {

namespace {
// Drag gesture constants (Harbor peek-hero.tsx).
constexpr int kDragThreshold = 8;     // DRAG_THRESHOLD
constexpr qreal kSnapFraction = 0.15; // SNAP_FRACTION
constexpr qreal kFlickVel = 0.45;     // FLICK_VELOCITY (px/ms)
constexpr int kDotsBand = 34;         // space reserved below the track for dot pills

QString upgradeBackground(QString url)
{
    url.replace(QStringLiteral("/background/small/"), QStringLiteral("/background/large/"));
    url.replace(QStringLiteral("/background/medium/"), QStringLiteral("/background/large/"));
    return url;
}
} // namespace

PeekHero::PeekHero(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("PeekHero"));
    setAttribute(Qt::WA_StyledBackground, true);
    // No mouse tracking: move events fire only while a button is held (the drag), never on
    // a bare hover — otherwise a missed release makes hovering drag the carousel.
    setMinimumHeight(kTrackH + kDotsBand);

    m_nam = new QNetworkAccessManager(this);
    auto* cache = new QNetworkDiskCache(m_nam);
    cache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                             + QStringLiteral("/shows-hero-cache"));
    cache->setMaximumCacheSize(qint64(128) * 1024 * 1024);
    m_nam->setCache(cache);

    // Active-card content overlay (bottom-left): title + meta + Play/Episodes.
    m_overlay = new QWidget(this);
    m_overlay->setObjectName(QStringLiteral("PeekOverlay"));
    m_overlay->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* ocol = new QVBoxLayout(m_overlay);
    ocol->setContentsMargins(40, 36, 40, 36);
    ocol->setSpacing(12);
    ocol->addStretch();

    m_title = new QLabel(m_overlay);
    m_title->setObjectName(QStringLiteral("PeekTitle"));
    m_title->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_title->setWordWrap(true);
    m_title->setStyleSheet(QStringLiteral(
        "#PeekTitle{font-family:'Fraunces','Georgia',serif;font-size:34px;font-weight:600;"
        "color:#ffffff;}"));
    ocol->addWidget(m_title);

    m_meta = new QLabel(m_overlay);
    m_meta->setObjectName(QStringLiteral("PeekMeta"));
    m_meta->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_meta->setTextFormat(Qt::RichText);
    m_meta->setStyleSheet(QStringLiteral("#PeekMeta{font-size:13px;color:rgba(255,255,255,0.82);}"));
    ocol->addWidget(m_meta);
    ocol->addSpacing(4);

    auto* btns = new QWidget(m_overlay);
    btns->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* brow = new QHBoxLayout(btns);
    brow->setContentsMargins(0, 0, 0, 0);
    brow->setSpacing(10);
    m_play = new QPushButton(QStringLiteral("▶  Play"), btns);
    m_play->setObjectName(QStringLiteral("HeroPlayBtn")); // reuse hero button styling
    m_play->setCursor(Qt::PointingHandCursor);
    m_play->setFixedHeight(40);
    m_episodes = new QPushButton(QStringLiteral("Episodes"), btns);
    m_episodes->setObjectName(QStringLiteral("HeroAddBtn"));
    m_episodes->setCursor(Qt::PointingHandCursor);
    m_episodes->setFixedHeight(40);
    brow->addWidget(m_play);
    brow->addWidget(m_episodes);
    brow->addStretch();
    ocol->addWidget(btns);

    auto openActive = [this]() {
        if (m_active >= 0 && m_active < m_slides.size())
            emit openDetailRequested(m_slides[m_active]);
    };
    connect(m_play, &QPushButton::clicked, this, openActive);
    connect(m_episodes, &QPushButton::clicked, this, openActive);

    // Dot pills below the track.
    m_dotsRow = new QWidget(this);
    m_dotsRow->setAttribute(Qt::WA_TranslucentBackground, true);
    m_dotsLayout = new QHBoxLayout(m_dotsRow);
    m_dotsLayout->setContentsMargins(0, 0, 0, 0);
    m_dotsLayout->setSpacing(8);

    m_autoTimer = new QTimer(this);
    m_autoTimer->setInterval(9500); // Harbor ROTATE_MS
    connect(m_autoTimer, &QTimer::timeout, this, [this]() { next(); });
}

void PeekHero::setSlides(const QVector<MetaItem>& slides)
{
    m_slides = slides;
    m_active = 0;
    m_dragDx = 0.0;
    rebuildDots();
    updateActiveContent();
    requestVisibleBgs();
    positionChildren();
    update();
    if (m_slides.size() > 1)
        m_autoTimer->start();
    else
        m_autoTimer->stop();
    setCursor(m_slides.size() > 1 ? Qt::OpenHandCursor : Qt::PointingHandCursor);
}

int PeekHero::wrap(int i) const
{
    const int n = m_slides.size();
    if (n <= 0)
        return 0;
    return ((i % n) + n) % n;
}

int PeekHero::cardWidth() const
{
    return qMin(kCardMaxW, int(width() * 0.70));
}

QRect PeekHero::cardRect(int offset, qreal scale, qreal dragDx) const
{
    const int cw = qMax(1, cardWidth());
    // Aspect-lock to Harbor's 920:420 landscape ratio (capped at 420 tall), so the card
    // stays wide/flat instead of going tall when the window narrows the 70% width.
    const int chBase = qMin(kCardH, cw * kCardH / kCardMaxW);
    const int w = int(cw * scale);
    const int h = int(chBase * scale);
    const qreal shift = offset * 0.75 * cw + (dragDx * cw / qreal(qMax(1, width())));
    const int cx = width() / 2 + int(shift);
    const int x = cx - w / 2;
    const int y = (kTrackH - h) / 2;
    return QRect(x, y, w, h);
}

void PeekHero::drawCard(QPainter& p, const MetaItem& m, const QRect& r, qreal opacity)
{
    QPainterPath path;
    path.addRoundedRect(r, 20, 20);
    p.save();
    p.setClipPath(path);
    p.setOpacity(opacity);

    const QImage img = m_bg.value(m.id);
    if (!img.isNull()) {
        const qreal ar = r.width() / qreal(qMax(1, r.height()));
        const qreal iar = img.width() / qreal(qMax(1, img.height()));
        QRect src;
        if (iar > ar) {
            const int sw = qRound(img.height() * ar);
            src = QRect((img.width() - sw) / 2, 0, sw, img.height());
        } else {
            const int sh = qRound(img.width() / ar);
            src = QRect(0, (img.height() - sh) / 2, img.width(), sh);
        }
        p.drawImage(r, img, src);
    } else {
        p.fillRect(r, QColor(0x1a, 0x1d, 0x24));
    }

    // Bottom-up scrim (Harbor from-black/85 via-black/35 to-transparent).
    QLinearGradient g(r.left(), r.bottom(), r.left(), r.top());
    g.setColorAt(0.0, QColor(0, 0, 0, 217));
    g.setColorAt(0.45, QColor(0, 0, 0, 90));
    g.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(r, g);

    p.restore();
}

void PeekHero::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (m_slides.isEmpty()) {
        QPainterPath ph;
        ph.addRoundedRect(cardRect(0, 1.0, 0.0), 20, 20);
        p.fillPath(ph, QColor(0x1a, 0x1d, 0x24));
        return;
    }

    // Neighbours first (z below), active last (z on top).
    if (m_slides.size() > 1) {
        drawCard(p, m_slides[wrap(m_active - 1)], cardRect(-1, 0.86, m_dragDx), 0.45);
        drawCard(p, m_slides[wrap(m_active + 1)], cardRect(1, 0.86, m_dragDx), 0.45);
    }
    drawCard(p, m_slides[wrap(m_active)], cardRect(0, 1.0, m_dragDx), 1.0);
}

void PeekHero::requestVisibleBgs()
{
    if (m_slides.isEmpty())
        return;
    loadBg(m_active);
    if (m_slides.size() > 1) {
        loadBg(m_active - 1);
        loadBg(m_active + 1);
    }
}

void PeekHero::loadBg(int index)
{
    if (m_slides.isEmpty())
        return;
    index = wrap(index);
    const MetaItem& m = m_slides[index];
    if (m.id.isEmpty() || m.background.isEmpty())
        return;
    if (m_bg.contains(m.id) || m_bgRequested.contains(m.id))
        return;
    m_bgRequested.insert(m.id);

    QNetworkRequest req((QUrl(upgradeBackground(m.background))));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    QNetworkReply* reply = m_nam->get(req);
    QPointer<PeekHero> self(this);
    const QString id = m.id;
    connect(reply, &QNetworkReply::finished, reply, [self, reply, id]() {
        reply->deleteLater();
        if (!self)
            return;
        if (reply->error() != QNetworkReply::NoError)
            return;
        QImage img;
        if (!img.loadFromData(reply->readAll()))
            return;
        self->m_bg.insert(id, img);
        self->update();
    });
}

void PeekHero::goTo(int idx)
{
    idx = wrap(idx);
    if (idx == m_active)
        return;
    m_active = idx;
    updateActiveContent();
    requestVisibleBgs();
    positionChildren();
    update();
    syncDots();
    if (m_slides.size() > 1)
        m_autoTimer->start(); // reset the rotate window on manual nav
}

void PeekHero::next()
{
    if (m_slides.size() < 2)
        return;
    goTo(m_active + 1);
}

void PeekHero::rebuildDots()
{
    for (QPushButton* d : m_dots)
        d->deleteLater();
    m_dots.clear();
    if (m_slides.size() < 2)
        return;
    for (int i = 0; i < m_slides.size(); ++i) {
        auto* dot = new QPushButton(m_dotsRow);
        dot->setObjectName(QStringLiteral("HeroDot")); // reuse dot styling
        dot->setCursor(Qt::PointingHandCursor);
        dot->setFixedHeight(6);
        connect(dot, &QPushButton::clicked, this, [this, i]() { goTo(i); });
        m_dotsLayout->addWidget(dot);
        m_dots.push_back(dot);
    }
    syncDots();
}

void PeekHero::syncDots()
{
    for (int i = 0; i < m_dots.size(); ++i) {
        QPushButton* d = m_dots[i];
        const bool active = (i == m_active);
        d->setFixedWidth(active ? 32 : 6); // Harbor active w-8, rest w-1.5
        d->setProperty("active", active);
        d->style()->unpolish(d);
        d->style()->polish(d);
    }
}

void PeekHero::updateActiveContent()
{
    if (m_slides.isEmpty())
        return;
    const MetaItem& m = m_slides[wrap(m_active)];
    m_title->setText(m.name);

    // Meta row: YEAR · GENRE · GENRE (tracked uppercase). IMDb is intentionally omitted —
    // ratings aren't an app-wide feature yet, and a rating on only some titles reads broken.
    QStringList segs;
    if (!m.releaseInfo.isEmpty())
        segs << QStringLiteral("<span style='letter-spacing:1.5px'>%1</span>")
                    .arg(m.releaseInfo.toUpper().toHtmlEscaped());
    if (!m.genres.isEmpty()) {
        QStringList g;
        for (int i = 0; i < m.genres.size() && i < 2; ++i)
            g << m.genres.at(i);
        segs << QStringLiteral("<span style='letter-spacing:1.5px'>%1</span>")
                    .arg(g.join(QStringLiteral(" · ")).toUpper().toHtmlEscaped());
    }
    m_meta->setText(segs.join(QStringLiteral("&nbsp;&nbsp;·&nbsp;&nbsp;")));
    m_meta->setVisible(!segs.isEmpty());
}

void PeekHero::positionChildren()
{
    if (m_slides.isEmpty()) {
        m_overlay->hide();
        m_dotsRow->hide();
        return;
    }
    const QRect r = cardRect(0, 1.0, 0.0);
    m_overlay->setGeometry(r);
    m_overlay->setVisible(!m_dragMoved);
    m_overlay->raise();

    if (m_dots.size() > 1) {
        m_dotsRow->adjustSize();
        int w = m_dotsRow->width();
        if (w <= 0)
            w = m_dots.size() * 16;
        m_dotsRow->setGeometry((width() - w) / 2, kTrackH + 10, w, 16);
        m_dotsRow->show();
        m_dotsRow->raise();
    } else {
        m_dotsRow->hide();
    }
}

void PeekHero::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    positionChildren();
}

void PeekHero::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
        return;
    m_dragActive = true;
    m_dragMoved = false;
    m_startX = e->pos().x();
    m_lastX = m_startX;
    if (!m_clock.isValid())
        m_clock.start();
    m_lastT = m_clock.elapsed();
    m_vel = 0.0;
    m_dragDx = 0.0;
}

void PeekHero::mouseMoveEvent(QMouseEvent* e)
{
    // A move with no left button held = a missed release left us stuck; reset so a bare
    // hover can never drag.
    if (!(e->buttons() & Qt::LeftButton)) {
        m_dragActive = false;
        m_dragMoved = false;
        return;
    }
    if (!m_dragActive)
        return;
    const int x = e->pos().x();
    const int dx = x - m_startX;
    const qint64 now = m_clock.isValid() ? m_clock.elapsed() : 0;
    const qint64 dt = now - m_lastT;
    if (dt > 0) {
        const qreal inst = qreal(x - m_lastX) / qreal(dt);
        m_vel = m_vel * 0.6 + inst * 0.4;
    }
    m_lastX = x;
    m_lastT = now;

    if (!m_dragMoved && qAbs(dx) > kDragThreshold && m_slides.size() > 1) {
        m_dragMoved = true;
        m_overlay->hide(); // content snaps out while the cards slide under the cursor
        setCursor(Qt::ClosedHandCursor);
    }
    if (m_dragMoved) {
        m_dragDx = dx;
        update();
    }
}

void PeekHero::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_dragActive)
        return;
    m_dragActive = false;

    if (m_dragMoved) {
        const qreal total = qreal(qMax(1, width()));
        const qreal dist = qreal(m_lastX - m_startX);
        const bool passed = qAbs(dist) > total * kSnapFraction;
        const bool flick = qAbs(m_vel) > kFlickVel;
        int dir = 0;
        if (passed || flick)
            dir = dist < 0 ? +1 : -1;
        m_dragDx = 0.0;
        m_dragMoved = false;
        setCursor(m_slides.size() > 1 ? Qt::OpenHandCursor : Qt::PointingHandCursor);
        if (dir != 0)
            goTo(m_active + dir);
        else {
            positionChildren();
            update();
        }
    } else if (e->button() == Qt::LeftButton) {
        if (m_active >= 0 && m_active < m_slides.size())
            emit openDetailRequested(m_slides[m_active]); // click (no drag) → detail
    }
}

void PeekHero::enterEvent(QEnterEvent*)
{
    m_autoTimer->stop(); // pause auto-rotate on hover (Harbor setPaused)
}

void PeekHero::leaveEvent(QEvent*)
{
    if (m_slides.size() > 1)
        m_autoTimer->start();
}

} // namespace tankoban

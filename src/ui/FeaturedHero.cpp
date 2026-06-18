// Tankoban 3 — FeaturedHero (Step 3b). See FeaturedHero.h.

#include "ui/FeaturedHero.h"

#include <functional>

#include <QElapsedTimer>
#include <QEnterEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QUrl>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace tankoban {

namespace {

const QColor kCanvas(0x12, 0x13, 0x17);

// Hero drag gesture (Harbor cinema-hero.tsx): a press becomes a drag past kDragBudge px;
// on release, a drag past kSnapRatio of the width OR a flick over kFlickVel (px/ms)
// advances/retreats a slide; otherwise it's a plain click that opens detail.
constexpr int kDragBudge = 6;       // Harbor DRAG_BUDGE
constexpr qreal kSnapRatio = 0.18;  // Harbor SNAP_RATIO
constexpr qreal kFlickVel = 0.45;   // Harbor FLICK_VELOCITY (px/ms)

// App-lifetime shared manager for hero backdrops, with its own on-disk cache so
// re-launches paint the hero instantly (same approach as PosterCard, separate dir).
QNetworkAccessManager* heroNam()
{
    static QNetworkAccessManager* nam = [] {
        auto* m = new QNetworkAccessManager();
        auto* cache = new QNetworkDiskCache(m);
        cache->setCacheDirectory(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + QStringLiteral("/hero-cache"));
        cache->setMaximumCacheSize(qint64(128) * 1024 * 1024); // 128 MB
        m->setCache(cache);
        return m;
    }();
    return nam;
}

// The hero is a large element, so pull the largest backdrop metahub offers (the
// cards use /medium — that's right for 150px, this is not). Honors the sharpness bar.
QString upgradeBackground(QString url)
{
    url.replace(QStringLiteral("/background/small/"), QStringLiteral("/background/large/"));
    url.replace(QStringLiteral("/background/medium/"), QStringLiteral("/background/large/"));
    return url;
}

QString buildStatsHtml(const HeroSlide& s)
{
    QStringList parts;
    const QString& year = s.meta.releaseInfo;
    if (!year.isEmpty())
        parts << QStringLiteral("<span style='color:#8b909b'>Year: </span>"
                                "<span style='color:#f3f1ea'>%1</span>")
                     .arg(year.toHtmlEscaped());
    if (!s.meta.imdbRating.isEmpty())
        parts << QStringLiteral("<span style='color:#e8b923;font-weight:700'>IMDb</span>&nbsp;"
                                "<span style='color:#f3f1ea;font-weight:600'>%1</span>")
                     .arg(s.meta.imdbRating.toHtmlEscaped());
    if (!s.meta.runtime.isEmpty())
        parts << QStringLiteral("<span style='color:#8b909b'>Runtime: </span>"
                                "<span style='color:#f3f1ea'>%1</span>")
                     .arg(s.meta.runtime.toHtmlEscaped());
    return parts.join(QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"));
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// HeroPanel — the painted billboard: backdrop + art-melt scrims + content. Plain
// QWidget (no Q_OBJECT); FeaturedHero drives it via callbacks + the button accessors.
// ─────────────────────────────────────────────────────────────────────────────
class HeroPanel : public QWidget {
public:
    explicit HeroPanel(QWidget* parent = nullptr);

    void applySlide(const HeroSlide& slide, const QString& bgUrl, bool animate);

    QPushButton* playBtn() const { return m_play; }
    QPushButton* addBtn() const { return m_add; }

    std::function<void()> onClicked;
    std::function<void()> onHoverEnter;
    std::function<void()> onHoverLeave;
    std::function<void(int)> onSwipe;  // dir: -1 prev, +1 next (drag/flick on release)
    std::function<bool()> canDrag;     // true when there is more than one slide

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void setBackdropUrl(const QString& url, bool animate);
    void rescaleBackdrops();
    QPixmap coverScaled(const QImage& src) const;

    // Drag gesture state (left-press → potential drag; see mouse handlers).
    bool m_dragActive = false;
    bool m_dragMoved = false;
    int m_startX = 0;
    int m_lastX = 0;
    qint64 m_lastT = 0;
    qreal m_vel = 0.0;
    int m_contentBaseX = 0;
    QElapsedTimer m_clock;

    QLabel* m_rank = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_desc = nullptr;
    QLabel* m_stats = nullptr;
    QPushButton* m_play = nullptr;
    QPushButton* m_add = nullptr;
    QWidget* m_content = nullptr;
    QGraphicsOpacityEffect* m_contentFx = nullptr;

    QImage m_rawCur, m_rawPrev;
    QPixmap m_pxCur, m_pxPrev;
    qreal m_fade = 1.0; // 0..1 prev→cur blend
    QString m_url;
};

HeroPanel::HeroPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("HeroPanel"));
    setCursor(Qt::PointingHandCursor);
    // No mouse tracking: move events should fire ONLY while a button is held (the drag),
    // never on a bare hover — otherwise a missed release makes hovering drag the carousel.

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(56, 56, 56, 56); // Harbor p-14

    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("HeroContent"));
    m_content->setAttribute(Qt::WA_TranslucentBackground, true);
    m_content->setMaximumWidth(620); // Harbor max-w-2xl
    m_contentFx = new QGraphicsOpacityEffect(m_content);
    m_content->setGraphicsEffect(m_contentFx);

    auto* col = new QVBoxLayout(m_content);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    m_rank = new QLabel(m_content);
    m_rank->setObjectName(QStringLiteral("HeroRank"));
    m_rank->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_rank->setTextFormat(Qt::RichText);
    m_rank->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    col->addWidget(m_rank, 0, Qt::AlignLeft);
    col->addSpacing(20);

    m_title = new QLabel(m_content);
    m_title->setObjectName(QStringLiteral("HeroTitle")); // Fraunces 60px via Theme QSS
    m_title->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_title->setWordWrap(true);
    col->addWidget(m_title);
    col->addSpacing(24);

    m_desc = new QLabel(m_content);
    m_desc->setObjectName(QStringLiteral("HeroDesc"));
    m_desc->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_desc->setWordWrap(true);
    m_desc->setMaximumWidth(560); // Harbor max-w-xl
    col->addWidget(m_desc);
    col->addSpacing(24);

    m_stats = new QLabel(m_content);
    m_stats->setObjectName(QStringLiteral("HeroStats"));
    m_stats->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_stats->setTextFormat(Qt::RichText);
    col->addWidget(m_stats);
    col->addSpacing(36);

    auto* btns = new QWidget(m_content);
    btns->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* brow = new QHBoxLayout(btns);
    brow->setContentsMargins(0, 0, 0, 0);
    brow->setSpacing(12);

    m_play = new QPushButton(QStringLiteral("▶  Play"), btns);
    m_play->setObjectName(QStringLiteral("HeroPlayBtn"));
    m_play->setCursor(Qt::PointingHandCursor);
    m_play->setFixedHeight(48);
    brow->addWidget(m_play);

    m_add = new QPushButton(QStringLiteral("＋  Add to Watchlist"), btns);
    m_add->setObjectName(QStringLiteral("HeroAddBtn"));
    m_add->setCursor(Qt::PointingHandCursor);
    m_add->setFixedHeight(48);
    brow->addWidget(m_add);
    brow->addStretch();

    col->addWidget(btns);

    row->addWidget(m_content, 0, Qt::AlignVCenter);
    row->addStretch(1);
}

void HeroPanel::applySlide(const HeroSlide& slide, const QString& bgUrl, bool animate)
{
    m_rank->setText(QStringLiteral("<span style='color:#e8b923'>↗</span>&nbsp; "
                                   "#%1 in %2 Today")
                        .arg(slide.position)
                        .arg(slide.rankLabel.toHtmlEscaped()));

    m_title->setText(slide.meta.name);

    QString desc = slide.meta.description;
    if (desc.size() > 240)
        desc = desc.left(239).trimmed() + QStringLiteral("…");
    m_desc->setText(desc);
    m_desc->setVisible(!desc.isEmpty());

    const QString stats = buildStatsHtml(slide);
    m_stats->setText(stats);
    m_stats->setVisible(!stats.isEmpty());

    // Each slide starts fresh (watchlist isn't persisted yet — Library step).
    m_add->setText(QStringLiteral("＋  Add to Watchlist"));
    m_add->setProperty("inwatch", false);
    style()->unpolish(m_add);
    style()->polish(m_add);

    if (animate) {
        auto* fx = new QPropertyAnimation(m_contentFx, "opacity", this);
        fx->setDuration(450);
        fx->setStartValue(0.0);
        fx->setEndValue(1.0);
        fx->setEasingCurve(QEasingCurve::OutCubic);
        fx->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        m_contentFx->setOpacity(1.0);
    }

    setBackdropUrl(bgUrl, animate);
}

void HeroPanel::setBackdropUrl(const QString& url, bool animate)
{
    if (url == m_url)
        return;
    m_url = url;
    if (url.isEmpty())
        return;

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                     QNetworkRequest::PreferCache); // backdrops are immutable per imdb id

    QNetworkReply* reply = heroNam()->get(req);
    QPointer<HeroPanel> self(this);
    connect(reply, &QNetworkReply::finished, reply, [self, reply, url, animate]() {
        reply->deleteLater();
        if (!self)
            return;
        if (reply->error() != QNetworkReply::NoError)
            return;
        if (self->m_url != url)
            return; // a newer slide superseded this load
        QImage img;
        if (!img.loadFromData(reply->readAll()))
            return;

        self->m_rawPrev = self->m_rawCur;
        self->m_pxPrev = self->m_pxCur;
        self->m_rawCur = img;
        self->m_pxCur = self->coverScaled(img);

        if (animate && !self->m_pxPrev.isNull()) {
            auto* a = new QVariantAnimation(self);
            a->setDuration(700);
            a->setStartValue(0.0);
            a->setEndValue(1.0);
            a->setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));
            connect(a, &QVariantAnimation::valueChanged, self,
                    [self](const QVariant& v) {
                        if (self) {
                            self->m_fade = v.toReal();
                            self->update();
                        }
                    });
            a->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            self->m_fade = 1.0;
            self->update();
        }
    });
}

QPixmap HeroPanel::coverScaled(const QImage& src) const
{
    const int w = width() - 4;
    const int h = height() - 4;
    if (w <= 0 || h <= 0 || src.isNull())
        return {};
    const qreal dpr = devicePixelRatioF();
    const QSize phys(int(w * dpr), int(h * dpr));
    QImage scaled = src.scaled(phys, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int x = qMax(0, (scaled.width() - phys.width()) / 2);
    const int y = qMax(0, (scaled.height() - phys.height()) / 2);
    scaled = scaled.copy(x, y, qMin(phys.width(), scaled.width()),
                         qMin(phys.height(), scaled.height()));
    QPixmap pm = QPixmap::fromImage(scaled);
    pm.setDevicePixelRatio(dpr);
    return pm;
}

void HeroPanel::rescaleBackdrops()
{
    if (!m_rawCur.isNull())
        m_pxCur = coverScaled(m_rawCur);
    if (!m_rawPrev.isNull())
        m_pxPrev = coverScaled(m_rawPrev);
    update();
}

void HeroPanel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    const QRectF full(0, 0, width(), height());
    QPainterPath outer;
    outer.addRoundedRect(full, 28, 28); // Harbor rounded-[28px]
    p.fillPath(outer, kCanvas);          // 2px canvas frame shows around the art

    const QRectF inner = full.adjusted(2, 2, -2, -2);
    QPainterPath innerPath;
    innerPath.addRoundedRect(inner, 26, 26);
    p.setClipPath(innerPath);

    // Backdrop (cross-fading prev → cur), at Harbor's 0.9 opacity.
    if (!m_pxPrev.isNull() && m_fade < 1.0) {
        p.setOpacity(0.9 * (1.0 - m_fade));
        p.drawPixmap(QPointF(2, 2), m_pxPrev);
    }
    if (!m_pxCur.isNull()) {
        p.setOpacity(0.9 * m_fade);
        p.drawPixmap(QPointF(2, 2), m_pxCur);
    }
    p.setOpacity(1.0);

    // Horizontal art-melt: from-canvas via-canvas/85 (50%) to-transparent.
    QLinearGradient hg(inner.left(), 0, inner.right(), 0);
    hg.setColorAt(0.0, kCanvas);
    hg.setColorAt(0.5, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 217));
    hg.setColorAt(1.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 0));
    p.fillRect(inner, hg);

    // Bottom melt over the lower 2/5: from-canvas via-canvas/70 (50%) to-transparent.
    const qreal bandH = inner.height() * 0.4;
    const qreal bandTop = inner.bottom() - bandH;
    QLinearGradient bg(0, inner.bottom(), 0, bandTop);
    bg.setColorAt(0.0, kCanvas);
    bg.setColorAt(0.5, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 178));
    bg.setColorAt(1.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 0));
    p.fillRect(QRectF(inner.left(), bandTop, inner.width(), bandH), bg);
}

void HeroPanel::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
        return;
    // Begin a potential drag; whether it's a drag or a click is decided on release.
    m_dragActive = true;
    m_dragMoved = false;
    m_startX = e->pos().x();
    m_lastX = m_startX;
    if (!m_clock.isValid())
        m_clock.start();
    m_lastT = m_clock.elapsed();
    m_vel = 0.0;
    m_contentBaseX = m_content ? m_content->x() : 0;
}

void HeroPanel::mouseMoveEvent(QMouseEvent* e)
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
        m_vel = m_vel * 0.6 + inst * 0.4; // Harbor's velocity smoothing
    }
    m_lastX = x;
    m_lastT = now;

    if (!m_dragMoved && qAbs(dx) > kDragBudge && canDrag && canDrag()) {
        m_dragMoved = true;
        setCursor(Qt::ClosedHandCursor);
    }
    // Live feedback: the content block follows the cursor while dragging. (The backdrop
    // is a cross-fade layer, not a translate track, so a full whole-slide track-follow
    // with a neighbour reveal is out of scope here — see report.)
    if (m_dragMoved && m_content)
        m_content->move(m_contentBaseX + dx, m_content->y());
}

void HeroPanel::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_dragActive)
        return;
    m_dragActive = false;

    if (m_dragMoved) {
        const qreal threshold = qreal(qMax(1, width())) * kSnapRatio;
        const qreal dist = qreal(m_lastX - m_startX);
        int dir = 0;
        if (dist < -threshold || m_vel < -kFlickVel)
            dir = +1; // dragged/flicked left → next slide
        else if (dist > threshold || m_vel > kFlickVel)
            dir = -1; // dragged/flicked right → previous slide
        if (m_content)
            m_content->move(m_contentBaseX, m_content->y()); // snap content home
        setCursor((canDrag && canDrag()) ? Qt::OpenHandCursor : Qt::PointingHandCursor);
        m_dragMoved = false;
        if (dir != 0 && onSwipe)
            onSwipe(dir);
    } else if (e->button() == Qt::LeftButton && onClicked) {
        onClicked(); // a click with no drag → open detail
    }
}

void HeroPanel::enterEvent(QEnterEvent*)
{
    if (onHoverEnter)
        onHoverEnter();
}

void HeroPanel::leaveEvent(QEvent*)
{
    if (onHoverLeave)
        onHoverLeave();
}

void HeroPanel::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    rescaleBackdrops();
}

// ─────────────────────────────────────────────────────────────────────────────
// FeaturedHero — the carousel: panel + dots + 13s auto-advance.
// ─────────────────────────────────────────────────────────────────────────────
FeaturedHero::FeaturedHero(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("FeaturedHero"));

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(20); // Harbor gap-5 between viewport and dots

    m_panel = new HeroPanel(this);
    m_panel->setFixedHeight(kHeroH);
    col->addWidget(m_panel);

    m_dotsRow = new QWidget(this);
    m_dotsRow->setAttribute(Qt::WA_TranslucentBackground, true);
    m_dotsLayout = new QHBoxLayout(m_dotsRow);
    m_dotsLayout->setContentsMargins(0, 0, 0, 0);
    m_dotsLayout->setSpacing(10);
    m_dotsLayout->addStretch();
    m_dotsLayout->addStretch();
    col->addWidget(m_dotsRow);

    m_autoTimer = new QTimer(this);
    m_autoTimer->setInterval(13000); // Harbor: 13s
    connect(m_autoTimer, &QTimer::timeout, this, [this]() { next(); });

    m_panel->onHoverEnter = [this]() { m_autoTimer->stop(); };
    m_panel->onHoverLeave = [this]() {
        if (m_slides.size() > 1)
            m_autoTimer->start();
    };
    m_panel->onClicked = [this]() {
        if (m_active >= 0 && m_active < m_slides.size())
            emit openDetailRequested(m_slides[m_active].meta);
    };
    m_panel->canDrag = [this]() { return m_slides.size() > 1; };
    m_panel->onSwipe = [this](int dir) {
        const int n = m_slides.size();
        if (n < 2)
            return;
        goTo(((m_active + dir) % n + n) % n); // wrap, matching auto-advance
    };
    connect(m_panel->playBtn(), &QPushButton::clicked, this, [this]() {
        if (m_active >= 0 && m_active < m_slides.size())
            emit openDetailRequested(m_slides[m_active].meta);
    });
    connect(m_panel->addBtn(), &QPushButton::clicked, this, [this]() {
        QPushButton* b = m_panel->addBtn();
        const bool now = !b->property("inwatch").toBool();
        b->setProperty("inwatch", now);
        b->setText(now ? QStringLiteral("✓  In Watchlist")
                       : QStringLiteral("＋  Add to Watchlist"));
        b->style()->unpolish(b);
        b->style()->polish(b);
    });
}

void FeaturedHero::setSlides(const QVector<HeroSlide>& slides)
{
    m_slides = slides;
    m_active = 0;
    rebuildDots();
    if (m_slides.isEmpty()) {
        m_autoTimer->stop();
        return;
    }
    showSlide(0, false);
    // Grab cursor invites the drag when there's more than one slide (Harbor cursor-grab).
    m_panel->setCursor(m_slides.size() > 1 ? Qt::OpenHandCursor : Qt::PointingHandCursor);
    if (m_slides.size() > 1)
        m_autoTimer->start();
    else
        m_autoTimer->stop();
}

void FeaturedHero::showSlide(int index, bool animate)
{
    if (index < 0 || index >= m_slides.size())
        return;
    m_active = index;
    const HeroSlide& s = m_slides[index];
    m_panel->applySlide(s, upgradeBackground(s.meta.background), animate);
    syncDots();
}

void FeaturedHero::goTo(int index)
{
    if (index == m_active)
        return;
    showSlide(index, true);
    if (m_slides.size() > 1)
        m_autoTimer->start(); // manual nav resets the 13s window
}

void FeaturedHero::next()
{
    if (m_slides.size() < 2)
        return;
    showSlide((m_active + 1) % m_slides.size(), true);
}

void FeaturedHero::rebuildDots()
{
    for (QPushButton* d : m_dots)
        d->deleteLater();
    m_dots.clear();
    if (m_slides.size() < 2)
        return;
    for (int i = 0; i < m_slides.size(); ++i) {
        auto* dot = new QPushButton(m_dotsRow);
        dot->setObjectName(QStringLiteral("HeroDot"));
        dot->setCursor(Qt::PointingHandCursor);
        dot->setFixedHeight(6);
        connect(dot, &QPushButton::clicked, this, [this, i]() { goTo(i); });
        m_dotsLayout->insertWidget(m_dotsLayout->count() - 1, dot); // before trailing stretch
        m_dots.push_back(dot);
    }
    syncDots();
}

void FeaturedHero::syncDots()
{
    for (int i = 0; i < m_dots.size(); ++i) {
        QPushButton* d = m_dots[i];
        const bool active = (i == m_active);
        d->setFixedWidth(active ? 48 : 24); // Harbor: active w-12, rest w-6
        d->setProperty("active", active);
        d->style()->unpolish(d);
        d->style()->polish(d);
    }
}

} // namespace tankoban

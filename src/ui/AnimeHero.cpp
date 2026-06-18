// Tankoban 3 — AnimeHero. See AnimeHero.h. (Adapted from CinemaHero for the Anime route.)

#include "ui/AnimeHero.h"

#include "ui/Icons.h"

#include <QColor>
#include <QSize>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QFont>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QHideEvent>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPaintEvent>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStandardPaths>
#include <QStyle>
#include <QSvgRenderer>
#include <QTimer>
#include <QUrl>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace tankoban {

namespace {
const QColor kCanvas(0x12, 0x13, 0x17);

QString upgradeBackground(QString url)
{
    url.replace(QStringLiteral("/background/small/"), QStringLiteral("/background/large/"));
    url.replace(QStringLiteral("/background/medium/"), QStringLiteral("/background/large/"));
    return url;
}

QNetworkAccessManager* animeNam()
{
    static QNetworkAccessManager* nam = [] {
        auto* m = new QNetworkAccessManager();
        auto* cache = new QNetworkDiskCache(m);
        cache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                 + QStringLiteral("/anime-hero-cache"));
        cache->setMaximumCacheSize(qint64(128) * 1024 * 1024);
        m->setCache(cache);
        return m;
    }();
    return nam;
}

QPushButton* makeArrow(QWidget* parent, const QString& glyph)
{
    auto* b = new QPushButton(glyph, parent);
    b->setObjectName(QStringLiteral("AnimeArrow"));
    b->setCursor(Qt::PointingHandCursor);
    b->setFixedSize(48, 48);
    b->setStyleSheet(QStringLiteral(
        "#AnimeArrow{border:1px solid rgba(255,255,255,0.18);border-radius:24px;"
        "background:rgba(18,19,23,0.7);color:#f3f1ea;font-size:18px;}"
        "#AnimeArrow:hover{background:rgba(45,51,63,0.95);}"));
    return b;
}
} // namespace

AnimeHero::AnimeHero(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("AnimeHero"));
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::ArrowCursor); // no drag/bare-click (Harbor); buttons/arrows set their own

    m_nam = animeNam();

    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("AnimeContent"));
    m_content->setAttribute(Qt::WA_TranslucentBackground, true);
    m_contentFx = new QGraphicsOpacityEffect(m_content);
    m_content->setGraphicsEffect(m_contentFx);

    auto* col = new QVBoxLayout(m_content);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(18);
    col->addStretch();

    m_title = new QLabel(m_content);
    m_title->setObjectName(QStringLiteral("AnimeTitle"));
    m_title->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_title->setWordWrap(true);
    m_title->setStyleSheet(QStringLiteral(
        "#AnimeTitle{font-family:'Fraunces','Iowan Old Style','Georgia',serif;font-size:52px;"
        "font-weight:500;color:#f3f1ea;}"));
    col->addWidget(m_title);

    m_meta = new QLabel(m_content);
    m_meta->setObjectName(QStringLiteral("AnimeMeta"));
    m_meta->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_meta->setStyleSheet(QStringLiteral("#AnimeMeta{font-size:13px;color:#aab1bd;}"));
    col->addWidget(m_meta);

    m_desc = new QLabel(m_content);
    m_desc->setObjectName(QStringLiteral("AnimeDesc"));
    m_desc->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_desc->setWordWrap(true);
    m_desc->setMaximumWidth(520);
    m_desc->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_desc->setStyleSheet(QStringLiteral("#AnimeDesc{color:#aab1bd;}"));
    {
        QFont df = m_desc->font(); // size via QFont so lineSpacing is exact for the clamp
        df.setPixelSize(15);
        m_desc->setFont(df);
        // Harbor line-clamp-3: a true 3-line visual clamp (not a character trim).
        m_desc->setMaximumHeight(m_desc->fontMetrics().lineSpacing() * 3);
    }
    col->addWidget(m_desc);

    auto* btns = new QWidget(m_content);
    btns->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* brow = new QHBoxLayout(btns);
    brow->setContentsMargins(0, 6, 0, 0);
    brow->setSpacing(12);
    // Harbor anime hero (anime-hero.tsx:188-217): solid-accent uppercase Start Watching with
    // a filled Play icon, plus a compact bordered bookmark.
    m_start = new QPushButton(QStringLiteral("START WATCHING"), btns);
    m_start->setObjectName(QStringLiteral("AnimeStartBtn"));
    m_start->setCursor(Qt::PointingHandCursor);
    m_start->setFixedHeight(44);
    m_start->setIcon(navIcon(QStringLiteral("play"), QColor(QStringLiteral("#121317")), 17, true));
    m_start->setIconSize(QSize(17, 17));
    {
        QFont f = m_start->font(); // QSS has no letter-spacing; set Harbor's tracking on the font
        f.setPixelSize(13);
        f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.0); // ~tracking-[0.08em] @ 13px
        m_start->setFont(f);
    }
    m_start->setStyleSheet(QStringLiteral(
        "#AnimeStartBtn{border:none;border-radius:6px;padding:0 20px;color:#121317;"
        "background:#e8b923;}"                        // Harbor bg-accent (gold) / text-canvas
        "#AnimeStartBtn:hover{background:#f0c63a;}")); // ~bg-accent/90

    m_save = new QPushButton(btns);
    m_save->setObjectName(QStringLiteral("AnimeSaveBtn"));
    m_save->setFixedSize(44, 44);
    m_save->setIcon(navIcon(QStringLiteral("bookmark"), QColor(QStringLiteral("#f3f1ea")), 18));
    m_save->setIconSize(QSize(18, 18));
    // V1A: keep the bookmark visible (Harbor's two-button row) but disabled — no Library
    // backend to persist to yet, so no fake "saved" toggle (Hemanth 2026-06-18). Re-wire then.
    m_save->setEnabled(false);
    m_save->setStyleSheet(QStringLiteral(
        "#AnimeSaveBtn{border:1px solid rgba(255,255,255,0.12);border-radius:6px;"
        "background:rgba(26,29,36,0.45);}"          // Harbor border-edge / bg-elevated/45
        "#AnimeSaveBtn:disabled{color:#f3f1ea;}"));  // legible, not greyed

    // MAL score chip (anime-hero.tsx:209-216): MyAnimeList logo + score, per-slide.
    m_malChip = new QWidget(btns);
    m_malChip->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* malRow = new QHBoxLayout(m_malChip);
    malRow->setContentsMargins(4, 0, 0, 0); // Harbor ms-1
    malRow->setSpacing(6);
    auto* malLogo = new QLabel(m_malChip);
    malLogo->setPixmap(malLogoPixmap(QColor(QStringLiteral("#aab1bd")), 12)); // h-[12px] ink-muted
    malRow->addWidget(malLogo);
    m_malScore = new QLabel(m_malChip);
    m_malScore->setStyleSheet(QStringLiteral("color:#f3f1ea;font-size:13px;font-weight:600;"));
    malRow->addWidget(m_malScore);

    brow->addWidget(m_start);
    brow->addWidget(m_save);
    brow->addWidget(m_malChip);
    brow->addStretch();
    col->addWidget(btns);

    connect(m_start, &QPushButton::clicked, this, [this]() {
        const int i = activeIndex();
        if (i >= 0)
            emit openDetailRequested(m_slides[i]); // Harbor AnimeHero Start Watching -> detail
    });
    // Save is intentionally inert in V1A (disabled above) — no fake-persistence toggle.

    m_prevArrow = makeArrow(this, QStringLiteral("‹"));
    m_nextArrow = makeArrow(this, QStringLiteral("›"));
    connect(m_prevArrow, &QPushButton::clicked, this, [this]() { goTo(m_active - 1); });
    connect(m_nextArrow, &QPushButton::clicked, this, [this]() { goTo(m_active + 1); });

    m_dotsRow = new QWidget(this);
    m_dotsRow->setAttribute(Qt::WA_TranslucentBackground, true);
    // Harbor dots (anime-hero.tsx:246-257): active = gold + wide, inactive = ink-subtle/35.
    // Own #AnimeDot rule so Home's shared #HeroDot is untouched.
    m_dotsRow->setStyleSheet(QStringLiteral(
        "QPushButton#AnimeDot{border:none;border-radius:3px;background:rgba(139,144,155,0.35);}"
        "QPushButton#AnimeDot[active=\"true\"]{background:#e8b923;}"));
    m_dotsLayout = new QHBoxLayout(m_dotsRow);
    m_dotsLayout->setContentsMargins(0, 0, 0, 0);
    m_dotsLayout->setSpacing(6); // Harbor gap-1.5

    m_autoTimer = new QTimer(this);
    m_autoTimer->setInterval(14000); // Harbor anime-hero ROTATE_MS
    connect(m_autoTimer, &QTimer::timeout, this, [this]() { next(); });
}

int AnimeHero::activeIndex() const
{
    if (m_slides.isEmpty())
        return -1;
    if (m_active < 0 || m_active >= m_slides.size())
        return 0;
    return m_active;
}

void AnimeHero::setSlides(const QVector<MetaItem>& slides)
{
    m_slides = slides;
    m_active = 0;
    rebuildDots();
    if (m_slides.isEmpty()) {
        m_autoTimer->stop();
        return;
    }
    showSlide(0, false);
    if (m_slides.size() > 1)
        m_autoTimer->start();
    else
        m_autoTimer->stop();
}

void AnimeHero::showSlide(int i, bool animate)
{
    if (i < 0 || i >= m_slides.size())
        return;
    m_active = i;
    const MetaItem& m = m_slides[i];

    m_title->setText(m.name);

    QStringList parts;
    if (!m.releaseInfo.isEmpty())
        parts << m.releaseInfo;
    parts << QStringLiteral("Subtitled"); // Harbor anime-hero hardcoded tag
    if (!m.genres.isEmpty()) {
        QStringList g;
        for (int k = 0; k < m.genres.size() && k < 3; ++k)
            g << m.genres.at(k);
        parts << g.join(QStringLiteral(" · "));
    }
    m_meta->setText(parts.join(QStringLiteral("    ·    ")));

    m_desc->setText(m.description); // 3-line visual clamp via maximumHeight (set in ctor)
    m_desc->setVisible(!m.description.isEmpty());

    if (!m.imdbRating.isEmpty()) { // MAL score chip, shown only when the slide has a score
        m_malScore->setText(m.imdbRating);
        m_malChip->show();
    } else {
        m_malChip->hide();
    }

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

    setBackdrop(upgradeBackground(m.background), animate);
    syncDots();
    positionChildren();
}

void AnimeHero::setBackdrop(const QString& url, bool animate)
{
    if (url == m_url)
        return;
    m_url = url;
    if (url.isEmpty())
        return;

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    QNetworkReply* reply = m_nam->get(req);
    QPointer<AnimeHero> self(this);
    connect(reply, &QNetworkReply::finished, reply, [self, reply, url, animate]() {
        reply->deleteLater();
        if (!self)
            return;
        if (reply->error() != QNetworkReply::NoError || self->m_url != url)
            return;
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
            a->setEasingCurve(QEasingCurve::OutCubic);
            connect(a, &QVariantAnimation::valueChanged, self, [self](const QVariant& v) {
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

QPixmap AnimeHero::coverScaled(const QImage& src) const
{
    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0 || src.isNull())
        return {};
    const qreal dpr = devicePixelRatioF();
    const QSize phys(int(w * dpr), int(h * dpr));
    QImage scaled = src.scaled(phys, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    // Harbor object-position "75% center": bias the horizontal crop to 75% (anime art is
    // poster-like and often weighted right); vertical stays centered.
    const int x = qMax(0, int((scaled.width() - phys.width()) * 0.75));
    const int y = qMax(0, (scaled.height() - phys.height()) / 2);
    scaled = scaled.copy(x, y, qMin(phys.width(), scaled.width()), qMin(phys.height(), scaled.height()));
    QPixmap pm = QPixmap::fromImage(scaled);
    pm.setDevicePixelRatio(dpr);
    return pm;
}

void AnimeHero::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p.fillRect(rect(), kCanvas);

    if (!m_pxPrev.isNull() && m_fade < 1.0) {
        p.setOpacity(1.0 - m_fade);
        p.drawPixmap(0, 0, m_pxPrev);
    }
    if (!m_pxCur.isNull()) {
        p.setOpacity(m_fade);
        p.drawPixmap(0, 0, m_pxCur);
    }
    p.setOpacity(1.0);

    QLinearGradient bg(0, height(), 0, 0);
    bg.setColorAt(0.0, kCanvas);
    bg.setColorAt(0.30, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 178));
    bg.setColorAt(0.66, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 0));
    p.fillRect(rect(), bg);

    const int lw = int(width() * 0.6);
    QLinearGradient lg(0, 0, lw, 0);
    lg.setColorAt(0.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 242));
    lg.setColorAt(0.5, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 140));
    lg.setColorAt(1.0, QColor(kCanvas.red(), kCanvas.green(), kCanvas.blue(), 0));
    p.fillRect(QRect(0, 0, lw, height()), lg);
}

void AnimeHero::positionChildren()
{
    const int leftPad = 48; // Harbor px-12
    const int bottomPad = 40;
    const int cw = qMin(520, qMax(200, width() - leftPad - 48));
    m_content->setGeometry(leftPad, 0, cw, qMax(0, height() - bottomPad));
    m_content->raise();

    const int ay = height() / 2 - 24;
    m_prevArrow->move(16, ay);
    m_nextArrow->move(width() - 48 - 16, ay);
    m_prevArrow->setVisible(m_slides.size() > 1);
    m_nextArrow->setVisible(m_slides.size() > 1);
    m_prevArrow->raise();
    m_nextArrow->raise();

    if (m_dots.size() > 1) {
        m_dotsRow->adjustSize();
        int w = m_dotsRow->width();
        if (w <= 0)
            w = m_dots.size() * 18;
        m_dotsRow->setGeometry((width() - w) / 2, height() - 18, w, 8);
        m_dotsRow->show();
        m_dotsRow->raise();
    } else {
        m_dotsRow->hide();
    }
}

void AnimeHero::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (!m_rawCur.isNull())
        m_pxCur = coverScaled(m_rawCur);
    if (!m_rawPrev.isNull())
        m_pxPrev = coverScaled(m_rawPrev);
    positionChildren();
}

void AnimeHero::goTo(int i)
{
    if (m_slides.isEmpty())
        return;
    const int n = m_slides.size();
    i = ((i % n) + n) % n;
    if (i == m_active)
        return;
    showSlide(i, true);
    if (n > 1)
        m_autoTimer->start();
}

void AnimeHero::next()
{
    if (m_slides.size() < 2)
        return;
    goTo(m_active + 1);
}

void AnimeHero::rebuildDots()
{
    for (QPushButton* d : m_dots)
        d->deleteLater();
    m_dots.clear();
    if (m_slides.size() < 2)
        return;
    for (int i = 0; i < m_slides.size(); ++i) {
        auto* dot = new QPushButton(m_dotsRow);
        dot->setObjectName(QStringLiteral("AnimeDot"));
        dot->setCursor(Qt::PointingHandCursor);
        dot->setFixedHeight(6);
        connect(dot, &QPushButton::clicked, this, [this, i]() { goTo(i); });
        m_dotsLayout->addWidget(dot);
        m_dots.push_back(dot);
    }
    syncDots();
}

void AnimeHero::syncDots()
{
    for (int i = 0; i < m_dots.size(); ++i) {
        QPushButton* d = m_dots[i];
        const bool active = (i == m_active);
        d->setFixedWidth(active ? 40 : 24); // Harbor active w-10 / inactive w-6
        d->setProperty("active", active);
        d->style()->unpolish(d);
        d->style()->polish(d);
    }
}

void AnimeHero::enterEvent(QEnterEvent*)
{
    m_autoTimer->stop(); // pause rotation on hover (Harbor onMouseEnter)
}

void AnimeHero::leaveEvent(QEvent*)
{
    if (m_slides.size() > 1)
        m_autoTimer->start();
}

void AnimeHero::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    if (m_slides.size() > 1) // resume when the Anime route becomes visible again
        m_autoTimer->start();
}

void AnimeHero::hideEvent(QHideEvent* e)
{
    QWidget::hideEvent(e);
    m_autoTimer->stop(); // pause when the route is hidden (Harbor usePageVisible / inView)
}

} // namespace tankoban

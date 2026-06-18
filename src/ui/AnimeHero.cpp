// Tankoban 3 — AnimeHero. See AnimeHero.h. (Adapted from CinemaHero for the Anime route.)

#include "ui/AnimeHero.h"

#include "ui/Icons.h"

#include <QColor>
#include <QSize>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
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
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QUrl>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace tankoban {

namespace {
const QColor kCanvas(0x12, 0x13, 0x17);
constexpr int kDragBudge = 6;
constexpr qreal kSnapRatio = 0.18;
constexpr qreal kFlickVel = 0.45;

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
    setCursor(Qt::PointingHandCursor);

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
    m_desc->setStyleSheet(QStringLiteral("#AnimeDesc{font-size:15px;color:#aab1bd;}"));
    col->addWidget(m_desc);

    auto* btns = new QWidget(m_content);
    btns->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* brow = new QHBoxLayout(btns);
    brow->setContentsMargins(0, 6, 0, 0);
    brow->setSpacing(12);
    // Harbor anime hero: a warm-orange filled Start Watching + a compact bordered bookmark.
    m_start = new QPushButton(QStringLiteral("▶  Start Watching"), btns);
    m_start->setObjectName(QStringLiteral("AnimeStartBtn"));
    m_start->setCursor(Qt::PointingHandCursor);
    m_start->setFixedHeight(48);
    m_start->setStyleSheet(QStringLiteral(
        "#AnimeStartBtn{border:none;border-radius:8px;padding:0 26px;font-size:15px;"
        "font-weight:700;color:#1a1206;"
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #efab6b,stop:1 #e2893c);}"
        "#AnimeStartBtn:hover{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "stop:0 #f4b67b,stop:1 #e8934a);}"));

    m_save = new QPushButton(btns);
    m_save->setObjectName(QStringLiteral("AnimeSaveBtn"));
    m_save->setCursor(Qt::PointingHandCursor);
    m_save->setFixedSize(48, 48);
    m_save->setIcon(navIcon(QStringLiteral("bookmark"), QColor(QStringLiteral("#f3f1ea")), 18));
    m_save->setIconSize(QSize(18, 18));
    m_save->setStyleSheet(QStringLiteral(
        "#AnimeSaveBtn{border:1px solid rgba(255,255,255,0.22);border-radius:8px;"
        "background:rgba(18,19,23,0.55);}"
        "#AnimeSaveBtn:hover{background:rgba(45,51,63,0.85);}"
        "#AnimeSaveBtn[saved=\"true\"]{border-color:#e8b923;background:rgba(232,185,35,0.16);}"));

    brow->addWidget(m_start);
    brow->addWidget(m_save);
    brow->addStretch();
    col->addWidget(btns);

    connect(m_start, &QPushButton::clicked, this, [this]() {
        const int i = activeIndex();
        if (i >= 0)
            emit openDetailRequested(m_slides[i]); // Harbor AnimeHero Start Watching -> detail
    });
    connect(m_save, &QPushButton::clicked, this, [this]() {
        const bool now = !m_save->property("saved").toBool();
        m_save->setProperty("saved", now);
        m_save->setIcon(navIcon(QStringLiteral("bookmark"),
                                QColor(now ? QStringLiteral("#e8b923") : QStringLiteral("#f3f1ea")), 18));
        m_save->style()->unpolish(m_save);
        m_save->style()->polish(m_save);
    });

    m_prevArrow = makeArrow(this, QStringLiteral("‹"));
    m_nextArrow = makeArrow(this, QStringLiteral("›"));
    connect(m_prevArrow, &QPushButton::clicked, this, [this]() { goTo(m_active - 1); });
    connect(m_nextArrow, &QPushButton::clicked, this, [this]() { goTo(m_active + 1); });

    m_dotsRow = new QWidget(this);
    m_dotsRow->setAttribute(Qt::WA_TranslucentBackground, true);
    m_dotsLayout = new QHBoxLayout(m_dotsRow);
    m_dotsLayout->setContentsMargins(0, 0, 0, 0);
    m_dotsLayout->setSpacing(10);

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
    setCursor(m_slides.size() > 1 ? Qt::OpenHandCursor : Qt::PointingHandCursor);
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

    QString desc = m.description;
    if (desc.size() > 260)
        desc = desc.left(259).trimmed() + QStringLiteral("…");
    m_desc->setText(desc);
    m_desc->setVisible(!desc.isEmpty());

    m_save->setProperty("saved", false);
    m_save->setIcon(navIcon(QStringLiteral("bookmark"), QColor(QStringLiteral("#f3f1ea")), 18));
    m_save->style()->unpolish(m_save);
    m_save->style()->polish(m_save);

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
    const int x = qMax(0, (scaled.width() - phys.width()) / 2);
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
        dot->setObjectName(QStringLiteral("HeroDot"));
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
        d->setFixedWidth(active ? 40 : 6);
        d->setProperty("active", active);
        d->style()->unpolish(d);
        d->style()->polish(d);
    }
}

void AnimeHero::mousePressEvent(QMouseEvent* e)
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
    m_contentBaseX = m_content ? m_content->x() : 0;
}

void AnimeHero::mouseMoveEvent(QMouseEvent* e)
{
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

    if (!m_dragMoved && qAbs(dx) > kDragBudge && m_slides.size() > 1) {
        m_dragMoved = true;
        setCursor(Qt::ClosedHandCursor);
    }
    if (m_dragMoved && m_content)
        m_content->move(m_contentBaseX + dx, m_content->y());
}

void AnimeHero::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_dragActive)
        return;
    m_dragActive = false;

    if (m_dragMoved) {
        const qreal threshold = qreal(qMax(1, width())) * kSnapRatio;
        const qreal dist = qreal(m_lastX - m_startX);
        int dir = 0;
        if (dist < -threshold || m_vel < -kFlickVel)
            dir = +1;
        else if (dist > threshold || m_vel > kFlickVel)
            dir = -1;
        if (m_content)
            m_content->move(m_contentBaseX, m_content->y());
        setCursor(m_slides.size() > 1 ? Qt::OpenHandCursor : Qt::PointingHandCursor);
        m_dragMoved = false;
        if (dir != 0)
            goTo(m_active + dir);
    } else if (e->button() == Qt::LeftButton) {
        const int i = activeIndex();
        if (i >= 0)
            emit openDetailRequested(m_slides[i]);
    }
}

void AnimeHero::enterEvent(QEnterEvent*)
{
    m_autoTimer->stop();
}

void AnimeHero::leaveEvent(QEvent*)
{
    if (m_slides.size() > 1)
        m_autoTimer->start();
}

} // namespace tankoban

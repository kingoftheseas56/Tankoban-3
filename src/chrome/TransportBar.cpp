#include "chrome/AudioMenu.h"
#include "chrome/SubtitleMenu.h"
#include "chrome/SpeedMenu.h"
#include "chrome/TransportBar.h"
#include "chrome/SeekBar.h"
#include "chrome/TimeLabel.h"
#include "chrome/TitleInfo.h"
#include "chrome/VolumeControl.h"
#include "util/Theme.h"

#include <QAbstractButton>
#include <QEnterEvent>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QTimer>

#include <algorithm>

namespace {

enum class Glyph { Back, Play, Pause, SeekBack, SeekForward, Maximize, Minimize };

QPointF lucidePoint(const QRectF& rect, qreal x, qreal y)
{
    const qreal scale = std::min(rect.width(), rect.height()) / 24.0;
    const qreal ox = rect.x() + (rect.width() - 24.0 * scale) * 0.5;
    const qreal oy = rect.y() + (rect.height() - 24.0 * scale) * 0.5;
    return QPointF(ox + x * scale, oy + y * scale);
}

void drawLucideSegment(QPainter& p, const QRectF& rect, qreal x1, qreal y1, qreal x2, qreal y2)
{
    p.drawLine(lucidePoint(rect, x1, y1), lucidePoint(rect, x2, y2));
}

void drawFullscreenGlyph(QPainter& p, Glyph glyph, const QRectF& rect)
{
    if (glyph == Glyph::Maximize) {
        drawLucideSegment(p, rect, 8, 3, 5, 3);
        drawLucideSegment(p, rect, 5, 3, 3, 5);
        drawLucideSegment(p, rect, 3, 5, 3, 8);

        drawLucideSegment(p, rect, 16, 3, 19, 3);
        drawLucideSegment(p, rect, 19, 3, 21, 5);
        drawLucideSegment(p, rect, 21, 5, 21, 8);

        drawLucideSegment(p, rect, 3, 16, 3, 19);
        drawLucideSegment(p, rect, 3, 19, 5, 21);
        drawLucideSegment(p, rect, 5, 21, 8, 21);

        drawLucideSegment(p, rect, 16, 21, 19, 21);
        drawLucideSegment(p, rect, 19, 21, 21, 19);
        drawLucideSegment(p, rect, 21, 19, 21, 16);
        return;
    }

    drawLucideSegment(p, rect, 8, 3, 8, 6);
    drawLucideSegment(p, rect, 8, 6, 6, 8);
    drawLucideSegment(p, rect, 6, 8, 3, 8);

    drawLucideSegment(p, rect, 16, 3, 16, 6);
    drawLucideSegment(p, rect, 16, 6, 18, 8);
    drawLucideSegment(p, rect, 18, 8, 21, 8);

    drawLucideSegment(p, rect, 3, 16, 6, 16);
    drawLucideSegment(p, rect, 6, 16, 8, 18);
    drawLucideSegment(p, rect, 8, 18, 8, 21);

    drawLucideSegment(p, rect, 16, 21, 16, 18);
    drawLucideSegment(p, rect, 16, 18, 18, 16);
    drawLucideSegment(p, rect, 18, 16, 21, 16);
}

class ChromeButton final : public QAbstractButton {
public:
    ChromeButton(Glyph glyph, int size, QWidget* parent = nullptr)
        : QAbstractButton(parent), m_glyph(glyph), m_size(size)
    {
        setFixedSize(size, size);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
    }

    void setGlyph(Glyph glyph) { m_glyph = glyph; update(); }
    void setHero(bool hero) { m_hero = hero; update(); }
    void setStepSeconds(int sec) { m_stepSeconds = sec; update(); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QColor bg = Qt::transparent;
        QColor fg = theme::white(isEnabled() ? 0.88 : 0.30);
        if (m_hero) bg = theme::white(isDown() ? 0.22 : (underMouse() ? 0.22 : 0.12));
        else if (underMouse() && isEnabled()) bg = theme::white(0.10);
        if (isChecked()) bg = theme::white(0.22);

        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawEllipse(rect().adjusted(0, 0, -1, -1));
        p.setPen(QPen(fg, m_glyph == Glyph::Back ? 2.2 : 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);

        const QPointF c = rect().center();
        if (m_glyph == Glyph::Back) {
            QPainterPath path;
            path.moveTo(c.x() + 5, c.y() - 10);
            path.lineTo(c.x() - 5, c.y());
            path.lineTo(c.x() + 5, c.y() + 10);
            p.drawPath(path);
        } else if (m_glyph == Glyph::Play) {
            p.setPen(Qt::NoPen);
            p.setBrush(fg);
            QPainterPath path;
            path.moveTo(c.x() - 5, c.y() - 11);
            path.lineTo(c.x() - 5, c.y() + 11);
            path.lineTo(c.x() + 13, c.y());
            path.closeSubpath();
            p.drawPath(path);
        } else if (m_glyph == Glyph::Pause) {
            p.setPen(Qt::NoPen);
            p.setBrush(fg);
            p.drawRoundedRect(QRectF(c.x() - 10, c.y() - 12, 6, 24), 2, 2);
            p.drawRoundedRect(QRectF(c.x() + 4, c.y() - 12, 6, 24), 2, 2);
        } else if (m_glyph == Glyph::Maximize || m_glyph == Glyph::Minimize) {
            p.setPen(QPen(fg, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            drawFullscreenGlyph(p, m_glyph, QRectF(c.x() - 11, c.y() - 11, 22, 22));
        } else {
            const bool forward = m_glyph == Glyph::SeekForward;
            const QRectF arc(c.x() - 15, c.y() - 15, 30, 30);
            p.drawArc(arc, forward ? 35 * 16 : 145 * 16, forward ? 270 * 16 : -270 * 16);
            QPainterPath arrow;
            if (forward) {
                arrow.moveTo(c.x() + 13, c.y() - 13);
                arrow.lineTo(c.x() + 20, c.y() - 11);
                arrow.lineTo(c.x() + 15, c.y() - 5);
            } else {
                arrow.moveTo(c.x() - 13, c.y() - 13);
                arrow.lineTo(c.x() - 20, c.y() - 11);
                arrow.lineTo(c.x() - 15, c.y() - 5);
            }
            p.drawPath(arrow);
            QFont f(QStringLiteral("Consolas"));
            f.setPointSizeF(8.0);
            f.setBold(true);
            p.setFont(f);
            p.setPen(fg);
            p.drawText(rect(), Qt::AlignCenter, QString::number(m_stepSeconds));
        }
    }

private:
    Glyph m_glyph;
    int m_size = 48;
    int m_stepSeconds = 10;
    bool m_hero = false;
};

} // namespace

TransportBar::TransportBar(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(1.0);
    setGraphicsEffect(m_opacity);
    m_fade = new QPropertyAnimation(m_opacity, "opacity", this);
    m_fade->setDuration(300);
    m_fade->setEasingCurve(QEasingCurve::OutCubic);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this] {
        if (m_playing && !m_menuOpen && !m_seekHovering) setChromeVisible(false);
    });

    m_back = new ChromeButton(Glyph::Back, 48, this);
    m_titleInfo = new TitleInfo(this);
    m_start = new TimeLabel(TimeLabel::Mode::Start, this);
    m_end = new TimeLabel(TimeLabel::Mode::End, this);
    m_seek = new SeekBar(this);
    m_volume = new VolumeControl(this);
    m_playPause = new ChromeButton(Glyph::Play, 64, this);
    static_cast<ChromeButton*>(m_playPause)->setHero(true);
    m_seekBack = new ChromeButton(Glyph::SeekBack, 56, this);
    m_seekForward = new ChromeButton(Glyph::SeekForward, 56, this);
    m_audioMenu = new AudioMenu(this);
    m_subtitleMenu = new SubtitleMenu(this);
    m_speedMenu = new SpeedMenu(this);
    m_fullscreen = new ChromeButton(Glyph::Maximize, 48, this);

    connect(m_back, &QAbstractButton::clicked, this, &TransportBar::backRequested);
    connect(m_titleInfo, &TitleInfo::clicked, this, [this] { wakeChrome(); });
    connect(m_playPause, &QAbstractButton::clicked, this, &TransportBar::playPauseRequested);
    connect(m_seekBack, &QAbstractButton::clicked, this, [this] { emit seekStepRequested(-10); });
    connect(m_seekForward, &QAbstractButton::clicked, this, [this] { emit seekStepRequested(10); });
    connect(m_volume, &VolumeControl::volumeRequested, this, &TransportBar::volumeRequested);
    connect(m_volume, &VolumeControl::muteRequested, this, &TransportBar::muteRequested);
    connect(m_audioMenu, &AudioMenu::trackRequested, this, &TransportBar::audioTrackRequested);
    connect(m_audioMenu, &AudioMenu::delayRequested, this, &TransportBar::audioDelayRequested);
    connect(m_audioMenu, &AudioMenu::openChanged, this, &TransportBar::setMenuOpen);
    connect(m_subtitleMenu, &SubtitleMenu::trackRequested, this, &TransportBar::subtitleTrackRequested);
    connect(m_subtitleMenu, &SubtitleMenu::delayRequested, this, &TransportBar::subtitleDelayRequested);
    connect(m_subtitleMenu, &SubtitleMenu::openChanged, this, &TransportBar::setMenuOpen);
    connect(m_speedMenu, &SpeedMenu::rateRequested, this, &TransportBar::speedRequested);
    connect(m_speedMenu, &SpeedMenu::openChanged, this, &TransportBar::setMenuOpen);
    connect(m_fullscreen, &QAbstractButton::clicked, this, &TransportBar::fullscreenRequested);
    connect(m_seek, &SeekBar::seekRequested, this, &TransportBar::seekRequested);
    connect(m_seek, &SeekBar::hoveringChanged, this, [this](bool hovering) {
        m_seekHovering = hovering;
        if (hovering) wakeChrome();
        else restartHideTimer();
    });

    wakeChrome();
}

void TransportBar::setSnapshot(const PlayerSnapshot& snap)
{
    m_playing = snap.status == PlayerSnapshot::Playing;
    m_seek->setDuration(snap.durationSec);
    m_start->setDuration(snap.durationSec);
    m_end->setDuration(snap.durationSec);
    m_volume->setSnapshot(snap);
    m_audioMenu->setSnapshot(snap);
    m_subtitleMenu->setSnapshot(snap);
    m_speedMenu->setSnapshot(snap);
    layoutChrome();   // speed pill width tracks rate -> re-place the right cluster
    static_cast<ChromeButton*>(m_playPause)->setGlyph(m_playing ? Glyph::Pause : Glyph::Play);
    restartHideTimer();
}

void TransportBar::setFullscreen(bool fullscreen)
{
    m_fullscreenOn = fullscreen;
    static_cast<ChromeButton*>(m_fullscreen)->setGlyph(fullscreen ? Glyph::Minimize : Glyph::Maximize);
}

void TransportBar::setTitleInfo(const QString& title, const QString& subtitle, bool clickable)
{
    m_titleInfo->setText(title, subtitle);
    m_titleInfo->setClickable(clickable);
    layoutChrome();
}

void TransportBar::wakeChrome()
{
    setChromeVisible(true);
    restartHideTimer();
}

void TransportBar::setMenuOpen(bool open)
{
    m_menuOpen = open;
    if (open) wakeChrome();
    else restartHideTimer();
}

void TransportBar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient top(0, 0, 0, 116);
    top.setColorAt(0.0, theme::black(0.55));
    top.setColorAt(0.55, theme::black(0.15));
    top.setColorAt(1.0, theme::black(0.0));
    p.fillRect(QRect(0, 0, width(), 128), top);

    QLinearGradient bottom(0, height(), 0, height() - 190);
    bottom.setColorAt(0.0, theme::black(0.70));
    bottom.setColorAt(0.58, theme::black(0.25));
    bottom.setColorAt(1.0, theme::black(0.0));
    p.fillRect(QRect(0, std::max(0, height() - 210), width(), 210), bottom);
}

void TransportBar::resizeEvent(QResizeEvent*)
{
    layoutChrome();
}

void TransportBar::mouseMoveEvent(QMouseEvent* e)
{
    wakeChrome();
    QWidget::mouseMoveEvent(e);
}

void TransportBar::enterEvent(QEnterEvent* e)
{
    wakeChrome();
    QWidget::enterEvent(e);
}

void TransportBar::layoutChrome()
{
    const int w = width();
    const int h = height();
    const int padX = 28;
    m_back->setGeometry(padX, 16, 48, 48);
    const QSize titleSize = m_titleInfo->sizeHint().boundedTo(QSize(std::max(0, w - 160), 80));
    m_titleInfo->setGeometry(w - padX - titleSize.width(), 16, titleSize.width(), std::max(48, titleSize.height()));

    const int bottomPad = 20;
    const int seekY = h - bottomPad - 64 - 10 - 48;
    const int ctrlY = h - bottomPad - 64;
    const int timeW = 58;
    const int gap = 12;
    const int seekX = padX + timeW + gap;
    const int seekW = std::max(1, w - padX * 2 - timeW * 2 - gap * 2);
    m_start->setGeometry(padX, seekY, timeW, 48);
    m_seek->setGeometry(seekX, seekY, seekW, 48);
    m_end->setGeometry(seekX + seekW + gap, seekY, timeW, 48);

    const int centerW = 56 + 6 + 64 + 6 + 56;
    const int cx = (w - centerW) / 2;
    m_volume->setGeometry(padX, ctrlY + 8, 220, 48);
    m_seekBack->setGeometry(cx, ctrlY + 4, 56, 56);
    m_playPause->setGeometry(cx + 62, ctrlY, 64, 64);
    m_seekForward->setGeometry(cx + 132, ctrlY + 4, 56, 56);
    // Bottom-right cluster, Harbor order: audio · subtitle · speed · fullscreen.
    const int fsX = w - padX - 48;
    const int speedW = m_speedMenu->width();   // 48 at 1x, wider pill when rate != 1
    const int speedX = fsX - 10 - speedW;
    const int subX = speedX - 10 - 48;
    const int audX = subX - 10 - 48;
    m_audioMenu->setGeometry(audX, ctrlY + 8, 48, 48);
    m_subtitleMenu->setGeometry(subX, ctrlY + 8, 48, 48);
    m_speedMenu->setGeometry(speedX, ctrlY + 8, speedW, 48);
    m_fullscreen->setGeometry(fsX, ctrlY + 8, 48, 48);
    m_audioMenu->syncPopupPosition();
    m_subtitleMenu->syncPopupPosition();
    m_speedMenu->syncPopupPosition();
}

void TransportBar::restartHideTimer()
{
    m_hideTimer->stop();
    if (m_menuOpen || m_seekHovering) return;
    m_hideTimer->start(m_playing ? 1800 : 4500);
}

void TransportBar::setChromeVisible(bool visible)
{
    m_fade->stop();
    m_fade->setStartValue(m_opacity->opacity());
    m_fade->setEndValue(visible ? 1.0 : 0.0);
    m_fade->start();
    setCursor(visible || !m_playing ? Qt::ArrowCursor : Qt::BlankCursor);
}

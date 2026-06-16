#include "chrome/SeekBar.h"
#include "engine/PlaybackClock.h"
#include "util/Theme.h"
#include "util/TimeFormat.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

SeekBar::SeekBar(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(48);                       // §6.1 hit-area row
    setCursor(Qt::PointingHandCursor);
    connect(&PlaybackClock::instance(), &PlaybackClock::changed, this, &SeekBar::onClock);
}

void SeekBar::setDuration(double sec) { m_dur = sec > 0 ? sec : 1.0; update(); }

double SeekBar::timeAt(int x) const
{
    const double f = std::clamp(double(x) / std::max(1, width()), 0.0, 1.0);
    return f * m_dur;
}

void SeekBar::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) return;
    grabMouse();
    m_pending.reset();
    m_scrub = timeAt(int(e->position().x()));
    m_dragging = true;
    emit hoveringChanged(true);
    update();
}

void SeekBar::mouseMoveEvent(QMouseEvent* e)
{
    m_hover = timeAt(int(e->position().x()));
    if (m_dragging) m_scrub = timeAt(int(e->position().x()));
    emit hoveringChanged(true);
    update();
}

void SeekBar::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) return;
    releaseMouse();
    if (m_scrub) { emit seekRequested(*m_scrub); m_pending = *m_scrub; }   // commit on release
    m_scrub.reset();
    m_dragging = false;
    update();
}

void SeekBar::leaveEvent(QEvent*)
{
    m_hover.reset();
    emit hoveringChanged(false);
    update();
}

void SeekBar::onClock()
{
    const double pos = PlaybackClock::instance().position();
    if (m_pending && std::abs(pos - *m_pending) < 0.75) m_pending.reset();   // §6.1 auto-clear
    update();
}

void SeekBar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double pos   = PlaybackClock::instance().position();
    const double value = m_scrub ? *m_scrub : (m_pending ? *m_pending : pos);
    const bool   active = m_scrub.has_value() || m_hover.has_value();
    const int    trackH = active ? 8 : 6;                       // §6.3 +2 on hover/scrub
    const int    dotD   = m_scrub ? 20 : 16;                    // §6.3 +4 scrubbing
    const double pct    = std::clamp(value / m_dur, 0.0, 1.0);
    const double bufPct = std::clamp((pos + PlaybackClock::instance().buffered()) / m_dur, 0.0, 1.0);

    const int    w  = width();
    const double cy = height() / 2.0;
    const double r  = trackH / 2.0;
    const QRectF track(0, cy - trackH / 2.0, w, trackH);

    p.setPen(Qt::NoPen);
    p.setBrush(theme::white(0.15));                            // 1 bg track
    p.drawRoundedRect(track, r, r);
    if (bufPct > 0) {                                          // 2 buffered
        p.setBrush(theme::white(0.28));
        p.drawRoundedRect(QRectF(0, track.top(), w * bufPct, trackH), r, r);
    }
    p.setBrush(theme::accent());                          // 3 played fill (gold)
    p.drawRoundedRect(QRectF(0, track.top(), w * pct, trackH), r, r);

    const double hx  = w * pct;                               // 5 handle
    const double rad = dotD / 2.0;
    p.setBrush(theme::black(0.45));                           // 4px dark ring
    p.drawEllipse(QPointF(hx, cy), rad + 4, rad + 4);
    p.setBrush(theme::accent());
    p.drawEllipse(QPointF(hx, cy), rad, rad);

    if (m_hover) {                                            // §6.4 hover time bubble (36px above)
        const QString t = fmtTime(*m_hover);
        QFont f = p.font(); f.setFamily(QStringLiteral("Consolas")); f.setPointSizeF(9); f.setBold(true);
        p.setFont(f);
        const QFontMetrics fm(f);
        const int tw = fm.horizontalAdvance(t) + 12, th = fm.height() + 6;
        const double tx = std::clamp(hx - tw / 2.0, 0.0, double(std::max(0, w - tw)));
        const QRectF bub(tx, cy - 36 - th, tw, th);
        p.setBrush(theme::black(0.9));
        p.drawRoundedRect(bub, 4, 4);
        p.setPen(Qt::white);
        p.drawText(bub, Qt::AlignCenter, t);
    }
}

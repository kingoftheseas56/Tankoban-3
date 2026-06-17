#include "chrome/SeekBar.h"
#include "engine/PlaybackClock.h"
#include "util/Theme.h"
#include "util/TimeFormat.h"

#include <QFont>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

SeekBar::SeekBar(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(48);
    setCursor(Qt::PointingHandCursor);
    connect(&PlaybackClock::instance(), &PlaybackClock::changed, this, &SeekBar::onClock);
}

SeekBar::~SeekBar()
{
    delete m_tip;
}

void SeekBar::setDuration(double sec)
{
    m_dur = sec > 0 ? sec : 1.0;
    updateTooltip();
    update();
}

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
    updateTooltip();
    update();
}

void SeekBar::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) return;
    releaseMouse();
    if (m_scrub) {
        emit seekRequested(*m_scrub);
        m_pending = *m_scrub;
    }
    m_scrub.reset();
    m_dragging = false;
    update();
}

void SeekBar::leaveEvent(QEvent*)
{
    m_hover.reset();
    hideTooltip();
    emit hoveringChanged(false);
    update();
}

void SeekBar::onClock()
{
    const double pos = PlaybackClock::instance().position();
    if (m_pending && std::abs(pos - *m_pending) < 0.75) m_pending.reset();
    updateTooltip();
    update();
}

void SeekBar::updateTooltip()
{
    if (!m_hover) {
        hideTooltip();
        return;
    }

    QWidget* host = parentWidget() ? parentWidget() : this;
    if (!m_tip) {
        m_tip = new QLabel(host);
        m_tip->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_tip->setAlignment(Qt::AlignCenter);
        QFont f(QStringLiteral("Consolas"));
        f.setPointSizeF(9.0);
        f.setBold(true);
        m_tip->setFont(f);
        m_tip->setStyleSheet(QStringLiteral(
            "QLabel{color:white;background:rgba(0,0,0,230);"
            "border:1px solid rgba(255,255,255,25);border-radius:6px;"
            "padding:4px 8px;}"));
    } else if (m_tip->parentWidget() != host) {
        m_tip->setParent(host);
    }

    m_tip->setText(fmtTime(*m_hover));
    m_tip->adjustSize();
    const double hoverPct = std::clamp(*m_hover / m_dur, 0.0, 1.0);
    const int hoverX = int(std::round(hoverPct * width()));
    QPoint pos = mapTo(host, QPoint(hoverX - m_tip->width() / 2,
                                    height() / 2 - 36 - m_tip->height()));
    pos.setX(std::clamp(pos.x(), 0, std::max(0, host->width() - m_tip->width())));
    pos.setY(std::max(0, pos.y()));
    m_tip->move(pos);
    m_tip->raise();
    m_tip->show();
}

void SeekBar::hideTooltip()
{
    if (m_tip) m_tip->hide();
}

void SeekBar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double pos = PlaybackClock::instance().position();
    const double value = m_scrub ? *m_scrub : (m_pending ? *m_pending : pos);
    const bool active = m_scrub.has_value() || m_hover.has_value();
    const int trackH = active ? 8 : 6;
    const int dotD = m_scrub ? 20 : 16;
    const double pct = std::clamp(value / m_dur, 0.0, 1.0);
    const double bufPct = std::clamp((pos + PlaybackClock::instance().buffered()) / m_dur, 0.0, 1.0);

    const int w = width();
    const double cy = height() / 2.0;
    const double r = trackH / 2.0;
    const QRectF track(0, cy - trackH / 2.0, w, trackH);

    p.setPen(Qt::NoPen);
    p.setBrush(theme::white(0.15));
    p.drawRoundedRect(track, r, r);

    if (bufPct > 0) {
        p.setBrush(theme::white(0.28));
        p.drawRoundedRect(QRectF(0, track.top(), w * bufPct, trackH), r, r);
    }

    p.setBrush(theme::accent());
    p.drawRoundedRect(QRectF(0, track.top(), w * pct, trackH), r, r);

    const double hx = w * pct;
    const double rad = dotD / 2.0;
    p.setBrush(theme::black(0.45));
    p.drawEllipse(QPointF(hx, cy), rad + 4, rad + 4);
    p.setBrush(theme::accent());
    p.drawEllipse(QPointF(hx, cy), rad, rad);
}

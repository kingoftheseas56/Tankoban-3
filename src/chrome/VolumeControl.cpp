#include "chrome/VolumeControl.h"

#include "engine/PlayerSnapshot.h"
#include "util/Theme.h"
#include "util/VolumeCurve.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {

QPointF lucidePoint(const QRectF& rect, qreal x, qreal y)
{
    const qreal scale = std::min(rect.width(), rect.height()) / 24.0;
    const qreal ox = rect.x() + (rect.width() - 24.0 * scale) * 0.5;
    const qreal oy = rect.y() + (rect.height() - 24.0 * scale) * 0.5;
    return QPointF(ox + x * scale, oy + y * scale);
}

void drawSegment(QPainter& p, const QRectF& rect, qreal x1, qreal y1, qreal x2, qreal y2)
{
    p.drawLine(lucidePoint(rect, x1, y1), lucidePoint(rect, x2, y2));
}

} // namespace

VolumeControl::VolumeControl(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);
    setFixedSize(220, 48);
}

void VolumeControl::setSnapshot(const PlayerSnapshot& snap)
{
    m_volume = std::clamp(snap.volume, 0.0, volume_curve::kMax);
    m_muted = snap.muted;
    update();
}

QRect VolumeControl::muteRect() const
{
    return QRect(0, 0, 48, 48);
}

QRect VolumeControl::trackRect() const
{
    return QRect(56, 20, volume_curve::kTrackWidth, 8);
}

QRect VolumeControl::valueRect() const
{
    return QRect(trackRect().right() + 9, 0, 36, 48);
}

VolumeControl::HitRegion VolumeControl::hitRegion(const QPoint& pos) const
{
    if (muteRect().contains(pos)) return HitRegion::Mute;
    const QRect trackHit = trackRect().adjusted(-4, -8, 4, 8);
    if (trackHit.contains(pos)) return HitRegion::Track;
    return HitRegion::None;
}

void VolumeControl::setFromTrackX(int x)
{
    const QRect rect = trackRect();
    const double fraction = std::clamp(double(x - rect.left()) / std::max(1, rect.width()), 0.0, 1.0);
    const double next = volume_curve::valueFromFraction(fraction);
    const double rounded = std::round(std::min(volume_curve::kMax, next) * 100.0) / 100.0;
    emit volumeRequested(rounded);
}

void VolumeControl::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) return;
    m_pressed = hitRegion(e->position().toPoint());
    if (m_pressed == HitRegion::Track) {
        m_dragging = true;
        grabMouse();
        setFromTrackX(int(std::lround(e->position().x())));
    }
    update();
}

void VolumeControl::mouseMoveEvent(QMouseEvent* e)
{
    m_hover = hitRegion(e->position().toPoint());
    setCursor(m_hover == HitRegion::None ? Qt::ArrowCursor : Qt::PointingHandCursor);
    if (m_dragging) setFromTrackX(int(std::lround(e->position().x())));
    update();
}

void VolumeControl::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) return;
    if (m_dragging) {
        releaseMouse();
        m_dragging = false;
    } else if (m_pressed == HitRegion::Mute && hitRegion(e->position().toPoint()) == HitRegion::Mute) {
        emit muteRequested(!m_muted);
    }
    m_pressed = HitRegion::None;
    update();
}

void VolumeControl::wheelEvent(QWheelEvent* e)
{
    e->accept();
    const double shown = m_muted ? 0.0 : std::clamp(m_volume, 0.0, volume_curve::kMax);
    const double step = (e->modifiers() & Qt::ShiftModifier) ? 0.5 : 0.05;
    const double next = std::clamp(shown + (e->angleDelta().y() > 0 ? step : -step), 0.0, volume_curve::kMax);
    emit volumeRequested(std::round(next * 100.0) / 100.0);
}

void VolumeControl::leaveEvent(QEvent*)
{
    m_hover = HitRegion::None;
    if (!m_dragging) setCursor(Qt::ArrowCursor);
    update();
}

void VolumeControl::drawMuteGlyph(QPainter& p, const QRectF& rect, bool muted, const QColor& color) const
{
    p.setPen(QPen(color, 1.75, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    QPainterPath speaker;
    speaker.moveTo(lucidePoint(rect, 3, 10));
    speaker.lineTo(lucidePoint(rect, 7, 10));
    speaker.lineTo(lucidePoint(rect, 12, 6));
    speaker.lineTo(lucidePoint(rect, 12, 18));
    speaker.lineTo(lucidePoint(rect, 7, 14));
    speaker.lineTo(lucidePoint(rect, 3, 14));
    speaker.closeSubpath();
    p.drawPath(speaker);

    if (muted) {
        drawSegment(p, rect, 16, 9, 21, 14);
        drawSegment(p, rect, 21, 9, 16, 14);
        return;
    }

    QPainterPath outer;
    outer.moveTo(lucidePoint(rect, 16, 8));
    outer.cubicTo(lucidePoint(rect, 18.5, 10), lucidePoint(rect, 18.5, 14), lucidePoint(rect, 16, 16));
    p.drawPath(outer);

    QPainterPath inner;
    inner.moveTo(lucidePoint(rect, 14, 10));
    inner.cubicTo(lucidePoint(rect, 15.5, 11), lucidePoint(rect, 15.5, 13), lucidePoint(rect, 14, 14));
    p.drawPath(inner);
}

void VolumeControl::paintEvent(QPaintEvent*)
{
    const double shown = m_muted ? 0.0 : std::clamp(m_volume, 0.0, volume_curve::kMax);
    const bool muted = m_muted || shown == 0.0;
    const bool boosting = shown > 1.001;
    const double fillFraction = volume_curve::fractionFromValue(shown);
    const double filledPct = fillFraction * 100.0;
    const QColor boost = volume_curve::boostColor(shown);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect mute = muteRect();
    if (m_hover == HitRegion::Mute || m_pressed == HitRegion::Mute) {
        p.setPen(Qt::NoPen);
        p.setBrush(theme::white(0.10));
        p.drawEllipse(mute.adjusted(0, 0, -1, -1));
    }
    drawMuteGlyph(p, QRectF(12, 12, 24, 24), muted, theme::white(m_hover == HitRegion::Mute ? 1.0 : 0.90));

    const QRect track = trackRect();
    p.setPen(Qt::NoPen);
    p.setBrush(theme::white(0.15));
    p.drawRoundedRect(track, 4, 4);

    if (fillFraction > 0.0) {
        const QRectF fill(track.left(), track.top(), track.width() * fillFraction, track.height());
        if (shown <= 1.0) {
            p.setBrush(QColor(255, 255, 255, 235));
        } else {
            QLinearGradient grad(fill.left(), fill.top(), fill.right(), fill.top());
            const double breakPoint = volume_curve::kNormalFraction / fillFraction;
            grad.setColorAt(0.0, QColor(255, 255, 255, 235));
            grad.setColorAt(std::clamp(breakPoint, 0.0, 1.0), QColor(255, 255, 255, 235));
            grad.setColorAt(std::clamp(breakPoint, 0.0, 1.0), QColor(249, 115, 22));
            grad.setColorAt(1.0, boost);
            p.setBrush(grad);
        }
        p.drawRoundedRect(fill, 4, 4);
    }

    const double knobX = track.left() + track.width() * fillFraction;
    const QPointF knob(knobX, track.center().y() + 0.5);
    p.setBrush(theme::black(0.35));
    p.drawEllipse(knob, 9, 9);
    p.setBrush(boosting ? boost : QColor(255, 255, 255));
    p.drawEllipse(knob, 7, 7);

    if (boosting) {
        QFont f(QStringLiteral("Inter"));
        f.setPointSizeF(9.0);
        f.setBold(true);
        p.setFont(f);
        p.setPen(boost);
        p.drawText(valueRect(), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1%").arg(int(std::lround(shown * 100.0))));
    }
}

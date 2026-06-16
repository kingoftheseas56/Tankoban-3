// Tankoban 3 — ToggleSwitch. See ToggleSwitch.h.

#include "ui/ToggleSwitch.h"

#include <QColor>
#include <QMouseEvent>
#include <QPainter>

namespace tankoban {

namespace {
const QColor kAccent(0xe8, 0xb9, 0x23);
const QColor kKnob(0xf3, 0xf1, 0xea);
} // namespace

ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFixedSize(40, 22);
}

void ToggleSwitch::setOn(bool on)
{
    if (m_on == on)
        return;
    m_on = on;
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r(0, 0, width(), height());
    p.setPen(Qt::NoPen);
    p.setBrush(m_on ? kAccent : QColor(255, 255, 255, 38));
    p.drawRoundedRect(r, height() / 2.0, height() / 2.0);

    const qreal d = height() - 6;
    const qreal x = m_on ? width() - d - 3 : 3;
    p.setBrush(kKnob);
    p.drawEllipse(QRectF(x, 3, d, d));
}

void ToggleSwitch::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_on = !m_on;
        update();
        emit toggled(m_on);
    }
}

} // namespace tankoban

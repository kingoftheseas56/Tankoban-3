#include "chrome/TitleInfo.h"

#include "util/Theme.h"

#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>

namespace {

void drawShadowedText(QPainter& p, const QRect& rect, int flags, const QString& text,
                      const QFont& font, const QColor& fg, const QColor& shadow, const QPoint& offset)
{
    p.setFont(font);
    p.setPen(shadow);
    p.drawText(rect.translated(offset), flags, text);
    p.setPen(fg);
    p.drawText(rect, flags, text);
}

QPointF lucidePoint(const QRectF& rect, qreal x, qreal y)
{
    const qreal scale = std::min(rect.width(), rect.height()) / 24.0;
    const qreal ox = rect.x() + (rect.width() - 24.0 * scale) * 0.5;
    const qreal oy = rect.y() + (rect.height() - 24.0 * scale) * 0.5;
    return QPointF(ox + x * scale, oy + y * scale);
}

} // namespace

TitleInfo::TitleInfo(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setMouseTracking(true);
}

void TitleInfo::setText(const QString& title, const QString& subtitle)
{
    m_title = title;
    m_subtitle = subtitle;
    setVisible(!m_title.isEmpty());
    updateGeometry();
    update();
}

void TitleInfo::setClickable(bool clickable)
{
    m_clickable = clickable;
    setAttribute(Qt::WA_TransparentForMouseEvents, !clickable);
    setCursor(clickable ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
}

void TitleInfo::paintEvent(QPaintEvent*)
{
    if (m_title.isEmpty()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (m_clickable && (m_hover || m_pressed)) {
        p.setPen(Qt::NoPen);
        p.setBrush(theme::white(0.10));
        p.drawRoundedRect(rect(), 8, 8);
    }

    const int hPad = m_clickable ? 8 : 0;
    const int vPad = m_clickable ? 2 : 0;
    const int iconGap = m_clickable ? 8 : 0;
    const int iconW = m_clickable ? 14 : 0;

    const QRect content = rect().adjusted(hPad, vPad, -hPad, -vPad);
    const QRect textRect = content.adjusted(0, 0, -(iconW + iconGap), 0);

    QFont titleFont(QStringLiteral("Inter"));
    titleFont.setPointSizeF(14.25); // ~19 CSS px
    titleFont.setWeight(QFont::DemiBold);

    QFont subFont(QStringLiteral("Inter"));
    subFont.setPointSizeF(9.75); // ~13 CSS px
    subFont.setWeight(QFont::Normal);

    QFontMetrics titleFm(titleFont);
    QFontMetrics subFm(subFont);

    const QString title = titleFm.elidedText(m_title, Qt::ElideRight, std::max(1, textRect.width()));
    const QString subtitle = subFm.elidedText(m_subtitle, Qt::ElideRight, std::max(1, textRect.width()));

    const int titleH = titleFm.height();
    const int subH = m_subtitle.isEmpty() ? 0 : subFm.height();
    const int gap = m_subtitle.isEmpty() ? 0 : 2;
    const int totalH = titleH + gap + subH;
    const int y0 = textRect.top() + (textRect.height() - totalH) / 2;

    drawShadowedText(
        p,
        QRect(textRect.left(), y0, textRect.width(), titleH),
        Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine,
        title,
        titleFont,
        QColor(255, 255, 255),
        QColor(0, 0, 0, 153),
        QPoint(0, 2));

    if (!m_subtitle.isEmpty()) {
        drawShadowedText(
            p,
            QRect(textRect.left(), y0 + titleH + gap, textRect.width(), subH),
            Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine,
            subtitle,
            subFont,
            QColor(255, 255, 255, 179),
            QColor(0, 0, 0, 153),
            QPoint(0, 1));
    }

    if (m_clickable) {
        p.setPen(QPen(QColor(255, 255, 255, m_hover ? 242 : 128), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        const QRectF iconRect(content.right() - 14, content.center().y() - 7, 14, 14);
        drawInfoGlyph(p, iconRect);
    }
}

void TitleInfo::mousePressEvent(QMouseEvent* e)
{
    if (!m_clickable || e->button() != Qt::LeftButton) return;
    m_pressed = true;
    update();
}

void TitleInfo::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_clickable || e->button() != Qt::LeftButton) return;
    const bool inside = rect().contains(e->position().toPoint());
    const bool wasPressed = m_pressed;
    m_pressed = false;
    update();
    if (wasPressed && inside) emit clicked();
}

void TitleInfo::enterEvent(QEnterEvent*)
{
    m_hover = true;
    update();
}

void TitleInfo::leaveEvent(QEvent*)
{
    m_hover = false;
    m_pressed = false;
    update();
}

QSize TitleInfo::sizeHint() const
{
    if (m_title.isEmpty()) return QSize(0, 0);

    QFont titleFont(QStringLiteral("Inter"));
    titleFont.setPointSizeF(14.25);
    titleFont.setWeight(QFont::DemiBold);
    QFont subFont(QStringLiteral("Inter"));
    subFont.setPointSizeF(9.75);

    const QFontMetrics titleFm(titleFont);
    const QFontMetrics subFm(subFont);
    const int titleW = titleFm.horizontalAdvance(m_title);
    const int subW = m_subtitle.isEmpty() ? 0 : subFm.horizontalAdvance(m_subtitle);
    const int iconBlock = m_clickable ? 22 : 0;
    const int hPad = m_clickable ? 16 : 0;
    const int titleH = titleFm.height();
    const int subH = m_subtitle.isEmpty() ? 0 : subFm.height();
    const int gap = m_subtitle.isEmpty() ? 0 : 2;
    const int vPad = m_clickable ? 4 : 0;

    return QSize(std::max(titleW, subW) + iconBlock + hPad,
                 titleH + subH + gap + vPad);
}

void TitleInfo::drawInfoGlyph(QPainter& p, const QRectF& rect) const
{
    p.drawEllipse(rect.adjusted(1, 1, -1, -1));
    const QPointF center = rect.center();
    p.drawLine(lucidePoint(rect, 12, 10), lucidePoint(rect, 12, 16));
    p.drawPoint(lucidePoint(rect, 12, 7));
}

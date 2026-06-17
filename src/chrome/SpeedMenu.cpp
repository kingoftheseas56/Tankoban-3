#include "chrome/SpeedMenu.h"

#include "util/Theme.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

// Harbor token palette (matches AudioMenu/SubtitleMenu so the popups read alike).
QColor popupBg()     { return QColor(17, 20, 25, 247); }   // bg-elevated/97
QColor popupBorder() { return QColor(255, 255, 255, 22); } // border-edge
QColor elevatedBg()  { return QColor(255, 255, 255, 26); }
QColor textInk()     { return QColor(245, 247, 250); }
QColor textSubtle()  { return QColor(164, 172, 183); }
QColor textMuted()   { return QColor(131, 139, 151); }

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

// lucide Gauge: a speedometer dial (open at the bottom) + a needle to the upper-right.
void drawGauge(QPainter& p, const QRectF& iconRect, const QColor& color)
{
    p.setPen(QPen(color, 1.85, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const QRectF dial(lucidePoint(iconRect, 2, 4), lucidePoint(iconRect, 22, 24));
    p.drawArc(dial, -30 * 16, 240 * 16);          // 240deg dial, ~120deg gap at the bottom
    drawSegment(p, iconRect, 12, 14, 16, 10);     // needle
}

QString rateLabel(double rate)
{
    // "1.25", "1.5", "2", ... + multiplication sign (QChar avoids source-encoding issues).
    return QString::number(rate) + QChar(0x00D7);
}

const double kChoices[] = {0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0};

// ── One preset speed row ──────────────────────────────────────────────────────
class SpeedRowButton final : public QAbstractButton {
public:
    explicit SpeedRowButton(double rate, QWidget* parent = nullptr)
        : QAbstractButton(parent), m_rate(rate)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
        setFixedHeight(40);
    }
    double rate() const { return m_rate; }
    void setSelectedRow(bool s) { if (m_selected != s) { m_selected = s; update(); } }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRect r = rect().adjusted(0, 0, -1, -1);
        if (m_selected) {
            p.setPen(QPen(popupBorder(), 1.0));     // ring-1 ring-edge
            p.setBrush(elevatedBg());               // bg-elevated
            p.drawRoundedRect(r, 8, 8);
        } else if (underMouse()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 12));  // hover canvas/55-ish
            p.drawRoundedRect(r, 8, 8);
        }

        const bool normal = std::abs(m_rate - 1.0) < 0.01;
        QFont label(QStringLiteral("Inter"));
        label.setPixelSize(14);
        label.setWeight(m_selected ? QFont::Medium : QFont::Normal);
        p.setFont(label);
        p.setPen(m_selected ? textInk() : textMuted());
        p.drawText(QRect(12, 0, width() - 24, height()), Qt::AlignLeft | Qt::AlignVCenter,
                   normal ? QStringLiteral("Normal") : rateLabel(m_rate));

        if (normal) {
            QFont hint(QStringLiteral("Inter"));
            hint.setPixelSize(11);
            hint.setWeight(QFont::DemiBold);
            hint.setCapitalization(QFont::AllUppercase);
            p.setFont(hint);
            p.setPen(textSubtle());
            p.drawText(QRect(12, 0, width() - 24, height()), Qt::AlignRight | Qt::AlignVCenter,
                       QStringLiteral("default"));
        }
    }

private:
    double m_rate;
    bool m_selected = false;
};

} // namespace

// ── The floating popup ────────────────────────────────────────────────────────
class SpeedPopup final : public QWidget {
public:
    explicit SpeedPopup(QWidget* parent = nullptr) : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool)
    {
        setAttribute(Qt::WA_TranslucentBackground);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(8, 8, 8, 8);       // section p-2
        root->setSpacing(2);                         // gap-0.5

        auto* title = new QLabel(QStringLiteral("Playback speed"), this);
        title->setFixedHeight(24);
        title->setContentsMargins(12, 4, 12, 6);
        QFont tf(QStringLiteral("Inter"));
        tf.setPixelSize(11);
        tf.setWeight(QFont::DemiBold);
        tf.setCapitalization(QFont::AllUppercase);
        tf.setLetterSpacing(QFont::AbsoluteSpacing, 1.6);
        title->setFont(tf);
        title->setStyleSheet(QStringLiteral("color: rgb(164,172,183); background: transparent;"));
        root->addWidget(title);

        for (double r : kChoices) {
            auto* row = new SpeedRowButton(r, this);
            connect(row, &QAbstractButton::clicked, this, [this, r] {
                if (m_rateHandler) m_rateHandler(r);
                if (m_closeHandler) m_closeHandler();
            });
            m_rows.push_back(row);
            root->addWidget(row);
        }

        // Deterministic height: margins(16) + title(24) + 7*40 + 7 gaps*2 = 334.
        setFixedSize(400, 16 + 24 + 7 * 40 + 7 * 2);
    }

    void setSnapshot(const PlayerSnapshot& snap)
    {
        for (SpeedRowButton* row : m_rows)
            row->setSelectedRow(std::abs(row->rate() - snap.rate) < 0.01);
    }

    void setRateHandler(std::function<void(double)> fn) { m_rateHandler = std::move(fn); }
    void setCloseHandler(std::function<void()> fn) { m_closeHandler = std::move(fn); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(QPen(popupBorder(), 1.0));
        p.setBrush(popupBg());
        p.drawRoundedRect(r, 16, 16);               // rounded-2xl
    }

private:
    QVector<SpeedRowButton*> m_rows;
    std::function<void(double)> m_rateHandler;
    std::function<void()> m_closeHandler;
};

// ── The trigger button ────────────────────────────────────────────────────────
SpeedMenu::SpeedMenu(QWidget* parent) : QWidget(parent)
{
    setFixedSize(48, 48);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);

    m_popup = new SpeedPopup();
    m_popup->setRateHandler([this](double r) {
        m_snapshot.rate = r;            // optimistic, mirrors MpvController
        m_popup->setSnapshot(m_snapshot);
        setSnapshot(m_snapshot);        // resize pill + repaint
        emit rateRequested(r);
        hidePopup();
    });
    m_popup->setCloseHandler([this] { hidePopup(); });

    qApp->installEventFilter(this);
}

SpeedMenu::~SpeedMenu()
{
    qApp->removeEventFilter(this);
    delete m_popup;
}

bool SpeedMenu::rateActive() const { return std::abs(m_snapshot.rate - 1.0) > 0.01; }

void SpeedMenu::setSnapshot(const PlayerSnapshot& snap)
{
    m_snapshot = snap;
    int w = 48;
    if (rateActive()) {
        QFont lf(QStringLiteral("Inter"));
        lf.setPixelSize(11);
        lf.setBold(true);
        const int labelW = QFontMetrics(lf).horizontalAdvance(rateLabel(snap.rate));
        w = 14 + 22 + 6 + labelW + 14;  // padL + gauge + gap + label + padR (pill)
    }
    if (width() != w) setFixedWidth(w);  // TransportBar::setSnapshot re-runs layoutChrome
    if (m_popup) m_popup->setSnapshot(snap);
    update();
}

void SpeedMenu::syncPopupPosition()
{
    if (!m_open || !m_popup) return;

    QScreen* screen = window() ? window()->screen() : QGuiApplication::primaryScreen();
    const QRect available = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 720);

    QPoint pos = mapToGlobal(QPoint(width() - m_popup->width(), -10 - m_popup->height()));
    if (pos.x() < available.left() + 16) pos.setX(available.left() + 16);
    if (pos.x() + m_popup->width() > available.right() - 15)
        pos.setX(available.right() - 15 - m_popup->width());
    if (pos.y() < available.top() + 16)
        pos = mapToGlobal(QPoint(width() - m_popup->width(), height() + 10));
    if (pos.y() + m_popup->height() > available.bottom() - 15)
        pos.setY(std::max(available.top() + 16, available.bottom() - 15 - m_popup->height()));

    m_popup->move(pos);
}

bool SpeedMenu::eventFilter(QObject*, QEvent* event)
{
    if (!m_open || !m_popup) return false;

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        const QPoint global = mouse->globalPosition().toPoint();
        if (!globalRect().contains(global) && !m_popup->frameGeometry().contains(global)) hidePopup();
    } else if (event->type() == QEvent::ApplicationDeactivate) {
        hidePopup();
    } else if (event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Escape) hidePopup();
    }
    return false;
}

void SpeedMenu::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const bool active = m_open || rateActive();
    QColor bg(Qt::transparent);
    if (active) bg = m_hover ? QColor(255, 255, 255, 76) : QColor(255, 255, 255, 56);  // white/30 : white/22
    else if (m_hover || m_pressed) bg = QColor(255, 255, 255, 26);                       // white/10
    if (bg.alpha() > 0) {
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), height() / 2.0, height() / 2.0);
    }

    const QColor fg = (active || m_hover) ? theme::white(1.0) : theme::white(0.85);
    const bool showLabel = rateActive();

    QFont lf(QStringLiteral("Inter"));
    lf.setPixelSize(11);
    lf.setBold(true);
    const QString label = rateLabel(m_snapshot.rate);
    const int gaugeW = 22;
    const int gap = showLabel ? 6 : 0;
    const int labelW = showLabel ? QFontMetrics(lf).horizontalAdvance(label) : 0;
    const int groupW = gaugeW + gap + labelW;
    const int startX = (width() - groupW) / 2;

    const QRectF iconRect(startX, (height() - 22) / 2.0, 22, 22);
    drawGauge(p, iconRect, fg);
    if (showLabel) {
        p.setFont(lf);
        p.setPen(fg);
        p.drawText(QRect(startX + gaugeW + gap, 0, labelW, height()),
                   Qt::AlignLeft | Qt::AlignVCenter, label);
    }
}

void SpeedMenu::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    m_pressed = true;
    update();
}

void SpeedMenu::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    const bool inside = rect().contains(event->position().toPoint());
    const bool wasPressed = m_pressed;
    m_pressed = false;
    update();
    if (wasPressed && inside) toggleOpen();
}

void SpeedMenu::enterEvent(QEnterEvent* event)
{
    m_hover = true;
    update();
    QWidget::enterEvent(event);
}

void SpeedMenu::leaveEvent(QEvent* event)
{
    m_hover = false;
    m_pressed = false;
    update();
    QWidget::leaveEvent(event);
}

void SpeedMenu::setOpen(bool open)
{
    if (m_open == open) return;
    m_open = open;
    emit openChanged(open);
    update();
}

void SpeedMenu::toggleOpen()
{
    if (m_open) hidePopup();
    else showPopup();
}

void SpeedMenu::showPopup()
{
    if (!m_popup) return;
    m_popup->setSnapshot(m_snapshot);
    syncPopupPosition();
    m_popup->show();
    m_popup->raise();
    setOpen(true);
}

void SpeedMenu::hidePopup()
{
    if (!m_popup) return;
    m_popup->hide();
    setOpen(false);
}

QRect SpeedMenu::globalRect() const
{
    return QRect(mapToGlobal(QPoint(0, 0)), size());
}

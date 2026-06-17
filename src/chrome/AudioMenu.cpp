#include "chrome/AudioMenu.h"

#include "util/Theme.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

QColor popupBg() { return QColor(17, 20, 25, 247); }
QColor popupBorder() { return QColor(255, 255, 255, 22); }
QColor raisedBg() { return QColor(255, 255, 255, 18); }
QColor elevatedBg() { return QColor(255, 255, 255, 26); }
QColor textInk() { return QColor(245, 247, 250); }
QColor textSubtle() { return QColor(164, 172, 183); }
QColor textMuted() { return QColor(131, 139, 151); }

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

QString langBadge(const QString& lang)
{
    if (lang.isEmpty()) return QString();
    const QString compact = lang.simplified().remove(' ').left(3).toUpper();
    return compact.isEmpty() ? QStringLiteral("AUD") : compact;
}

QString trackTitle(const TrackInfo& t)
{
    if (!t.title.trimmed().isEmpty() && t.title != t.lang) return t.title;
    if (!t.label.trimmed().isEmpty() && t.label != t.lang) return t.label;
    if (!t.lang.trimmed().isEmpty()) return t.lang.toUpper();
    return QStringLiteral("Track");
}

QString trackSubtitle(const TrackInfo& t)
{
    QStringList parts;
    if (!t.lang.trimmed().isEmpty()) parts.push_back(t.lang.toUpper());
    if (!t.codec.trimmed().isEmpty()) parts.push_back(t.codec.toUpper());
    if (!t.channels.trimmed().isEmpty()) parts.push_back(t.channels);
    if (t.isDefault) parts.push_back(QStringLiteral("DEFAULT"));
    return parts.join(QStringLiteral(" / "));
}

double roundedDelay(double sec)
{
    return std::round(sec * 100.0) / 100.0;
}

class PopupIconButton final : public QAbstractButton {
public:
    enum class Icon { Close, Reset };

    PopupIconButton(Icon icon, QWidget* parent = nullptr) : QAbstractButton(parent), m_icon(icon)
    {
        setFixedSize(m_icon == Icon::Close ? 28 : 24, m_icon == Icon::Close ? 28 : 24);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        if (underMouse() || isDown()) {
            p.setPen(Qt::NoPen);
            p.setBrush(raisedBg());
            p.drawEllipse(rect().adjusted(0, 0, -1, -1));
        }
        p.setPen(QPen(underMouse() ? textInk() : textSubtle(), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (m_icon == Icon::Close) {
            const QRectF r(7, 7, 14, 14);
            drawSegment(p, r, 5, 5, 19, 19);
            drawSegment(p, r, 19, 5, 5, 19);
            return;
        }

        const QRectF r(4, 4, 16, 16);
        p.drawArc(r.adjusted(1, 1, -1, -1), 30 * 16, 280 * 16);
        QPainterPath arrow;
        arrow.moveTo(lucidePoint(r, 17, 4));
        arrow.lineTo(lucidePoint(r, 20, 5));
        arrow.lineTo(lucidePoint(r, 18, 8));
        p.drawPath(arrow);
    }

private:
    Icon m_icon;
};

class TrackRowButton final : public QAbstractButton {
public:
    explicit TrackRowButton(const TrackInfo& track, QWidget* parent = nullptr)
        : QAbstractButton(parent), m_track(track)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
        setFixedHeight(46);
    }

    void setSelectedTrack(bool selected)
    {
        if (m_selected == selected) return;
        m_selected = selected;
        update();
    }

    QString trackId() const { return m_track.id; }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        if (m_selected) {
            p.setPen(QPen(QColor(255, 255, 255, 28), 1.0));
            p.setBrush(elevatedBg());
            p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);
        } else if (underMouse()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 10));
            p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);
        }

        const QRect circle(10, 14, 16, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(m_selected ? theme::accent() : raisedBg());
        p.drawEllipse(circle);
        if (m_selected) {
            p.setPen(QPen(QColor(19, 21, 27), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawLine(circle.left() + 4, circle.center().y(), circle.left() + 7, circle.bottom() - 4);
            p.drawLine(circle.left() + 7, circle.bottom() - 4, circle.right() - 3, circle.top() + 4);
        }

        int textLeft = circle.right() + 11;
        const QString badge = langBadge(m_track.lang);
        if (!badge.isEmpty()) {
            QFont badgeFont(QStringLiteral("Inter"));
            badgeFont.setPointSizeF(8.25);
            badgeFont.setBold(true);
            p.setFont(badgeFont);
            QFontMetrics badgeFm(badgeFont);
            const int badgeW = badgeFm.horizontalAdvance(badge) + 12;
            const QRect badgeRect(textLeft, 12, badgeW, 18);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 22));
            p.drawRoundedRect(badgeRect, 9, 9);
            p.setPen(textSubtle());
            p.drawText(badgeRect, Qt::AlignCenter, badge);
            textLeft = badgeRect.right() + 10;
        }

        const QRect textRect(textLeft, 8, width() - textLeft - 12, height() - 16);

        QFont titleFont(QStringLiteral("Inter"));
        titleFont.setPointSizeF(9.4);
        titleFont.setWeight(QFont::Medium);
        p.setFont(titleFont);
        QFontMetrics titleFm(titleFont);
        const QString title = titleFm.elidedText(trackTitle(m_track), Qt::ElideRight, textRect.width());
        p.setPen(textInk());
        p.drawText(QRect(textRect.left(), textRect.top(), textRect.width(), 16),
                   Qt::AlignLeft | Qt::AlignVCenter, title);

        QFont subFont(QStringLiteral("Inter"));
        subFont.setPointSizeF(7.9);
        subFont.setCapitalization(QFont::AllUppercase);
        p.setFont(subFont);
        QFontMetrics subFm(subFont);
        const QString subtitle = subFm.elidedText(trackSubtitle(m_track), Qt::ElideRight, textRect.width());
        p.setPen(textSubtle());
        p.drawText(QRect(textRect.left(), textRect.top() + 18, textRect.width(), 14),
                   Qt::AlignLeft | Qt::AlignVCenter, subtitle);
    }

private:
    TrackInfo m_track;
    bool m_selected = false;
};

} // namespace

class AudioPopup final : public QWidget {
public:
    explicit AudioPopup(QWidget* parent = nullptr) : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool)
    {
        setAttribute(Qt::WA_TranslucentBackground);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        auto* header = new QWidget(this);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(16, 10, 10, 10);
        headerLayout->setSpacing(8);

        auto* title = new QLabel(QStringLiteral("Audio"), header);
        QFont titleFont(QStringLiteral("Inter"));
        titleFont.setPointSizeF(10.2);
        titleFont.setWeight(QFont::DemiBold);
        title->setFont(titleFont);
        title->setStyleSheet(QStringLiteral("color: rgb(245,247,250); background: transparent;"));

        m_count = new QLabel(header);
        QFont countFont(QStringLiteral("Inter"));
        countFont.setPointSizeF(8.5);
        m_count->setFont(countFont);
        m_count->setStyleSheet(QStringLiteral("color: rgb(164,172,183); background: transparent;"));

        auto* close = new PopupIconButton(PopupIconButton::Icon::Close, header);
        connect(close, &QAbstractButton::clicked, this, [this] {
            if (m_closeHandler) m_closeHandler();
        });

        headerLayout->addWidget(title);
        headerLayout->addWidget(m_count);
        headerLayout->addStretch(1);
        headerLayout->addWidget(close);
        root->addWidget(header);

        m_scroll = new QScrollArea(this);
        m_scroll->setFrameShape(QFrame::NoFrame);
        m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scroll->setWidgetResizable(true);
        m_scroll->setStyleSheet(QStringLiteral(
            "QScrollArea { background: transparent; }"
            "QScrollBar:vertical { width: 10px; background: transparent; margin: 4px 2px 4px 0; }"
            "QScrollBar::handle:vertical { background: rgba(255,255,255,0.18); border-radius: 5px; min-height: 28px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"));
        m_trackHost = new QWidget(m_scroll);
        m_trackLayout = new QVBoxLayout(m_trackHost);
        m_trackLayout->setContentsMargins(8, 8, 8, 8);
        m_trackLayout->setSpacing(2);
        m_scroll->setWidget(m_trackHost);
        root->addWidget(m_scroll);

        auto* delayRow = new QWidget(this);
        auto* delayLayout = new QHBoxLayout(delayRow);
        delayLayout->setContentsMargins(12, 8, 12, 8);
        delayLayout->setSpacing(6);

        auto* syncLabel = new QLabel(QStringLiteral("SYNC"), delayRow);
        QFont syncFont(QStringLiteral("Inter"));
        syncFont.setPointSizeF(7.9);
        syncFont.setBold(true);
        syncFont.setCapitalization(QFont::AllUppercase);
        syncLabel->setFont(syncFont);
        syncLabel->setStyleSheet(QStringLiteral("color: rgb(131,139,151); background: transparent;"));

        auto makeStepButton = [delayRow](const QString& text) {
            auto* btn = new QPushButton(text, delayRow);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFocusPolicy(Qt::NoFocus);
            btn->setFixedHeight(24);
            btn->setStyleSheet(QStringLiteral(
                "QPushButton {"
                " background: rgba(255,255,255,0.07); color: rgb(245,247,250);"
                " border: none; border-radius: 6px; padding: 0 8px;"
                " font-family: Inter; font-size: 11px; font-weight: 600; }"
                "QPushButton:hover { background: rgba(255,255,255,0.12); }"
                "QPushButton:disabled { color: rgba(245,247,250,0.35); }"));
            return btn;
        };

        m_minus = makeStepButton(QStringLiteral("-0.1"));
        m_plus = makeStepButton(QStringLiteral("+0.1"));
        m_delayValue = new QLabel(delayRow);
        QFont mono(QStringLiteral("Consolas"));
        mono.setPointSizeF(9.5);
        m_delayValue->setFont(mono);
        m_delayValue->setAlignment(Qt::AlignCenter);
        m_delayValue->setStyleSheet(QStringLiteral("color: rgb(245,247,250); background: transparent;"));

        m_reset = new PopupIconButton(PopupIconButton::Icon::Reset, delayRow);

        connect(m_minus, &QPushButton::clicked, this, [this] {
            if (m_delayHandler) m_delayHandler(roundedDelay(m_snapshot.audioDelaySec - 0.1));
        });
        connect(m_plus, &QPushButton::clicked, this, [this] {
            if (m_delayHandler) m_delayHandler(roundedDelay(m_snapshot.audioDelaySec + 0.1));
        });
        connect(m_reset, &QAbstractButton::clicked, this, [this] {
            if (m_delayHandler) m_delayHandler(0.0);
        });

        delayLayout->addWidget(syncLabel);
        delayLayout->addWidget(m_minus);
        delayLayout->addWidget(m_delayValue, 1);
        delayLayout->addWidget(m_plus);
        delayLayout->addWidget(m_reset);
        root->addWidget(delayRow);
    }

    void setSnapshot(const PlayerSnapshot& snap)
    {
        m_snapshot = snap;
        rebuildTracks();
        refreshDelay();
        resize(popupWidth(), popupHeightForCount(std::max(1, int(m_snapshot.audioTracks.size()))));
    }

    void setTrackHandler(std::function<void(const QString&)> fn) { m_trackHandler = std::move(fn); }
    void setDelayHandler(std::function<void(double)> fn) { m_delayHandler = std::move(fn); }
    void setCloseHandler(std::function<void()> fn) { m_closeHandler = std::move(fn); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(popupBorder(), 1.0));
        p.setBrush(popupBg());
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 16, 16);

        p.setPen(QPen(QColor(255, 255, 255, 18), 1.0));
        p.drawLine(0, 48, width(), 48);
        p.drawLine(0, height() - 41, width(), height() - 41);
    }

private:
    static int popupWidth() { return 360; }

    static int popupHeightForCount(int rows)
    {
        const int headerH = 49;
        const int footerH = 41;
        const int contentH = std::clamp(rows * 48 + 16, 72, 310);
        return headerH + contentH + footerH;
    }

    void rebuildTracks()
    {
        while (QLayoutItem* item = m_trackLayout->takeAt(0)) {
            if (QWidget* w = item->widget()) w->deleteLater();
            delete item;
        }

        if (!m_snapshot.audioTracks.isEmpty()) {
            for (const TrackInfo& track : m_snapshot.audioTracks) {
                auto* row = new TrackRowButton(track, m_trackHost);
                row->setSelectedTrack(track.selected);
                connect(row, &QAbstractButton::clicked, this, [this, row] {
                    if (m_trackHandler) m_trackHandler(row->trackId());
                });
                m_trackLayout->addWidget(row);
            }
            m_trackLayout->addStretch(1);
            m_count->setText(QString::number(m_snapshot.audioTracks.size()));
            m_count->setVisible(true);
            return;
        }

        auto* empty = new QLabel(QStringLiteral("This file has one audio track."), m_trackHost);
        empty->setWordWrap(true);
        empty->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        empty->setContentsMargins(8, 8, 8, 8);
        QFont emptyFont(QStringLiteral("Inter"));
        emptyFont.setPointSizeF(9.2);
        empty->setFont(emptyFont);
        empty->setStyleSheet(QStringLiteral("color: rgb(131,139,151); background: transparent;"));
        m_trackLayout->addWidget(empty);
        m_trackLayout->addStretch(1);
        m_count->setVisible(false);
    }

    void refreshDelay()
    {
        m_delayValue->setText(QStringLiteral("%1%2s")
                                  .arg(m_snapshot.audioDelaySec >= 0.0 ? QStringLiteral("+") : QString())
                                  .arg(m_snapshot.audioDelaySec, 0, 'f', 2));
        m_reset->setVisible(std::abs(m_snapshot.audioDelaySec) > 0.0001);
    }

    PlayerSnapshot m_snapshot;
    QLabel* m_count = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_trackHost = nullptr;
    QVBoxLayout* m_trackLayout = nullptr;
    QPushButton* m_minus = nullptr;
    QPushButton* m_plus = nullptr;
    QLabel* m_delayValue = nullptr;
    PopupIconButton* m_reset = nullptr;
    std::function<void(const QString&)> m_trackHandler;
    std::function<void(double)> m_delayHandler;
    std::function<void()> m_closeHandler;
};

AudioMenu::AudioMenu(QWidget* parent) : QWidget(parent)
{
    setFixedSize(48, 48);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);

    m_popup = new AudioPopup();
    m_popup->setTrackHandler([this](const QString& id) {
        emit trackRequested(id);
        hidePopup();
    });
    m_popup->setDelayHandler([this](double sec) {
        m_snapshot.audioDelaySec = sec;
        m_popup->setSnapshot(m_snapshot);
        emit delayRequested(sec);
    });
    m_popup->setCloseHandler([this] { hidePopup(); });

    qApp->installEventFilter(this);
}

AudioMenu::~AudioMenu()
{
    qApp->removeEventFilter(this);
    delete m_popup;
}

void AudioMenu::setSnapshot(const PlayerSnapshot& snap)
{
    m_snapshot = snap;
    if (m_popup) m_popup->setSnapshot(snap);
    update();
}

void AudioMenu::syncPopupPosition()
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

bool AudioMenu::eventFilter(QObject*, QEvent* event)
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

void AudioMenu::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (m_open || m_hover || m_pressed) {
        p.setPen(Qt::NoPen);
        p.setBrush(m_open ? QColor(255, 255, 255, 56) : QColor(255, 255, 255, 26));
        p.drawEllipse(rect().adjusted(0, 0, -1, -1));
    }

    p.setPen(QPen(QColor(255, 255, 255, m_open ? 255 : 220), 1.95, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const QRectF iconRect(14, 14, 20, 20);

    QPainterPath left;
    left.moveTo(lucidePoint(iconRect, 5, 7));
    left.lineTo(lucidePoint(iconRect, 10, 17));
    left.lineTo(lucidePoint(iconRect, 15, 7));
    p.drawPath(left);
    drawSegment(p, iconRect, 7.3, 12.4, 12.7, 12.4);

    drawSegment(p, iconRect, 17, 7, 21, 7);
    drawSegment(p, iconRect, 19, 7, 19, 17);
    drawSegment(p, iconRect, 16.5, 10.5, 21.5, 10.5);
    drawSegment(p, iconRect, 16.5, 13.8, 21.5, 13.8);
}

void AudioMenu::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    m_pressed = true;
    update();
}

void AudioMenu::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    const bool inside = rect().contains(event->position().toPoint());
    const bool wasPressed = m_pressed;
    m_pressed = false;
    update();
    if (wasPressed && inside) toggleOpen();
}

void AudioMenu::enterEvent(QEnterEvent* event)
{
    m_hover = true;
    update();
    QWidget::enterEvent(event);
}

void AudioMenu::leaveEvent(QEvent* event)
{
    m_hover = false;
    m_pressed = false;
    update();
    QWidget::leaveEvent(event);
}

void AudioMenu::setOpen(bool open)
{
    if (m_open == open) return;
    m_open = open;
    emit openChanged(open);
    update();
}

void AudioMenu::toggleOpen()
{
    if (m_open) hidePopup();
    else showPopup();
}

void AudioMenu::showPopup()
{
    if (!m_popup) return;
    m_popup->setSnapshot(m_snapshot);
    syncPopupPosition();
    m_popup->show();
    m_popup->raise();
    setOpen(true);
}

void AudioMenu::hidePopup()
{
    if (!m_popup) return;
    m_popup->hide();
    setOpen(false);
}

QRect AudioMenu::globalRect() const
{
    return QRect(mapToGlobal(QPoint(0, 0)), size());
}

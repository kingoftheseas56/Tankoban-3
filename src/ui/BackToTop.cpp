// Tankoban 3 — BackToTop. See BackToTop.h.

#include "ui/BackToTop.h"

#include "ui/Icons.h"

#include <QAbstractAnimation>
#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>

namespace tankoban {

BackToTop::BackToTop(QScrollArea* area, QWidget* parent)
    : QPushButton(parent)
    , m_area(area)
{
    setObjectName(QStringLiteral("BackToTop"));
    setCursor(Qt::PointingHandCursor);
    setFixedSize(44, 44);
    setIcon(navIcon(QStringLiteral("chev-up"), QColor(QStringLiteral("#f3f1ea")), 22));
    setIconSize(QSize(22, 22));
    setStyleSheet(QStringLiteral(
        "#BackToTop{border:1px solid rgba(255,255,255,0.14);border-radius:22px;"
        "background:rgba(26,29,36,0.92);}"
        "#BackToTop:hover{background:rgba(45,51,63,0.96);}"));
    hide();

    if (m_area) {
        connect(m_area->verticalScrollBar(), &QScrollBar::valueChanged, this,
                [this]() { refresh(); });
        connect(m_area->verticalScrollBar(), &QScrollBar::rangeChanged, this,
                [this]() { refresh(); });
    }
    connect(this, &QPushButton::clicked, this, [this]() {
        if (!m_area)
            return;
        auto* sb = m_area->verticalScrollBar();
        auto* anim = new QPropertyAnimation(sb, "value", this);
        anim->setDuration(320);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setStartValue(sb->value());
        anim->setEndValue(0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });

    if (parent)
        parent->installEventFilter(this);
    reposition();
}

bool BackToTop::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parent()
        && (event->type() == QEvent::Resize || event->type() == QEvent::Show))
        reposition();
    return QPushButton::eventFilter(watched, event);
}

void BackToTop::refresh()
{
    if (!m_area)
        return;
    const bool visible = m_area->verticalScrollBar()->value() > 400; // ~after a screenful
    setVisible(visible);
    if (visible) {
        reposition();
        raise();
    }
}

void BackToTop::reposition()
{
    if (QWidget* p = parentWidget())
        move(p->width() - width() - 28, p->height() - height() - 28);
}

} // namespace tankoban

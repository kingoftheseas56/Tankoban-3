// Tankoban 3 — CatalogRow (Step 3 / lazy + Harbor arrows). See CatalogRow.h.

#include "ui/CatalogRow.h"

#include "ui/Icons.h"
#include "ui/PosterCard.h"

#include <QAbstractAnimation>
#include <QColor>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QSize>
#include <QTimer>
#include <QVBoxLayout>

namespace tankoban {

CatalogRow::CatalogRow(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("CatalogRow"));

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(12);

    m_title = new QLabel(title, this);
    m_title->setObjectName(QStringLiteral("RowTitle"));
    col->addWidget(m_title);

    m_status = new QLabel(QStringLiteral("Loading…"), this);
    m_status->setObjectName(QStringLiteral("RowStatus"));
    col->addWidget(m_status);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("RowScroll"));
    m_scroll->setWidgetResizable(true);
    // Harbor: the scrollbar is hidden; you scroll via the hover edge-arrows.
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setFixedHeight(PosterCard::kPosterH + 44);

    m_track = new QWidget(m_scroll);
    m_track->setObjectName(QStringLiteral("RowTrack"));
    m_trackLayout = new QHBoxLayout(m_track);
    m_trackLayout->setContentsMargins(0, 0, 0, 0);
    m_trackLayout->setSpacing(16);
    m_trackLayout->addStretch();
    m_scroll->setWidget(m_track);
    col->addWidget(m_scroll);

    // Hover edge-arrows (Harbor). Floated over the row, shown on hover.
    auto makeArrow = [this](const QString& chev, int dir) {
        auto* b = new QPushButton(this);
        b->setObjectName(QStringLiteral("RowArrow"));
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(36, 36);
        b->setIcon(navIcon(chev, QColor(QStringLiteral("#f3f1ea")), 18));
        b->setIconSize(QSize(18, 18));
        b->hide();
        connect(b, &QPushButton::clicked, this, [this, dir]() { scrollByPage(dir); });
        return b;
    };
    m_leftArrow = makeArrow(QStringLiteral("chev-left"), -1);
    m_rightArrow = makeArrow(QStringLiteral("chev-right"), +1);

    connect(m_scroll->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() { updateVisible(); updateArrows(); });
    connect(m_scroll->horizontalScrollBar(), &QScrollBar::rangeChanged, this,
            [this]() { updateVisible(); updateArrows(); });
}

void CatalogRow::setStatus(const QString& text)
{
    m_status->setText(text);
    m_status->setVisible(!text.isEmpty());
}

void CatalogRow::setItems(const QVector<MetaItem>& items)
{
    setStatus(items.isEmpty() ? QStringLiteral("No results") : QString());
    const int cap = qMin(items.size(), 30);
    for (int i = 0; i < cap; ++i) {
        auto* card = new PosterCard(items.at(i), m_track);
        m_trackLayout->insertWidget(m_trackLayout->count() - 1, card); // before stretch
        m_cards.push_back(card);
    }
    QTimer::singleShot(0, this, [this]() { updateVisible(); updateArrows(); });
}

void CatalogRow::updateVisible()
{
    if (!m_scroll || m_cards.isEmpty())
        return;
    if (m_cards.size() > 1 && m_cards.last()->x() == 0)
        return; // not laid out yet
    const int sx = m_scroll->horizontalScrollBar()->value();
    const int vw = m_scroll->viewport()->width();
    const int margin = 400;
    const int lo = sx - margin;
    const int hi = sx + vw + margin;
    for (PosterCard* c : m_cards) {
        const int cx = c->x();
        if (cx + c->width() >= lo && cx <= hi)
            c->ensureLoaded();
    }
}

void CatalogRow::updateArrows()
{
    if (!m_scroll || !m_leftArrow || !m_rightArrow)
        return;
    auto* sb = m_scroll->horizontalScrollBar();
    const bool scrollable = sb->maximum() > sb->minimum();
    const int ay = m_scroll->y() + PosterCard::kPosterH / 2 - 18;
    m_leftArrow->move(m_scroll->x() + 6, ay);
    m_rightArrow->move(m_scroll->x() + m_scroll->width() - 36 - 6, ay);
    m_leftArrow->setVisible(m_hovered && scrollable && sb->value() > sb->minimum());
    m_rightArrow->setVisible(m_hovered && scrollable && sb->value() < sb->maximum());
    m_leftArrow->raise();
    m_rightArrow->raise();
}

void CatalogRow::scrollByPage(int dir)
{
    if (!m_scroll)
        return;
    auto* sb = m_scroll->horizontalScrollBar();
    const int step = int(m_scroll->viewport()->width() * 0.85);
    const int target = qBound(sb->minimum(), sb->value() + dir * step, sb->maximum());
    auto* anim = new QPropertyAnimation(sb, "value", this);
    anim->setDuration(300);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setStartValue(sb->value());
    anim->setEndValue(target);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CatalogRow::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    QTimer::singleShot(0, this, [this]() { updateVisible(); updateArrows(); });
}

void CatalogRow::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    updateVisible();
    updateArrows();
}

void CatalogRow::enterEvent(QEnterEvent*)
{
    m_hovered = true;
    updateArrows();
}

void CatalogRow::leaveEvent(QEvent*)
{
    m_hovered = false;
    updateArrows();
}

} // namespace tankoban

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
    col->setSpacing(20); // Harbor row flex flex-col gap-5

    // Header: title on the left, optional "View all" on the right (Harbor row.tsx —
    // a flex justify-between header that reveals View all whenever onViewAll is wired).
    auto* header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(8);

    m_title = new QLabel(title, this);
    m_title->setObjectName(QStringLiteral("RowTitle"));
    header->addWidget(m_title);
    header->addStretch();

    m_viewAll = new QPushButton(QStringLiteral("View all"), this);
    m_viewAll->setObjectName(QStringLiteral("RowViewAll"));
    m_viewAll->setCursor(Qt::PointingHandCursor);
    m_viewAll->setIcon(navIcon(QStringLiteral("chev-right"), QColor(QStringLiteral("#8b8f99")), 14));
    m_viewAll->setIconSize(QSize(14, 14));
    m_viewAll->setLayoutDirection(Qt::RightToLeft); // icon trails the label, Harbor-style
    m_viewAll->setStyleSheet(QStringLiteral(
        "#RowViewAll{border:none;background:transparent;color:#8b8f99;font-size:12.5px;"
        "font-weight:500;padding:2px 2px;}"
        "#RowViewAll:hover{color:#f3f1ea;}"));
    m_viewAll->hide();
    connect(m_viewAll, &QPushButton::clicked, this, &CatalogRow::viewAllRequested);
    header->addWidget(m_viewAll);

    col->addLayout(header);

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
    // Fits the card: hover-lift + poster + gap-2.5 + a 2-line title.
    m_scroll->setFixedHeight(PosterCard::kHoverLift + PosterCard::kPosterH + 10 + 38);

    m_track = new QWidget(m_scroll);
    m_track->setObjectName(QStringLiteral("RowTrack"));
    m_trackLayout = new QHBoxLayout(m_track);
    m_trackLayout->setContentsMargins(0, 0, 0, 0);
    m_trackLayout->setSpacing(20); // Harbor GAP = 20
    m_trackLayout->addStretch();
    m_scroll->setWidget(m_track);
    col->addWidget(m_scroll);

    // Hover edge-arrows (Harbor). Floated over the row, shown on hover.
    auto makeArrow = [this](const QString& chev, int dir) {
        auto* b = new QPushButton(this);
        b->setObjectName(QStringLiteral("RowArrow"));
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(44, 44); // Harbor: 44px circular edge buttons
        b->setIcon(navIcon(chev, QColor(QStringLiteral("#f3f1ea")), 20));
        b->setIconSize(QSize(20, 20));
        b->hide();
        connect(b, &QPushButton::clicked, this, [this, dir]() { scrollByPage(dir); });
        return b;
    };
    m_leftArrow = makeArrow(QStringLiteral("chev-left"), -1);
    m_rightArrow = makeArrow(QStringLiteral("chev-right"), +1);

    connect(m_scroll->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() { updateVisible(); updateArrows(); maybeEmitEndReached(); });
    connect(m_scroll->horizontalScrollBar(), &QScrollBar::rangeChanged, this,
            [this]() { updateVisible(); updateArrows(); });
}

void CatalogRow::setStatus(const QString& text)
{
    m_status->setText(text);
    m_status->setVisible(!text.isEmpty());
}

void CatalogRow::setViewAllVisible(bool visible)
{
    if (m_viewAll)
        m_viewAll->setVisible(visible);
}

void CatalogRow::setItems(const QVector<MetaItem>& items)
{
    setStatus(items.isEmpty() ? QStringLiteral("No results") : QString());
    const int cap = qMin(items.size(), 30);
    for (int i = 0; i < cap; ++i) {
        auto* card = new PosterCard(items.at(i), m_track);
        connect(card, &PosterCard::activated, this, &CatalogRow::activated);
        m_trackLayout->insertWidget(m_trackLayout->count() - 1, card); // before stretch
        m_cards.push_back(card);
    }
    QTimer::singleShot(0, this, [this]() { updateVisible(); updateArrows(); });
}

void CatalogRow::appendItems(const QVector<MetaItem>& items)
{
    for (const MetaItem& m : items) {
        auto* card = new PosterCard(m, m_track);
        connect(card, &PosterCard::activated, this, &CatalogRow::activated);
        m_trackLayout->insertWidget(m_trackLayout->count() - 1, card); // before stretch
        m_cards.push_back(card);
    }
    QTimer::singleShot(0, this, [this]() { updateVisible(); updateArrows(); });
}

void CatalogRow::maybeEmitEndReached()
{
    if (!m_scroll)
        return;
    auto* sb = m_scroll->horizontalScrollBar();
    if (sb->maximum() <= sb->minimum())
        return;
    // Harbor measureScroll: fire when the remaining scroll is small (≈ < 800px).
    if (sb->maximum() - sb->value() < int(m_scroll->viewport()->width() * 0.6))
        emit endReached();
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
    const int ay = m_scroll->y() + PosterCard::kHoverLift + PosterCard::kPosterH / 2 - 22;
    m_leftArrow->move(m_scroll->x() + 8, ay);
    m_rightArrow->move(m_scroll->x() + m_scroll->width() - 44 - 8, ay);
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

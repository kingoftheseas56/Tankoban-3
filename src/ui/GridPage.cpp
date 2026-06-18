// Tankoban 3 — GridPage. See GridPage.h.

#include "ui/GridPage.h"

#include "ui/FlowLayout.h"
#include "ui/Icons.h"
#include "ui/PosterCard.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QTimer>
#include <QVBoxLayout>

namespace tankoban {

GridPage::GridPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ContentPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("HomeScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outer->addWidget(m_scroll);

    m_content = new QWidget(m_scroll);
    m_content->setObjectName(QStringLiteral("ContentPage"));
    m_content->setAttribute(Qt::WA_StyledBackground, true);

    auto* col = new QVBoxLayout(m_content);
    col->setContentsMargins(48, 28, 48, 48);
    col->setSpacing(28);

    // Header: round back button + big title + "{N} titles" count (Harbor grid.tsx).
    auto* header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(16);

    auto* back = new QPushButton(m_content);
    back->setObjectName(QStringLiteral("GridBack"));
    back->setCursor(Qt::PointingHandCursor);
    back->setFixedSize(40, 40);
    back->setIcon(navIcon(QStringLiteral("chev-left"), QColor(QStringLiteral("#c9ced6")), 18));
    back->setIconSize(QSize(18, 18));
    back->setStyleSheet(QStringLiteral(
        "#GridBack{border:none;border-radius:20px;background:rgba(255,255,255,0.06);}"
        "#GridBack:hover{background:rgba(255,255,255,0.12);}"));
    connect(back, &QPushButton::clicked, this, &GridPage::backRequested);
    header->addWidget(back);

    m_title = new QLabel(m_content);
    m_title->setObjectName(QStringLiteral("GridTitle"));
    m_title->setStyleSheet(QStringLiteral(
        "#GridTitle{font-size:28px;font-weight:600;color:#f3f1ea;letter-spacing:-0.5px;}"));
    header->addWidget(m_title);

    m_count = new QLabel(m_content);
    m_count->setObjectName(QStringLiteral("GridCount"));
    m_count->setStyleSheet(QStringLiteral("#GridCount{font-size:14px;color:#8b8f99;}"));
    header->addWidget(m_count);
    header->addStretch();
    col->addLayout(header);

    // Responsive poster grid (Harbor minmax(150px) / gap-x 16 / gap-y 32).
    m_grid = new QWidget(m_content);
    m_grid->setObjectName(QStringLiteral("GridTrack"));
    m_flow = new FlowLayout(m_grid, /*margin=*/0, /*hSpacing=*/16, /*vSpacing=*/32);
    col->addWidget(m_grid);

    // Empty state (Harbor grid.tsx: "Nothing here yet." when done and empty).
    m_empty = new QLabel(QStringLiteral("Nothing here yet."), m_content);
    m_empty->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_empty->setStyleSheet(QStringLiteral("color:#8b8f99;font-size:14px;padding:80px 0;"));
    m_empty->hide();
    col->addWidget(m_empty);

    col->addStretch(1);

    m_scroll->setWidget(m_content);

    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() { updateVisible(); });
}

void GridPage::setCatalog(const QString& title, const QVector<MetaItem>& items)
{
    m_title->setText(title);
    m_count->setText(QString::number(items.size())
                     + (items.size() == 1 ? QStringLiteral(" title") : QStringLiteral(" titles")));
    if (m_empty)
        m_empty->setVisible(items.isEmpty());

    clearCards();
    m_cards.reserve(items.size());
    for (const MetaItem& m : items) {
        auto* card = new PosterCard(m, m_grid);
        connect(card, &PosterCard::activated, this, &GridPage::openDetailRequested);
        m_flow->addWidget(card);
        m_cards.push_back(card);
    }
    // Scroll back to the top for the new category, then lazy-load the first viewport.
    m_scroll->verticalScrollBar()->setValue(0);
    QTimer::singleShot(0, this, [this]() { updateVisible(); });
}

void GridPage::clearCards()
{
    QLayoutItem* item = nullptr;
    while ((item = m_flow->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    m_cards.clear();
}

void GridPage::updateVisible()
{
    if (m_cards.isEmpty())
        return;
    const int sy = m_scroll->verticalScrollBar()->value();
    const int vh = m_scroll->viewport()->height();
    const int margin = 600;
    const int lo = sy - margin;
    const int hi = sy + vh + margin;
    for (PosterCard* c : m_cards) {
        const int cy = c->mapTo(m_content, QPoint(0, 0)).y();
        if (cy + c->height() >= lo && cy <= hi)
            c->ensureLoaded();
    }
}

void GridPage::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    QTimer::singleShot(0, this, [this]() { updateVisible(); });
}

void GridPage::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    updateVisible();
}

} // namespace tankoban

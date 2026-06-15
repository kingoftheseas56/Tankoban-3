// Tankoban 3 — CatalogRow (Step 3 / lazy). See CatalogRow.h.

#include "ui/CatalogRow.h"

#include "ui/PosterCard.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
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
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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

    // Re-evaluate which cards are visible on scroll + when the track's size is known.
    connect(m_scroll->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() { updateVisible(); });
    connect(m_scroll->horizontalScrollBar(), &QScrollBar::rangeChanged, this,
            [this]() { updateVisible(); });
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
    QTimer::singleShot(0, this, [this]() { updateVisible(); });
}

void CatalogRow::updateVisible()
{
    if (!m_scroll || m_cards.isEmpty())
        return;
    // Wait until the row is laid out — before layout every card reports x()==0, and
    // we'd fetch all of them, defeating the lazy-load.
    if (m_cards.size() > 1 && m_cards.last()->x() == 0)
        return;
    const int sx = m_scroll->horizontalScrollBar()->value();
    const int vw = m_scroll->viewport()->width();
    const int margin = 400; // preload a little ahead of the viewport
    const int lo = sx - margin;
    const int hi = sx + vw + margin;
    for (PosterCard* c : m_cards) {
        const int cx = c->x();
        if (cx + c->width() >= lo && cx <= hi)
            c->ensureLoaded();
    }
}

void CatalogRow::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    QTimer::singleShot(0, this, [this]() { updateVisible(); });
}

void CatalogRow::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    updateVisible();
}

} // namespace tankoban

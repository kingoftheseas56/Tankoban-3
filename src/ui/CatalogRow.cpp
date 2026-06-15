// Tankoban 3 — CatalogRow (Step 3). See CatalogRow.h.

#include "ui/CatalogRow.h"

#include "ui/PosterCard.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
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

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("RowScroll"));
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setFixedHeight(PosterCard::kPosterH + 44);

    m_track = new QWidget(scroll);
    m_track->setObjectName(QStringLiteral("RowTrack"));
    m_trackLayout = new QHBoxLayout(m_track);
    m_trackLayout->setContentsMargins(0, 0, 0, 0);
    m_trackLayout->setSpacing(16);
    m_trackLayout->addStretch();

    scroll->setWidget(m_track);
    col->addWidget(scroll);
}

void CatalogRow::setStatus(const QString& text)
{
    m_status->setText(text);
    m_status->setVisible(!text.isEmpty());
}

void CatalogRow::setItems(const QVector<MetaItem>& items)
{
    setStatus(items.isEmpty() ? QStringLiteral("No results") : QString());
    // Cap the initial render (long rows of dozens of covers were the slowness);
    // proper load-on-scroll + a "See all" page come later.
    const int cap = qMin(items.size(), 20);
    for (int i = 0; i < cap; ++i) {
        auto* card = new PosterCard(items.at(i), m_track);
        m_trackLayout->insertWidget(m_trackLayout->count() - 1, card); // before stretch
    }
}

} // namespace tankoban

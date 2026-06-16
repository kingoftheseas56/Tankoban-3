// Tankoban 3 — DetailPage (Detail path). See DetailPage.h.

#include "ui/DetailPage.h"

#include "core/MetaService.h"
#include "ui/DetailHero.h"
#include "ui/EpisodeList.h"

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

namespace tankoban {

DetailPage::DetailPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("DetailPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("DetailScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outer->addWidget(m_scroll);

    auto* page = new QWidget(m_scroll);
    page->setObjectName(QStringLiteral("DetailPageBody"));
    page->setAttribute(Qt::WA_StyledBackground, true);
    auto* col = new QVBoxLayout(page);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    m_hero = new DetailHero(page);
    col->addWidget(m_hero);

    auto* body = new QWidget(page);
    body->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* bodyCol = new QVBoxLayout(body);
    bodyCol->setContentsMargins(48, 28, 48, 48);
    bodyCol->setSpacing(40);

    m_synopsis = new QLabel(body);
    m_synopsis->setObjectName(QStringLiteral("DetailSynopsis"));
    m_synopsis->setWordWrap(true);
    m_synopsis->setMaximumWidth(820);
    bodyCol->addWidget(m_synopsis);

    m_episodes = new EpisodeList(body);
    bodyCol->addWidget(m_episodes);
    bodyCol->addStretch();

    col->addWidget(body);
    m_scroll->setWidget(page);

    // Floating Back button (over the hero, fixed top-left).
    m_back = new QPushButton(QStringLiteral("←"), this);
    m_back->setObjectName(QStringLiteral("DetailBack"));
    m_back->setCursor(Qt::PointingHandCursor);
    m_back->setFixedSize(40, 40);

    connect(m_back, &QPushButton::clicked, this, [this]() { emit backRequested(); });
    connect(m_hero, &DetailHero::playClicked, this,
            [this]() { emit playRequested(m_current); });
    connect(m_episodes, &EpisodeList::episodeActivated, this,
            [this](const EpisodeItem& ep) { emit episodeRequested(m_current, ep); });

    m_meta = new MetaService(this);
    connect(m_meta, &MetaService::detailReady, this,
            [this](const QString& id, const MetaDetail& d) {
                if (id != m_current.id)
                    return; // a newer Detail superseded this load
                m_hero->applyDetail(d);
                if (!d.description.isEmpty()) {
                    m_synopsis->setText(d.description);
                    m_synopsis->setVisible(true);
                }
                const bool hasEpisodes =
                    d.type == QLatin1String("series") && !d.videos.isEmpty();
                if (hasEpisodes)
                    m_episodes->setEpisodes(d.videos);
                m_episodes->setVisible(hasEpisodes);
            });
}

void DetailPage::load(const MetaItem& preview)
{
    m_current = preview;
    m_hero->setPreview(preview);
    m_synopsis->setText(preview.description);
    m_synopsis->setVisible(!preview.description.isEmpty());
    m_episodes->setVisible(false); // revealed when /meta returns episodes
    m_scroll->verticalScrollBar()->setValue(0);
    m_meta->fetchDetail(preview.type, preview.id);
}

void DetailPage::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (m_back) {
        m_back->move(24, 18);
        m_back->raise();
    }
}

} // namespace tankoban

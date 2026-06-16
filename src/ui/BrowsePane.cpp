// Tankoban 3 — BrowsePane (Addons marketplace). See BrowsePane.h.

#include "ui/BrowsePane.h"

#include "core/AddonRegistry.h"
#include "core/StremioAddonsProvider.h"
#include "ui/AddonCard.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

namespace tankoban {

namespace {
constexpr int kGap = 16;

struct ModeDef {
    const char* id;
    const char* label;
};
const ModeDef kModes[] = {
    {"top", "Top rated"},
    {"rising", "Top rising"},
    {"new", "Just added"},
};
} // namespace

BrowsePane::BrowsePane(StremioAddonsProvider* provider, AddonRegistry* registry, QWidget* parent)
    : QWidget(parent)
    , m_provider(provider)
    , m_registry(registry)
{
    setObjectName(QStringLiteral("ContentPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(12);

    // Mode chips.
    m_modeRow = new QHBoxLayout();
    m_modeRow->setSpacing(8);
    for (const ModeDef& m : kModes) {
        auto* b = new QPushButton(tr(m.label), this);
        b->setObjectName(QStringLiteral("StoreChip"));
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        const QString id = QString::fromLatin1(m.id);
        connect(b, &QPushButton::clicked, this, [this, id]() { setMode(id); });
        m_modeBtns.push_back(b);
        m_modeRow->addWidget(b);
    }
    m_modeRow->addStretch();
    col->addLayout(m_modeRow);

    // Category chips (horizontal scroll).
    auto* catScroll = new QScrollArea(this);
    catScroll->setObjectName(QStringLiteral("StoreCatScroll"));
    catScroll->setWidgetResizable(true);
    catScroll->setFrameShape(QFrame::NoFrame);
    catScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    catScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    catScroll->setFixedHeight(48);
    m_catRow = new QWidget(catScroll);
    m_catRow->setObjectName(QStringLiteral("ContentPage"));
    m_catRow->setAttribute(Qt::WA_StyledBackground, true);
    m_catLayout = new QHBoxLayout(m_catRow);
    m_catLayout->setContentsMargins(0, 4, 0, 4);
    m_catLayout->setSpacing(8);
    m_catLayout->addStretch();
    catScroll->setWidget(m_catRow);
    col->addWidget(catScroll);

    // Grid (vertical scroll).
    auto* gridScroll = new QScrollArea(this);
    gridScroll->setObjectName(QStringLiteral("AddonScroll"));
    gridScroll->setWidgetResizable(true);
    gridScroll->setFrameShape(QFrame::NoFrame);
    gridScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* gridHost = new QWidget(gridScroll);
    gridHost->setObjectName(QStringLiteral("ContentPage"));
    gridHost->setAttribute(Qt::WA_StyledBackground, true);
    auto* hostCol = new QVBoxLayout(gridHost);
    hostCol->setContentsMargins(0, 4, 0, 8);
    hostCol->setSpacing(14);

    auto* gridContainer = new QWidget(gridHost);
    gridContainer->setAttribute(Qt::WA_TranslucentBackground, true);
    m_grid = new QGridLayout(gridContainer);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(kGap);
    m_grid->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    hostCol->addWidget(gridContainer);

    m_status = new QLabel(tr("Loading the marketplace…"), gridHost);
    m_status->setObjectName(QStringLiteral("AddonPanePlaceholder"));
    m_status->setAlignment(Qt::AlignCenter);
    hostCol->addWidget(m_status);

    m_loadMore = new QPushButton(tr("Load more"), gridHost);
    m_loadMore->setObjectName(QStringLiteral("StoreLoadMore"));
    m_loadMore->setCursor(Qt::PointingHandCursor);
    m_loadMore->setFixedHeight(40);
    m_loadMore->setVisible(false);
    connect(m_loadMore, &QPushButton::clicked, this, [this]() { loadMore(); });
    hostCol->addWidget(m_loadMore, 0, Qt::AlignHCenter);
    hostCol->addStretch();

    gridScroll->setWidget(gridHost);
    col->addWidget(gridScroll, 1);

    connect(m_provider, &StremioAddonsProvider::listReady, this,
            [this](const QString& key, const StorePage& page) {
                if (key != m_reqKey)
                    return;
                m_loading = false;
                m_totalPages = page.totalPages;
                if (page.page <= 1) {
                    for (AddonCard* c : m_cards)
                        c->deleteLater();
                    m_cards.clear();
                }
                m_page = page.page;
                addCards(page.addons);
                m_status->setVisible(m_cards.isEmpty());
                if (m_cards.isEmpty())
                    m_status->setText(tr("No addons found."));
                m_loadMore->setVisible(page.hasNext);
            });
    connect(m_provider, &StremioAddonsProvider::listFailed, this,
            [this](const QString& key, const QString&) {
                if (key != m_reqKey)
                    return;
                m_loading = false;
                if (m_cards.isEmpty()) {
                    m_status->setVisible(true);
                    m_status->setText(tr("Couldn't reach the marketplace. Check your connection."));
                }
            });
    connect(m_provider, &StremioAddonsProvider::categoriesReady, this,
            [this](const QVector<StoreCategory>& cats) {
                // build chips: All + each category
                auto addChip = [this](const QString& label, const QString& slug, bool active) {
                    auto* b = new QPushButton(label, m_catRow);
                    b->setObjectName(QStringLiteral("StoreChip"));
                    b->setProperty("catslug", slug);
                    b->setCheckable(true);
                    b->setChecked(active);
                    b->setCursor(Qt::PointingHandCursor);
                    connect(b, &QPushButton::clicked, this,
                            [this, slug, label]() { setCategory(slug, label); });
                    m_catLayout->insertWidget(m_catLayout->count() - 1, b);
                };
                addChip(tr("All"), QString(), true);
                for (const StoreCategory& c : cats)
                    if (m_includeAdult || c.slug != QLatin1String("nsfw"))
                        addChip(c.name, c.slug, false);
            });

    m_modeBtns.first()->setChecked(true);
}

void BrowsePane::primeIfEmpty()
{
    if (m_primed)
        return;
    m_primed = true;
    m_provider->fetchCategories();
    reload();
}

void BrowsePane::setSearch(const QString& q)
{
    if (m_search == q)
        return;
    m_search = q;
    if (m_primed)
        reload();
}

void BrowsePane::setIncludeAdult(bool adult)
{
    if (m_includeAdult == adult)
        return;
    m_includeAdult = adult;
    if (m_primed)
        reload();
}

void BrowsePane::setMode(const QString& mode)
{
    m_mode = mode;
    for (int i = 0; i < m_modeBtns.size(); ++i)
        m_modeBtns[i]->setChecked(QString::fromLatin1(kModes[i].id) == mode);
    reload();
}

void BrowsePane::setCategory(const QString& slug, const QString&)
{
    m_category = slug;
    for (QObject* o : m_catRow->children())
        if (auto* b = qobject_cast<QPushButton*>(o))
            b->setChecked(b->property("catslug").toString() == slug);
    reload();
}

void BrowsePane::reload()
{
    m_reqKey = m_mode + QStringLiteral("|") + m_category + QStringLiteral("|") + m_search;
    m_page = 1;
    m_loading = true;
    m_status->setVisible(true);
    m_status->setText(tr("Loading the marketplace…"));
    m_loadMore->setVisible(false);
    m_provider->list(m_reqKey, m_mode, m_category, m_search, 1, m_includeAdult);
}

void BrowsePane::loadMore()
{
    if (m_loading || m_page >= m_totalPages)
        return;
    m_loading = true;
    m_provider->list(m_reqKey, m_mode, m_category, m_search, m_page + 1, m_includeAdult);
}

void BrowsePane::addCards(const QVector<StoreAddon>& addons)
{
    for (const StoreAddon& a : addons) {
        auto* card = new AddonCard(a, false, nullptr);
        const QString url = a.manifestUrl;
        connect(card, &AddonCard::installRequested, this,
                [this](const StoreAddon& sa) { m_registry->installFromUrl(sa.manifestUrl); });
        connect(card, &AddonCard::openRequested, this, &BrowsePane::openAddon);
        m_cards.push_back(card);
    }
    refreshInstalledState();
    relayout();
}

void BrowsePane::refreshInstalledState()
{
    QVector<InstalledAddon> inst = m_registry->installed();
    for (AddonCard* c : m_cards) {
        bool installed = false;
        for (const InstalledAddon& a : inst)
            if (a.manifestUrl == c->addon().manifestUrl) {
                installed = true;
                break;
            }
        c->setInstalled(installed);
    }
}

void BrowsePane::relayout()
{
    const int w = width() > 0 ? width() : 960;
    const int cols = qMax(1, (w + kGap) / (AddonCard::kCardW + kGap));
    // detach all, re-add in order
    for (AddonCard* c : m_cards)
        m_grid->removeWidget(c);
    for (int i = 0; i < m_cards.size(); ++i)
        m_grid->addWidget(m_cards[i], i / cols, i % cols, Qt::AlignLeft | Qt::AlignTop);
}

void BrowsePane::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    relayout();
}

} // namespace tankoban

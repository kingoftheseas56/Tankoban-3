// Tankoban 3 — HomePage (Step 3 + 3b). See HomePage.h.

#include "ui/HomePage.h"

#include "core/AddonClient.h"
#include "ui/CatalogRow.h"
#include "ui/FeaturedHero.h"

#include <QDebug>
#include <QFrame>
#include <QScrollArea>
#include <QSet>
#include <QVBoxLayout>
#include <QWidget>

namespace tankoban {

namespace {
const QString kCinemeta = QStringLiteral("https://v3-cinemeta.strem.io");
} // namespace

HomePage::HomePage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ContentPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("HomeScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* page = new QWidget(scroll);
    page->setObjectName(QStringLiteral("ContentPage"));
    page->setAttribute(Qt::WA_StyledBackground, true);

    auto* col = new QVBoxLayout(page);
    col->setContentsMargins(40, 32, 40, 32);
    col->setSpacing(34);

    m_hero = new FeaturedHero(page);
    m_popularMovies = new CatalogRow(QStringLiteral("Popular Movies"), page);
    m_popularSeries = new CatalogRow(QStringLiteral("Popular Series"), page);
    col->addWidget(m_hero);
    col->addWidget(m_popularMovies);
    col->addWidget(m_popularSeries);
    col->addStretch();

    scroll->setWidget(page);

    // Hero Play/click and any poster click both ask MainWindow to open Detail.
    connect(m_hero, &FeaturedHero::openDetailRequested, this, &HomePage::openDetailRequested);
    connect(m_popularMovies, &CatalogRow::activated, this, &HomePage::openDetailRequested);
    connect(m_popularSeries, &CatalogRow::activated, this, &HomePage::openDetailRequested);

    // "View all" → full-category grid (Harbor onViewAll), carrying the full fetched list.
    m_popularMovies->setViewAllVisible(true);
    m_popularSeries->setViewAllVisible(true);
    connect(m_popularMovies, &CatalogRow::viewAllRequested, this,
            [this]() { emit openGridRequested(QStringLiteral("Popular Movies"), m_movieItems); });
    connect(m_popularSeries, &CatalogRow::viewAllRequested, this,
            [this]() { emit openGridRequested(QStringLiteral("Popular Series"), m_seriesItems); });

    m_addons = new AddonClient(this);
    connect(m_addons, &AddonClient::catalogReady, this,
            [this](const QString& key, const QVector<MetaItem>& items) {
                if (key == QLatin1String("movies")) {
                    m_movieItems = items;
                    m_popularMovies->setItems(items);
                } else if (key == QLatin1String("series")) {
                    m_seriesItems = items;
                    m_popularSeries->setItems(items);
                }
                buildHero();
            });
    connect(m_addons, &AddonClient::catalogFailed, this,
            [this](const QString& key, const QString& err) {
                if (key == QLatin1String("movies"))
                    m_popularMovies->setStatus(QStringLiteral("Failed: ") + err);
                else if (key == QLatin1String("series"))
                    m_popularSeries->setStatus(QStringLiteral("Failed: ") + err);
            });

    m_addons->fetchCatalog(QStringLiteral("movies"), kCinemeta,
                           QStringLiteral("movie"), QStringLiteral("top"));
    m_addons->fetchCatalog(QStringLiteral("series"), kCinemeta,
                           QStringLiteral("series"), QStringLiteral("top"));
}

void HomePage::buildHero()
{
    // Harbor's hero: a few varied top titles, deduped by id, capped at 4, ranked by
    // type ("#N in Movies/TV Today"). Harbor draws the #1 of several genre catalogs;
    // we interleave top-movies + top-series until the genre rows land (then we match
    // Harbor's exact pool). Backdrop is required — a hero without wide art isn't one.
    QVector<HeroSlide> slides;
    QSet<QString> seen;
    auto tryAdd = [&](const MetaItem& m) {
        if (slides.size() >= 4 || m.id.isEmpty() || m.background.isEmpty()
            || seen.contains(m.id))
            return;
        seen.insert(m.id);
        HeroSlide s;
        s.meta = m;
        s.position = slides.size() + 1;
        s.rankLabel = (m.type == QLatin1String("series")) ? QStringLiteral("TV")
                                                          : QStringLiteral("Movies");
        slides.push_back(s);
    };

    const int maxI = qMax(m_movieItems.size(), m_seriesItems.size());
    for (int i = 0; i < maxI && slides.size() < 4; ++i) {
        if (i < m_movieItems.size())
            tryAdd(m_movieItems.at(i));
        if (i < m_seriesItems.size())
            tryAdd(m_seriesItems.at(i));
    }

    if (!slides.isEmpty())
        m_hero->setSlides(slides);
}

} // namespace tankoban

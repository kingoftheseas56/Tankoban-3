// Tankoban 3 — RowPage. See RowPage.h.

#include "ui/RowPage.h"

#include "core/AddonClient.h"
#include "ui/CatalogRow.h"
#include "ui/FeaturedHero.h"

#include <QFrame>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QVBoxLayout>
#include <QWidget>

namespace tankoban {

RowPage::RowPage(const QString& routeId, QWidget* parent)
    : QWidget(parent)
    , m_routeId(routeId)
{
    setObjectName(QStringLiteral("ContentPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    // Per-route hero shape (Harbor: movies HERO_POOL_TARGET=5, shows=6).
    if (routeId == QLatin1String("shows")) {
        m_heroTarget = 6;
        m_heroType = QStringLiteral("series");
    } else {
        m_heroTarget = 5;
        m_heroType = QStringLiteral("movie");
    }

    // Same scroll/page scaffold as HomePage (identical object names → identical theming).
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
    connect(m_hero, &FeaturedHero::openDetailRequested, this, &RowPage::openDetailRequested);
    col->addWidget(m_hero);

    m_defs = rowsForRoute(routeId);
    m_rowWidgets.reserve(m_defs.size());
    for (const RowDef& def : m_defs) {
        auto* row = new CatalogRow(def.title, page);
        connect(row, &CatalogRow::activated, this, &RowPage::openDetailRequested);
        // Every Cinemeta-backed shelf can open a full-category grid (Harbor onViewAll).
        row->setViewAllVisible(true);
        const QString key = def.key;
        const QString title = def.title;
        connect(row, &CatalogRow::viewAllRequested, this, [this, key, title]() {
            emit openGridRequested(title, m_items.value(key));
        });
        col->addWidget(row);
        m_rowWidgets.push_back(row);
    }
    col->addStretch();

    scroll->setWidget(page);

    // Fire every catalog fetch in parallel (Harbor's Promise.all); render once all land.
    m_addons = new AddonClient(this);
    connect(m_addons, &AddonClient::catalogReady, this,
            [this](const QString& key, const QVector<MetaItem>& items) {
                m_items.insert(key, items);
                if (--m_pending <= 0)
                    render();
            });
    connect(m_addons, &AddonClient::catalogFailed, this,
            [this](const QString& key, const QString&) {
                m_items.insert(key, {}); // a failed shelf resolves to empty (Harbor .catch([]))
                if (--m_pending <= 0)
                    render();
            });

    m_pending = m_defs.size();
    for (const RowDef& def : m_defs)
        m_addons->fetchCatalog(def.key, def.base, def.type, def.catalogId);
}

void RowPage::render()
{
    // Hero pool: the first ("top") shelf's backdrop-bearing titles, capped per route —
    // Harbor: top.filter(m => m.background).slice(0, HERO_POOL_TARGET).
    QVector<HeroSlide> slides;
    QSet<QString> seen; // dedup set, seeded with the hero ids (Harbor restRows seed)
    if (!m_defs.isEmpty()) {
        const QVector<MetaItem>& top = m_items.value(m_defs.first().key);
        for (const MetaItem& m : top) {
            if (slides.size() >= m_heroTarget)
                break;
            if (m.id.isEmpty() || m.background.isEmpty() || seen.contains(m.id))
                continue;
            HeroSlide s;
            s.meta = m;
            s.position = slides.size() + 1;
            s.rankLabel = (m_heroType == QLatin1String("series")) ? QStringLiteral("TV")
                                                                  : QStringLiteral("Movies");
            slides.push_back(s);
            seen.insert(m.id);
        }
    }
    if (!slides.isEmpty())
        m_hero->setSlides(slides);

    // Shelves, deduped in route order against everything shown above (Harbor restRows):
    // cap to 30 first (MAX_PER_ROW), drop items already shown, omit a row left under 4.
    for (int i = 0; i < m_defs.size(); ++i) {
        QVector<MetaItem> items = m_items.value(m_defs.at(i).key);
        if (items.size() > 30)
            items = items.mid(0, 30);
        QVector<MetaItem> kept;
        kept.reserve(items.size());
        for (const MetaItem& m : items) {
            if (m.id.isEmpty() || seen.contains(m.id))
                continue;
            seen.insert(m.id);
            kept.push_back(m);
        }
        if (kept.size() >= 4) {
            m_rowWidgets.at(i)->setItems(kept);
            m_rowWidgets.at(i)->show();
        } else {
            m_rowWidgets.at(i)->hide();
        }
    }
}

QVector<RowPage::RowDef> RowPage::rowsForRoute(const QString& routeId)
{
    const QString kCinemeta = QStringLiteral("https://v3-cinemeta.strem.io");
    QVector<RowDef> defs;

    // Harbor's Cinemeta no-TMDB branch: a "Top {type}" shelf, then one "Top {Genre}"
    // shelf per genre, in Harbor's exact genre order. The catalog id mirrors Harbor's
    // cinemetaTopPath: "top" and "top/genre={Genre}" (genres here are URL-safe as-is,
    // matching encodeURIComponent), so AddonClient builds the identical Cinemeta URL.
    auto cinemetaSet = [&](const QString& type, const QString& topTitle,
                           const QStringList& genres) {
        defs.push_back({QStringLiteral("cinemeta-top"), topTitle, kCinemeta, type,
                        QStringLiteral("top")});
        for (const QString& g : genres) {
            QString slug = g.toLower();
            slug.remove(QRegularExpression(QStringLiteral("[^a-z]")));
            defs.push_back({QStringLiteral("cinemeta-genre-") + slug,
                            QStringLiteral("Top ") + g, kCinemeta, type,
                            QStringLiteral("top/genre=") + g});
        }
    };

    if (routeId == QLatin1String("movies")) {
        cinemetaSet(QStringLiteral("movie"), QStringLiteral("Top Movies"),
                    {QStringLiteral("Action"), QStringLiteral("Drama"), QStringLiteral("Comedy"),
                     QStringLiteral("Sci-Fi"), QStringLiteral("Thriller"), QStringLiteral("Horror"),
                     QStringLiteral("Romance"), QStringLiteral("Animation"),
                     QStringLiteral("Adventure"), QStringLiteral("Crime"), QStringLiteral("Mystery"),
                     QStringLiteral("Fantasy"), QStringLiteral("Documentary")});
    } else if (routeId == QLatin1String("shows")) {
        cinemetaSet(QStringLiteral("series"), QStringLiteral("Top Series"),
                    {QStringLiteral("Drama"), QStringLiteral("Comedy"), QStringLiteral("Crime"),
                     QStringLiteral("Sci-Fi"), QStringLiteral("Thriller"), QStringLiteral("Mystery"),
                     QStringLiteral("Action"), QStringLiteral("Animation"),
                     QStringLiteral("Adventure"), QStringLiteral("Fantasy"),
                     QStringLiteral("Documentary"), QStringLiteral("Romance"),
                     QStringLiteral("Horror")});
    }
    // anime / discover: built in later increments (different providers).

    return defs;
}

} // namespace tankoban

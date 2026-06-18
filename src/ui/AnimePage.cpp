// Tankoban 3 — AnimePage. See AnimePage.h.

#include "ui/AnimePage.h"

#include "core/JikanClient.h"
#include "ui/AnimeHero.h"
#include "ui/BackToTop.h"
#include "ui/CatalogRow.h"

#include <QFrame>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSet>
#include <QVBoxLayout>
#include <QWidget>

namespace tankoban {

namespace {
// Harbor anime-rows.tsx SPECS, in Harbor's exact order (airing, top-airing, upcoming,
// top-tv, top-movies, popular, all-time, gems, then era/genre rows). The hero seeds from
// whichever hero-key rows have backdrops as they arrive — NOT by reordering them first.
// MAL genre ids: Action 1, Romance 22, Slice of Life 36, Mecha 18, Fantasy 10, Sci-Fi 24,
// Psychological 40, Horror 14.
const char* const kGenre = "/anime?genres=%1&order_by=score&sort=desc&min_score=7&sfw=true&page=1";
const char* const kEra =
    "/anime?start_date=%1-01-01&end_date=%2-12-31&order_by=score&sort=desc&min_score=7.5&sfw=true&page=1";
constexpr int kRowMinVisible = 12; // Harbor ROW_MIN_VISIBLE
constexpr int kRowMaxPages = 5;    // Harbor ROW_MAX_PAGES
constexpr int kRowCap = 80;        // Harbor row metas cap (loadMore stops at >= 80)
} // namespace

AnimePage::AnimePage(QWidget* parent)
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
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    m_hero = new AnimeHero(page);
    connect(m_hero, &AnimeHero::openDetailRequested, this, &AnimePage::openDetailRequested);
    col->addWidget(m_hero);

    auto* rowsHost = new QWidget(page);
    rowsHost->setObjectName(QStringLiteral("ContentPage"));
    rowsHost->setAttribute(Qt::WA_StyledBackground, true);
    auto* rowsCol = new QVBoxLayout(rowsHost);
    rowsCol->setContentsMargins(48, 48, 48, 128);
    rowsCol->setSpacing(48);

    // Built-in Jikan rows (hero-keys first). path includes page=1; lazy-more deferred.
    m_defs = {
        {QStringLiteral("airing"), QStringLiteral("Airing Now"),
         QStringLiteral("/seasons/now?page=1"), true},
        {QStringLiteral("top-airing"), QStringLiteral("Top Airing on MAL"),
         QStringLiteral("/top/anime?filter=airing&page=1"), true},
        {QStringLiteral("upcoming"), QStringLiteral("Upcoming Season"),
         QStringLiteral("/seasons/upcoming?page=1"), true},
        {QStringLiteral("top-tv"), QStringLiteral("Top Series on MAL"),
         QStringLiteral("/top/anime?type=tv&page=1"), false},
        {QStringLiteral("top-movies"), QStringLiteral("Top Movies on MAL"),
         QStringLiteral("/top/anime?type=movie&page=1"), false},
        {QStringLiteral("popular"), QStringLiteral("Most Popular on MAL"),
         QStringLiteral("/top/anime?filter=bypopularity&page=1"), true},
        {QStringLiteral("all-time"), QStringLiteral("Top Rated on MAL"),
         QStringLiteral("/top/anime?page=1"), false},
        {QStringLiteral("gems"), QStringLiteral("Hidden Gems on MAL"),
         QStringLiteral("/anime?order_by=members&sort=asc&min_score=7.8&sfw=true&type=tv&page=1"), false},
        {QStringLiteral("era-2020s"), QStringLiteral("2020s Hits"),
         QString::fromLatin1(kEra).arg(QStringLiteral("2020"), QStringLiteral("2029")), false},
        {QStringLiteral("era-2010s"), QStringLiteral("2010s Classics"),
         QString::fromLatin1(kEra).arg(QStringLiteral("2010"), QStringLiteral("2019")), false},
        {QStringLiteral("era-2000s"), QStringLiteral("2000s Era"),
         QString::fromLatin1(kEra).arg(QStringLiteral("2000"), QStringLiteral("2009")), false},
        {QStringLiteral("era-1990s"), QStringLiteral("Foundation Years (90s)"),
         QString::fromLatin1(kEra).arg(QStringLiteral("1990"), QStringLiteral("1999")), false},
        {QStringLiteral("genre-action"), QStringLiteral("Action & Adventure"),
         QString::fromLatin1(kGenre).arg(1), false},
        {QStringLiteral("genre-romance"), QStringLiteral("Romance"),
         QString::fromLatin1(kGenre).arg(22), false},
        {QStringLiteral("genre-slice"), QStringLiteral("Slice of Life"),
         QString::fromLatin1(kGenre).arg(36), false},
        {QStringLiteral("genre-mecha"), QStringLiteral("Mecha"),
         QString::fromLatin1(kGenre).arg(18), false},
        {QStringLiteral("genre-fantasy"), QStringLiteral("Fantasy"),
         QString::fromLatin1(kGenre).arg(10), false},
        {QStringLiteral("genre-scifi"), QStringLiteral("Sci-Fi"),
         QString::fromLatin1(kGenre).arg(24), false},
        {QStringLiteral("genre-psych"), QStringLiteral("Psychological"),
         QString::fromLatin1(kGenre).arg(40), false},
        {QStringLiteral("genre-horror"), QStringLiteral("Horror & Supernatural"),
         QString::fromLatin1(kGenre).arg(14), false},
    };

    m_rowWidgets.reserve(m_defs.size());
    for (const RowDef& def : m_defs) {
        auto* row = new CatalogRow(def.title, rowsHost);
        connect(row, &CatalogRow::activated, this, &AnimePage::openDetailRequested);
        row->setViewAllVisible(true);
        const QString key = def.key;
        const QString title = def.title;
        connect(row, &CatalogRow::viewAllRequested, this,
                [this, key, title]() { emit openGridRequested(title, m_items.value(key)); });
        connect(row, &CatalogRow::endReached, this, [this, key]() { loadMore(key); });
        row->hide(); // hidden until its first data lands (no visible "Loading…" shelves)
        rowsCol->addWidget(row);
        m_rowWidgets.push_back(row);
    }
    rowsCol->addStretch();
    col->addWidget(rowsHost);

    scroll->setWidget(page);

    m_jikan = new JikanClient(this);
    connect(m_jikan, &JikanClient::rowReady, this, &AnimePage::onRow);
    connect(m_jikan, &JikanClient::rowFailed, this, [this](const QString& key, const QString&) {
        if (m_rowLoading.remove(key))
            return; // a lazy-more page failed — keep the existing shelf content
        for (int i = 0; i < m_defs.size(); ++i)
            if (m_defs.at(i).key == key)
                m_rowWidgets.at(i)->hide();
    });

    for (const RowDef& def : m_defs) {
        if (def.key == QLatin1String("gems"))
            m_jikan->fetchGems(def.key); // Harbor's 2-page underrated-gems fetch + filter
        else
            m_jikan->fetchRow(def.key, def.path);
    }

    new BackToTop(scroll, this); // Harbor anime.tsx renders <BackToTop> for the route
}

void AnimePage::onRow(const QString& key, const QVector<MetaItem>& items)
{
    int idx = -1;
    for (int i = 0; i < m_defs.size(); ++i)
        if (m_defs.at(i).key == key) {
            idx = i;
            break;
        }
    if (idx < 0)
        return;

    // Lazy-more page: append just the fresh (deduped, capped) items to the existing shelf.
    if (m_rowLoading.remove(key)) {
        QVector<MetaItem>& cur = m_items[key];
        QSet<QString> ids;
        for (const MetaItem& m : cur)
            ids.insert(m.id);
        QVector<MetaItem> fresh;
        for (const MetaItem& m : items) {
            if (cur.size() + fresh.size() >= kRowCap)
                break;
            if (m.id.isEmpty() || ids.contains(m.id))
                continue;
            fresh.push_back(m);
            ids.insert(m.id);
        }
        cur += fresh;
        if (!fresh.isEmpty())
            m_rowWidgets.at(idx)->appendItems(fresh);
        m_rowPage[key] = m_rowPage.value(key, 1) + 1;
        m_rowHasMore[key] = items.size() >= kRowMinVisible && cur.size() < kRowCap;
        return;
    }

    // First load.
    m_items.insert(key, items);
    m_rowPage[key] = 1;
    m_rowHasMore[key] = (key != QLatin1String("gems")) && items.size() >= kRowMinVisible;

    // Populate the shelf. Harbor skips top-airing (TOP_PICKS_KEY) from normal row render —
    // it's fetched only to seed the hero — and hides a ready row only when it's empty.
    if (key != QLatin1String("top-airing")) {
        if (!items.isEmpty()) {
            QVector<MetaItem> kept = items.size() > 30 ? items.mid(0, 30) : items;
            m_rowWidgets.at(idx)->setItems(kept);
            m_rowWidgets.at(idx)->show();
        } else {
            m_rowWidgets.at(idx)->hide();
        }
    }

    // Hero pool: first ~6 backdrop-bearing titles from the hero-key rows (which load first).
    if (m_defs.at(idx).hero && !m_heroFull) {
        QSet<QString> ids;
        for (const MetaItem& m : m_heroPool)
            ids.insert(m.id);
        for (const MetaItem& m : items) {
            if (m_heroPool.size() >= 6)
                break;
            if (m.id.isEmpty() || m.background.isEmpty() || ids.contains(m.id))
                continue;
            m_heroPool.push_back(m);
            ids.insert(m.id);
        }
        if (!m_heroPool.isEmpty() && m_hero)
            m_hero->setSlides(m_heroPool);
        if (m_heroPool.size() >= 6)
            m_heroFull = true;
    }
}

void AnimePage::loadMore(const QString& key)
{
    if (key == QLatin1String("gems"))
        return; // gems uses a special multi-page fetcher; its own paging is deferred
    if (m_rowLoading.contains(key) || !m_rowHasMore.value(key, false))
        return;
    const int page = m_rowPage.value(key, 1);
    if (page >= kRowMaxPages || m_items.value(key).size() >= kRowCap)
        return;
    int idx = -1;
    for (int i = 0; i < m_defs.size(); ++i)
        if (m_defs.at(i).key == key) {
            idx = i;
            break;
        }
    if (idx < 0)
        return;
    QString path = m_defs.at(idx).path;
    path.replace(QStringLiteral("page=1"), QStringLiteral("page=") + QString::number(page + 1));
    m_rowLoading.insert(key);
    m_jikan->fetchRow(key, path);
}

void AnimePage::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (m_hero)
        m_hero->setFixedHeight(qMax(560, int(height() * 0.72))); // Harbor anime hero (min-h-520)
}

} // namespace tankoban

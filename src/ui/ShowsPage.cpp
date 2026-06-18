// Tankoban 3 — ShowsPage. See ShowsPage.h.

#include "ui/ShowsPage.h"

#include "core/AddonClient.h"
#include "ui/CatalogRow.h"
#include "ui/PeekHero.h"

#include <QDate>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QSet>
#include <QTime>
#include <QVBoxLayout>

namespace tankoban {

namespace {
const QString kCinemeta = QStringLiteral("https://v3-cinemeta.strem.io");
constexpr int kHeroTarget = 6; // Harbor shows HERO_POOL_TARGET

struct Copy {
    const char* kicker;
    const char* title;
    const char* subtitle;
};

// Time-of-day masthead copy, ported 1:1 from harbor-ref src/views/shows/hero-curation.ts
// (BUCKET_VARIANTS + bucketCopy). Pure date/time text — no TMDB dependency.
const Copy kMorning[] = {
    {"Morning Lineup", "Easing into series", "Slow-burn worlds and bright chapters worth opening with coffee."},
    {"Good Morning", "Today's openers", "Series to ease into while the day is still quiet."},
    {"Daybreak", "First-light picks", "Worlds to step into before the inbox catches up."},
    {"AM Picks", "Coffee-and-couch", "Half-hours, anthologies, and a few epics for the morning routine."},
    {"Open the Day", "Series with mileage", "Long-running comforts and new chapters worth pressing play on."},
    {"Quiet Hours", "Slow-burn starts", "Stories that reward your attention before the day gets loud."},
    {"This Morning", "Worth catching up on", "What everyone has been quietly binging this week."},
};
const Copy kAfternoon[] = {
    {"Afternoon Picks", "Daytime watching", "Easy half-hours and lighter dramas to ride out the afternoon."},
    {"Midday Lineup", "Between meetings", "Episodes you can drop into without losing the thread."},
    {"Afternoon Roll", "Pick up an episode", "Lunch-break comedies and slow-cooker dramas, ready when you are."},
    {"The Long Lunch", "Series to disappear into", "Worlds wide enough for an hour or a whole free afternoon."},
    {"Daylight Watching", "Bright-side series", "Sharp comedies, sunny worlds, and the occasional binge bait."},
    {"Holdover Picks", "Carry it through the day", "Companion series for whatever the afternoon throws at you."},
    {"PM Picks", "Couch hours", "Series for the part of the day that runs on coffee and snacks."},
};
const Copy kEvening[] = {
    {"Tonight", "Tonight's lineup", "Prestige drama, weekly chapters, and series worth disappearing into."},
    {"Prime Time", "What to watch tonight", "Crowd-pleasers, prestige picks, and the kind of series people text about."},
    {"Sundown", "Evening on the couch", "Drop-in chapters and long arcs for the post-dinner stretch."},
    {"Press Play", "Tonight's marquee", "The series that make the rest of the night disappear."},
    {"Tonight's Slate", "Episodes worth the evening", "What's hot this week, what's prestige forever, what's worth the hours."},
    {"Showtime", "Tonight's main event", "Series for the part of the day you actually look forward to."},
    {"Saved for Now", "Tonight's binge bait", "Pilots that pull you in and finales that earn the season."},
};
const Copy kNight[] = {
    {"Late Night", "After-hours picks", "Dark, immersive, and binge-worthy when the house is quiet."},
    {"Past Midnight", "One more episode", "Series for the part of the night that won't let you sleep."},
    {"Witching Hour", "Late-night chapters", "Pull-you-under stories for the quietest part of the day."},
    {"Lights Out", "Headphone series", "Slow, strange, and absorbing. Best with the lights down low."},
    {"Insomnia Lineup", "Worth the lost hour", "Dense plots and rich worlds for when sleep is not happening."},
    {"Late Show", "After the news", "Quiet dramas, sharp thrillers, and series you save for yourself."},
    {"Night Owl", "While the world's asleep", "Series with the patience to match your late-night hours."},
};

Copy bucketCopy()
{
    const int h = QTime::currentTime().hour();
    int bucketIdx;
    const Copy* variants;
    if (h >= 5 && h < 12) { variants = kMorning; bucketIdx = 0; }
    else if (h >= 12 && h < 17) { variants = kAfternoon; bucketIdx = 1; }
    else if (h >= 17 && h < 22) { variants = kEvening; bucketIdx = 2; }
    else { variants = kNight; bucketIdx = 3; }
    const int doy = QDate::currentDate().dayOfYear();
    const int idx = ((doy + bucketIdx * 3) % 7 + 7) % 7; // 7 variants per bucket
    return variants[idx];
}
} // namespace

ShowsPage::ShowsPage(QWidget* parent)
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
    // Harbor shows.tsx content column: px-12 (48) / pt-32 (~big top air) / gap-12 (48).
    // The generous top air + 48px gaps are core to the calm "gallery" rhythm.
    col->setContentsMargins(48, 96, 48, 48);
    col->setSpacing(48);

    // PageMast (Harbor shows.tsx PageMast): kicker + display title + subtitle.
    const Copy copy = bucketCopy();
    auto* mast = new QWidget(page);
    mast->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* mastCol = new QVBoxLayout(mast);
    mastCol->setContentsMargins(0, 0, 0, 0);
    mastCol->setSpacing(8);

    auto* kicker = new QLabel(QString::fromUtf8(copy.kicker).toUpper(), mast);
    kicker->setStyleSheet(QStringLiteral(
        "font-size:11px;font-weight:700;letter-spacing:4px;color:#8b909b;"));
    mastCol->addWidget(kicker);

    auto* mastTitle = new QLabel(QString::fromUtf8(copy.title), mast);
    mastTitle->setStyleSheet(QStringLiteral(
        "font-family:'Fraunces','Georgia',serif;font-size:44px;font-weight:600;color:#f3f1ea;"));
    mastCol->addWidget(mastTitle);

    auto* mastSub = new QLabel(QString::fromUtf8(copy.subtitle), mast);
    mastSub->setWordWrap(true);
    mastSub->setStyleSheet(QStringLiteral("font-size:15px;color:#aab1bd;"));
    mastCol->addWidget(mastSub);
    col->addWidget(mast);

    // PeekHero (Shows landscape carousel — NOT the Movies billboard).
    m_hero = new PeekHero(page);
    connect(m_hero, &PeekHero::openDetailRequested, this, &ShowsPage::openDetailRequested);
    connect(m_hero, &PeekHero::playRequested, this, &ShowsPage::playRequested);
    col->addWidget(m_hero);

    // Cinemeta series shelves — Harbor's no-key order: Top Series, then genres.
    const QStringList genres = {
        QStringLiteral("Drama"), QStringLiteral("Comedy"), QStringLiteral("Crime"),
        QStringLiteral("Sci-Fi"), QStringLiteral("Thriller"), QStringLiteral("Mystery"),
        QStringLiteral("Action"), QStringLiteral("Animation"), QStringLiteral("Adventure"),
        QStringLiteral("Fantasy"), QStringLiteral("Documentary"), QStringLiteral("Romance"),
        QStringLiteral("Horror")};
    m_defs.push_back({QStringLiteral("cinemeta-top"), QStringLiteral("Top Series"), kCinemeta,
                      QStringLiteral("series"), QStringLiteral("top")});
    for (const QString& g : genres) {
        QString slug = g.toLower();
        slug.remove(QChar('-'));
        m_defs.push_back({QStringLiteral("cinemeta-genre-") + slug, QStringLiteral("Top ") + g,
                          kCinemeta, QStringLiteral("series"), QStringLiteral("top/genre=") + g});
    }

    m_rowWidgets.reserve(m_defs.size());
    for (const RowDef& def : m_defs) {
        auto* row = new CatalogRow(def.title, page);
        connect(row, &CatalogRow::activated, this, &ShowsPage::openDetailRequested);
        row->setViewAllVisible(true);
        const QString key = def.key;
        const QString title = def.title;
        connect(row, &CatalogRow::viewAllRequested, this,
                [this, key, title]() { emit openGridRequested(title, m_items.value(key)); });
        col->addWidget(row);
        m_rowWidgets.push_back(row);
    }
    col->addStretch();

    scroll->setWidget(page);

    m_addons = new AddonClient(this);
    connect(m_addons, &AddonClient::catalogReady, this,
            [this](const QString& key, const QVector<MetaItem>& items) {
                m_items.insert(key, items);
                if (--m_pending <= 0)
                    render();
            });
    connect(m_addons, &AddonClient::catalogFailed, this,
            [this](const QString& key, const QString&) {
                m_items.insert(key, {});
                if (--m_pending <= 0)
                    render();
            });

    m_pending = m_defs.size();
    for (const RowDef& def : m_defs)
        m_addons->fetchCatalog(def.key, def.base, def.type, def.catalogId);
}

void ShowsPage::render()
{
    QSet<QString> seen;

    // Hero: top series with backdrops (Harbor: top.filter(m => m.background).slice(0, 6)).
    QVector<MetaItem> heroSlides;
    if (!m_defs.isEmpty()) {
        const QVector<MetaItem>& top = m_items.value(m_defs.first().key);
        for (const MetaItem& m : top) {
            if (heroSlides.size() >= kHeroTarget)
                break;
            if (m.id.isEmpty() || m.background.isEmpty() || seen.contains(m.id))
                continue;
            heroSlides.push_back(m);
            seen.insert(m.id);
        }
    }
    if (m_hero && !heroSlides.isEmpty())
        m_hero->setSlides(heroSlides);

    // Shelves: dedup against the HERO ONLY (Harbor shows.tsx restRows seeds `seen` with hero
    // ids and does NOT add row items to it), so the same series may recur across genre rows —
    // unlike Movies, which globally de-dupes. Hide rows left under 4.
    for (int i = 0; i < m_defs.size(); ++i) {
        QVector<MetaItem> items = m_items.value(m_defs.at(i).key);
        if (items.size() > 30)
            items = items.mid(0, 30);
        QVector<MetaItem> kept;
        kept.reserve(items.size());
        for (const MetaItem& m : items) {
            if (m.id.isEmpty() || seen.contains(m.id)) // `seen` holds only the hero ids
                continue;
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

} // namespace tankoban

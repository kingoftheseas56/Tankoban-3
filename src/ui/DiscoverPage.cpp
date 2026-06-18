// Tankoban 3 — DiscoverPage. See DiscoverPage.h.

#include "ui/DiscoverPage.h"

#include "core/AddonClient.h"
#include "ui/BackToTop.h"
#include "ui/CatalogRow.h"

#include <QDate>
#include <QFrame>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

namespace tankoban {

namespace {
const QString kCinemeta = QStringLiteral("https://v3-cinemeta.strem.io");
constexpr int kMinRow = 16;        // Harbor use-deduped-rows.ts MIN_ROW (backfill floor)
constexpr int kRowDisplayCap = 30; // horizontal preview cap (RowPage/AnimePage convention)

// Harbor tags.ts dailySeed(): year*10000 + month*100 + day. Stable within a calendar day.
quint32 dailySeed()
{
    const QDate d = QDate::currentDate();
    return quint32(d.year() * 10000 + d.month() * 100 + d.day());
}

// Harbor tags.ts pickRandom(): seeded Fisher-Yates over an LCG, take the first n. The C++
// port uses exact 32-bit unsigned wrap-around arithmetic (Harbor's JS does the multiply in
// lossy float64, so the *specific* daily set can differ — but the behavior is identical:
// a deterministic daily rotation of n picks from the same source set).
QStringList pickRandom(const QStringList& src, int n, quint32 seed)
{
    QStringList copy = src;
    quint32 s = seed;
    for (int i = copy.size() - 1; i > 0; --i) {
        s = (s * 1103515245u + 12345u) & 0x7fffffffu;
        const int j = int(s % quint32(i + 1));
        copy.swapItemsAt(i, j);
    }
    return copy.mid(0, qMin(n, copy.size()));
}
} // namespace

DiscoverPage::DiscoverPage(QWidget* parent)
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

    // Pure row stack — no hero (Harbor's top widget is FeaturedBanner, deferred in v1).
    // Harbor discover.tsx: px-12 / pt-20 / pb-20 (48 / 80 / 80) + gap-14 (56).
    auto* col = new QVBoxLayout(page);
    col->setContentsMargins(48, 80, 48, 80);
    col->setSpacing(56);

    m_defs = buildShelves();
    m_rowWidgets.reserve(m_defs.size());
    for (const RowDef& def : m_defs) {
        auto* row = new CatalogRow(def.title, page);
        connect(row, &CatalogRow::activated, this, &DiscoverPage::openDetailRequested);
        row->setViewAllVisible(true);
        const QString key = def.key;
        const QString title = def.title;
        // View-all shows the full fetched shelf (Harbor's grid re-admits the cross-row
        // dupes the row deduped out, so the grid is the complete category). No-key fetchers
        // ignore the page arg, so this stays static — faithful to Harbor's own no-key path.
        connect(row, &CatalogRow::viewAllRequested, this, [this, key, title]() {
            emit openGridRequested(title, m_items.value(key));
        });
        row->hide(); // hidden until its data lands (no visible "Loading…" shelves)
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

    new BackToTop(scroll, this); // Harbor discover.tsx renders <BackToTop> for the route
}

void DiscoverPage::render()
{
    // Cross-row dedupe in display order (Harbor use-deduped-rows.ts). No featured/criticsPick
    // in v1, so the protected set is empty: claim unique-by-id first, then backfill from the
    // row's own raw items up to MIN_ROW so genre rows overlapping Top Movies don't go thin.
    // (AddonClient already drops empty-poster items, so every item here has a poster.)
    QSet<QString> seen;
    for (int i = 0; i < m_defs.size(); ++i) {
        const QVector<MetaItem>& raw = m_items.value(m_defs.at(i).key);

        QVector<MetaItem> kept;
        QSet<QString> taken;
        kept.reserve(raw.size());
        for (const MetaItem& m : raw) {
            if (m.id.isEmpty() || seen.contains(m.id))
                continue;
            seen.insert(m.id);
            taken.insert(m.id);
            kept.push_back(m);
        }
        if (kept.size() < kMinRow) {
            for (const MetaItem& m : raw) {
                if (kept.size() >= kMinRow)
                    break;
                if (m.id.isEmpty() || taken.contains(m.id))
                    continue;
                taken.insert(m.id);
                kept.push_back(m); // backfill: shared across rows, not re-claimed globally
            }
        }

        if (kept.isEmpty()) {
            m_rowWidgets.at(i)->hide();
        } else {
            QVector<MetaItem> shown = kept.size() > kRowDisplayCap ? kept.mid(0, kRowDisplayCap) : kept;
            m_rowWidgets.at(i)->setItems(shown);
            m_rowWidgets.at(i)->show();
        }
    }
}

QVector<DiscoverPage::RowDef> DiscoverPage::buildShelves()
{
    // Harbor themes.ts fallbackShelves(): Top Movies, Top Series, then 5 daily-seeded genre
    // shelves (all movie/top/genre=<g>) drawn from the same 8-genre set, in pickRandom order.
    QVector<RowDef> defs;
    defs.push_back({QStringLiteral("discover-top-movies"), QStringLiteral("Top Movies"),
                    kCinemeta, QStringLiteral("movie"), QStringLiteral("top")});
    defs.push_back({QStringLiteral("discover-top-series"), QStringLiteral("Top Series"),
                    kCinemeta, QStringLiteral("series"), QStringLiteral("top")});

    const QStringList genres = {QStringLiteral("Drama"),  QStringLiteral("Comedy"),
                                QStringLiteral("Action"), QStringLiteral("Crime"),
                                QStringLiteral("Sci-Fi"), QStringLiteral("Thriller"),
                                QStringLiteral("Horror"), QStringLiteral("Romance")};
    const QStringList picked = pickRandom(genres, 5, dailySeed());
    for (const QString& g : picked) {
        QString slug = g.toLower();
        slug.remove(QRegularExpression(QStringLiteral("[^a-z]")));
        defs.push_back({QStringLiteral("discover-genre-") + slug, QStringLiteral("Top ") + g,
                        kCinemeta, QStringLiteral("movie"),
                        QStringLiteral("top/genre=") + g});
    }
    return defs;
}

} // namespace tankoban

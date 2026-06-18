// Tankoban 3 — RowPage. See RowPage.h.

#include "ui/RowPage.h"

#include "core/AddonClient.h"
#include "ui/CatalogRow.h"
#include "ui/CinemaHero.h"

#include <QFrame>
#include <QRegularExpression>
#include <QResizeEvent>
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

    // page = [ full-bleed CinemaHero ] + [ padded row stack ] (Harbor movies.tsx layout).
    auto* col = new QVBoxLayout(page);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    m_hero = new CinemaHero(page);
    connect(m_hero, &CinemaHero::openDetailRequested, this, &RowPage::openDetailRequested);
    connect(m_hero, &CinemaHero::playRequested, this, &RowPage::playRequested);
    col->addWidget(m_hero);

    auto* rowsHost = new QWidget(page);
    rowsHost->setObjectName(QStringLiteral("ContentPage"));
    rowsHost->setAttribute(Qt::WA_StyledBackground, true);
    auto* rowsCol = new QVBoxLayout(rowsHost);
    rowsCol->setContentsMargins(48, 48, 48, 128); // Harbor px-12 / pt-12 / pb-32
    rowsCol->setSpacing(48);                        // Harbor gap-12

    m_defs = rowsForRoute(routeId);
    m_rowWidgets.reserve(m_defs.size());
    for (const RowDef& def : m_defs) {
        auto* row = new CatalogRow(def.title, rowsHost);
        connect(row, &CatalogRow::activated, this, &RowPage::openDetailRequested);
        row->setViewAllVisible(true);
        const QString key = def.key;
        const QString title = def.title;
        connect(row, &CatalogRow::viewAllRequested, this, [this, key, title]() {
            emit openGridRequested(title, m_items.value(key));
        });
        rowsCol->addWidget(row);
        m_rowWidgets.push_back(row);
    }
    rowsCol->addStretch();
    col->addWidget(rowsHost);

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

void RowPage::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (m_hero)
        m_hero->setFixedHeight(qMax(640, int(height() * 0.78))); // Harbor 78vh / min 640
}

void RowPage::render()
{
    // Hero: top movies with backdrops (Harbor: top.filter(m => m.background).slice(0, 5)).
    QVector<MetaItem> heroSlides;
    QSet<QString> seen;
    if (!m_defs.isEmpty()) {
        const QVector<MetaItem>& top = m_items.value(m_defs.first().key);
        for (const MetaItem& m : top) {
            if (heroSlides.size() >= m_heroTarget)
                break;
            if (m.id.isEmpty() || m.background.isEmpty() || seen.contains(m.id))
                continue;
            heroSlides.push_back(m);
            seen.insert(m.id);
        }
    }
    if (m_hero && !heroSlides.isEmpty())
        m_hero->setSlides(heroSlides);

    // Shelves, deduped in route order against the hero + earlier shelves; hide rows under 4.
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
    }

    return defs;
}

} // namespace tankoban

// Tankoban 3 — HomePage (Step 3 + 3b).
//
// The main page: a featured hero carousel (3b) above the catalogue rows (3), all fed
// by the addon engine from Cinemeta. More rows + the detail page come next.

#pragma once

#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

namespace tankoban {

class AddonClient;
class CatalogRow;
class FeaturedHero;

class HomePage : public QWidget {
    Q_OBJECT
public:
    explicit HomePage(QWidget* parent = nullptr);

private:
    void buildHero(); // (re)build the 4-slide hero pool from the loaded catalogs

    AddonClient* m_addons = nullptr;
    FeaturedHero* m_hero = nullptr;
    CatalogRow* m_popularMovies = nullptr;
    CatalogRow* m_popularSeries = nullptr;

    QVector<MetaItem> m_movieItems;
    QVector<MetaItem> m_seriesItems;
};

} // namespace tankoban

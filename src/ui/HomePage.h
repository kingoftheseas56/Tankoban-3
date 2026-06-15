// Tankoban 3 — HomePage (Step 3).
//
// The main page: catalogue rows fed by the addon engine. Step 3 = Popular Movies +
// Popular Series from Cinemeta, real posters. The featured hero + more rows come next.

#pragma once

#include <QWidget>

namespace tankoban {

class AddonClient;
class CatalogRow;

class HomePage : public QWidget {
    Q_OBJECT
public:
    explicit HomePage(QWidget* parent = nullptr);

private:
    AddonClient* m_addons = nullptr;
    CatalogRow* m_popularMovies = nullptr;
    CatalogRow* m_popularSeries = nullptr;
};

} // namespace tankoban

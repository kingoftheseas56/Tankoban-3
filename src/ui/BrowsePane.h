// Tankoban 3 — BrowsePane (Addons marketplace).
//
// Harbor's Browse tab: the live stremio-addons.net marketplace as a responsive grid of
// AddonCards, with mode chips (Top rated / Top rising / Just added), category chips, the
// page search, and load-more paging.

#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

#include "core/AddonStoreModels.h"

class QGridLayout;
class QHBoxLayout;
class QLabel;
class QPushButton;

namespace tankoban {

class StremioAddonsProvider;
class AddonRegistry;
class AddonCard;

class BrowsePane : public QWidget {
    Q_OBJECT
public:
    BrowsePane(StremioAddonsProvider* provider, AddonRegistry* registry,
               QWidget* parent = nullptr);

    void setSearch(const QString& q); // from the page search box
    void setIncludeAdult(bool adult);
    void primeIfEmpty();              // first load when the tab is shown
    void refreshInstalledState();     // re-mark installed cards after registry changes

signals:
    void openAddon(const StoreAddon& addon);

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    void setMode(const QString& mode);
    void setCategory(const QString& slug, const QString& label);
    void reload();
    void loadMore();
    void relayout();
    void addCards(const QVector<StoreAddon>& addons);

    StremioAddonsProvider* m_provider = nullptr;
    AddonRegistry* m_registry = nullptr;

    QHBoxLayout* m_modeRow = nullptr;
    QWidget* m_catRow = nullptr;
    QHBoxLayout* m_catLayout = nullptr;
    QGridLayout* m_grid = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_loadMore = nullptr;

    QVector<AddonCard*> m_cards;
    QVector<QPushButton*> m_modeBtns;

    QString m_mode = QStringLiteral("top");
    QString m_category;      // slug, empty = all
    QString m_search;
    bool m_includeAdult = false;
    int m_page = 1;
    int m_totalPages = 1;
    bool m_loading = false;
    bool m_primed = false;
    QString m_reqKey;
};

} // namespace tankoban

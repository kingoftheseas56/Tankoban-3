// Tankoban 3 — GridPage ("View all" destination).
//
// The full-category grid a row's "View all" opens, faithful to harbor-ref src/views/grid.tsx:
// a back arrow + big title + "{N} titles" count over a responsive auto-fill poster grid
// (minmax 150px, gap-x 16 / gap-y 32). Reuses PosterCard with the same lazy, viewport-gated
// cover loading as the rows. Pushed as a page in the content stack (sidebar stays visible),
// matching how DetailPage is hosted.

#pragma once

#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

class QLabel;
class QScrollArea;
class QShowEvent;
class QResizeEvent;

namespace tankoban {

class PosterCard;
class FlowLayout;
class JikanClient;

class GridPage : public QWidget {
    Q_OBJECT
public:
    explicit GridPage(QWidget* parent = nullptr);

    // Fill the grid with one category's full title list (replaces any previous contents).
    void setCatalog(const QString& title, const QVector<MetaItem>& items);

    // Like setCatalog, but infinite-scrolls via Harbor's fetcher model: a Jikan path template
    // (with "page=1") + the page already covered by `initial`, so the grid pages on its own
    // (Harbor grid.tsx IntersectionObserver up to PAGE_CAP). Empty template = no paging.
    void setPagedCatalog(const QString& title, const QVector<MetaItem>& initial,
                         const QString& jikanPathTemplate, int startPage);

signals:
    void backRequested();
    void openDetailRequested(const MetaItem& meta);

protected:
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    void clearCards();
    void updateVisible();  // lazy-load covers in/near the vertical viewport
    void appendCards(const QVector<MetaItem>& items);
    void checkLoadMore();  // fetch the next grid page when scrolled near the bottom
    void onGridPage(const QString& key, const QVector<MetaItem>& items);

    QLabel* m_title = nullptr;
    QLabel* m_count = nullptr;
    QLabel* m_empty = nullptr; // "Nothing here yet." (Harbor grid.tsx done+empty)
    QScrollArea* m_scroll = nullptr;
    QWidget* m_content = nullptr; // the scrolled widget (cards are descendants)
    QWidget* m_grid = nullptr;
    FlowLayout* m_flow = nullptr;
    QVector<PosterCard*> m_cards;

    JikanClient* m_jikan = nullptr;     // pages anime View-all (Harbor grid fetcher)
    QString m_pagePath;                  // Jikan path template ("page=1"); empty = no paging
    int m_page = 0;                      // highest page already covered by the grid
    bool m_pageLoading = false;
    bool m_pageDone = true;
    QSet<QString> m_seenIds;             // dedupe across appended pages
    static constexpr int kPageCap = 40;  // Harbor PAGE_CAP
};

} // namespace tankoban

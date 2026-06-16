// Tankoban 3 — DetailPage (Detail path).
//
// The pushed Detail screen, recreated from Harbor's detail.tsx: a full-bleed backdrop
// hero, a synopsis, and (for series) the episode list — with a floating Back button.
// Renders immediately from the clicked preview, then fills in from MetaService (/meta).

#pragma once

#include <QWidget>

#include "core/MetaDetail.h"
#include "core/MetaItem.h"

class QLabel;
class QPushButton;
class QScrollArea;

namespace tankoban {

class MetaService;
class DetailHero;
class EpisodeList;

class DetailPage : public QWidget {
    Q_OBJECT
public:
    explicit DetailPage(QWidget* parent = nullptr);

    void load(const MetaItem& preview);

signals:
    void backRequested();
    void playRequested(const MetaItem& meta);                          // movie / generic
    void episodeRequested(const MetaItem& meta, const EpisodeItem& ep); // series episode

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    MetaItem m_current;
    MetaService* m_meta = nullptr;
    DetailHero* m_hero = nullptr;
    QLabel* m_synopsis = nullptr;
    EpisodeList* m_episodes = nullptr;
    QPushButton* m_back = nullptr;
    QScrollArea* m_scroll = nullptr;
};

} // namespace tankoban

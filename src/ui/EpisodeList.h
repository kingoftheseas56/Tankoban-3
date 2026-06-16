// Tankoban 3 — EpisodeList (Detail path).
//
// The series episode section, recreated from Harbor's detail/cinemeta-episodes.tsx:
// a season selector (custom Harbor-style dropdown) + a vertical list of episode rows
// (landscape still, number, title, "S{s}E{e} · aired"). Clicking a row asks to play
// that episode (Picker, next slice).

#pragma once

#include <QMap>
#include <QVector>
#include <QWidget>

#include "core/MetaDetail.h"

class QLabel;
class QMenu;
class QPushButton;
class QVBoxLayout;

namespace tankoban {

class EpisodeList : public QWidget {
    Q_OBJECT
public:
    explicit EpisodeList(QWidget* parent = nullptr);

    void setEpisodes(const QVector<EpisodeItem>& videos);

signals:
    void episodeActivated(const EpisodeItem& ep);

private:
    void showSeason(int season);

    QMap<int, QVector<EpisodeItem>> m_bySeason;
    QPushButton* m_seasonBtn = nullptr;
    QMenu* m_seasonMenu = nullptr;
    QLabel* m_count = nullptr;
    QWidget* m_rows = nullptr;
    QVBoxLayout* m_rowsLayout = nullptr;
};

} // namespace tankoban

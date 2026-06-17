// Tankoban 3 — VideoPlayerPage (in-app player surface).
//
// A full-rect overlay (sibling of PlayPickerPage) that hosts the player track's
// PlayerView and gives the app shell a clean entry point: play(PlayerRequest).
// Back is host-driven via backRequested() — the shell decides where back returns
// to — so the player never closes the whole app (the old window()->close() trap).
#pragma once

#include <QString>
#include <QWidget>

class PlayerView;          // player track, global namespace
class QResizeEvent;

namespace tankoban {

// What the shell hands the in-app player. Deliberately tiny for the direct-link
// slice; the player track will extend it (http headers, attached subtitles,
// resume position) as the playback spine grows.
struct PlayerRequest {
    QString url;
    QString title;
    QString subtitle;
    double startSec = 0.0;
};

class VideoPlayerPage : public QWidget {
    Q_OBJECT
public:
    explicit VideoPlayerPage(QWidget* parent = nullptr);

    void play(const PlayerRequest& request);
    void stop();   // pause playback when leaving the player (clean teardown)

signals:
    void backRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    PlayerView* m_player = nullptr;
};

} // namespace tankoban

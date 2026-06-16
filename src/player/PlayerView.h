#pragma once
// PlayerView — the fullscreen player stage. Owns the MpvController, hosts the
// MpvGlWidget (video), feeds the PlaybackClock from snapshots, and installs the
// HotkeyDispatcher. Click/double-click are handled by event-filtering the video
// widget directly (no translucent overlay — that amplifies fullscreen flicker).
// No chrome yet (Plan 2). (spec §9.3 fullscreen, §9.5 click.)
#include "engine/PlayerSnapshot.h"
#include <QWidget>

class MpvController;
class MpvGlWidget;
class QTimer;

class PlayerView : public QWidget {
    Q_OBJECT
public:
    explicit PlayerView(QWidget* parent = nullptr);
    void play(const QString& url, double startSec = 0.0);
    MpvController* controller() const { return m_controller; }
    PlayerSnapshot snap() const;

public slots:
    void playPauseToggle();
    void toggleFullscreen();

protected:
    void resizeEvent(QResizeEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;   // clicks on the video

private:
    MpvController* m_controller = nullptr;
    MpvGlWidget* m_video = nullptr;
    QTimer* m_clickTimer = nullptr;
    bool m_swallowRelease = false;
    bool m_fakeFs = false;        // borderless-fullscreen via geometry (no OS transition = no blink)
    QRect m_savedGeom;            // windowed geometry to restore on exit
};

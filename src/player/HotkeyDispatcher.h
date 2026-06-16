#pragma once
// HotkeyDispatcher — app-level key handling for the player (spec §9.1 + §9.1a
// hold-to-speed). Installed as an event filter; gates out typing targets.
#include <QObject>

class PlayerView;
class MpvController;
class QTimer;

class HotkeyDispatcher : public QObject {
    Q_OBJECT
public:
    explicit HotkeyDispatcher(PlayerView* view, QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void seekStep(double delta);
    void seekTo(double sec);
    void changeVolume(double delta);
    void changeSubDelay(double delta);
    void changeSpeed(double delta);
    void cycleSubtitles();
    void onSpaceHold();

    PlayerView* m_view = nullptr;
    MpvController* m_mpv = nullptr;
    QTimer* m_holdTimer = nullptr;
    bool m_spaceDown = false;
    bool m_holdActive = false;
    double m_baseRate = 1.0;
};

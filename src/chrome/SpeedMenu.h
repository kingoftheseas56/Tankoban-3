#pragma once
// SpeedMenu — Harbor's bottom-right playback-speed control, phase 1.
// Ports transport/speed-menu.tsx (speed section only): a Gauge trigger that
// grows into a pill showing the current rate when rate != 1x, opening a 400px
// popover with the 7 preset speed rows. Mirrors the AudioMenu/SubtitleMenu Qt
// pattern. Sleep-timer column, hold-to-speed pill, and `[ ]` hotkeys are NOT
// part of this control.

#include "engine/PlayerSnapshot.h"

#include <QWidget>

class QEnterEvent;
class QMouseEvent;

class SpeedPopup;

class SpeedMenu : public QWidget {
    Q_OBJECT
public:
    explicit SpeedMenu(QWidget* parent = nullptr);
    ~SpeedMenu() override;

    void setSnapshot(const PlayerSnapshot& snap);
    void syncPopupPosition();

signals:
    void rateRequested(double rate);
    void openChanged(bool open);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setOpen(bool open);
    void toggleOpen();
    void showPopup();
    void hidePopup();
    QRect globalRect() const;
    bool rateActive() const;

    SpeedPopup* m_popup = nullptr;
    PlayerSnapshot m_snapshot;
    bool m_hover = false;
    bool m_pressed = false;
    bool m_open = false;
};

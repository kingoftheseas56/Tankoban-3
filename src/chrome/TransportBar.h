#pragma once

#include "engine/PlayerSnapshot.h"

#include <QWidget>

class QAbstractButton;
class QGraphicsOpacityEffect;
class QPropertyAnimation;
class QTimer;
class AudioMenu;
class SeekBar;
class TimeLabel;
class VolumeControl;
class TitleInfo;

class TransportBar : public QWidget {
    Q_OBJECT
public:
    explicit TransportBar(QWidget* parent = nullptr);

    void setSnapshot(const PlayerSnapshot& snap);
    void setFullscreen(bool fullscreen);
    void setTitleInfo(const QString& title, const QString& subtitle = QString(), bool clickable = false);

signals:
    void backRequested();
    void playPauseRequested();
    void seekRequested(double sec);
    void seekStepRequested(double delta);
    void volumeRequested(double value0to6);
    void muteRequested(bool muted);
    void audioTrackRequested(const QString& id);
    void audioDelayRequested(double sec);
    void fullscreenRequested();

public slots:
    void wakeChrome();
    void setMenuOpen(bool open);

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;

private:
    void layoutChrome();
    void restartHideTimer();
    void setChromeVisible(bool visible);

    TimeLabel* m_start = nullptr;
    TimeLabel* m_end = nullptr;
    SeekBar* m_seek = nullptr;
    QAbstractButton* m_back = nullptr;
    QAbstractButton* m_playPause = nullptr;
    QAbstractButton* m_seekBack = nullptr;
    QAbstractButton* m_seekForward = nullptr;
    QAbstractButton* m_fullscreen = nullptr;
    AudioMenu* m_audioMenu = nullptr;
    VolumeControl* m_volume = nullptr;
    TitleInfo* m_titleInfo = nullptr;
    QGraphicsOpacityEffect* m_opacity = nullptr;
    QPropertyAnimation* m_fade = nullptr;
    QTimer* m_hideTimer = nullptr;
    bool m_playing = false;
    bool m_menuOpen = false;
    bool m_seekHovering = false;
    bool m_fullscreenOn = false;
};

#pragma once
// PlaybackClock — a singleton that exposes a smooth playback position. Fed the
// real time-pos on every snapshot; between snapshots it interpolates at ~60Hz
// (snapping to each real tick), so time widgets read smoothly without churning
// the full snapshot signal (spec §4.6, §0.1 #2).
#include <QObject>
#include <QElapsedTimer>

class QTimer;

class PlaybackClock : public QObject {
    Q_OBJECT
public:
    static PlaybackClock& instance();
    double position() const { return m_displayPos; }
    double buffered() const { return m_buffered; }
    void onSnapshot(double positionSec, double bufferedSec, bool playing, double rate);
    void reset();

signals:
    void changed();

private:
    explicit PlaybackClock(QObject* parent = nullptr);
    void tick();
    double m_anchorPos = 0, m_displayPos = 0, m_buffered = 0, m_rate = 1.0;
    bool m_playing = false;
    QElapsedTimer m_since;
    QTimer* m_timer = nullptr;
};

#include "engine/PlaybackClock.h"
#include <QTimer>

namespace {
// If no fresh real position re-anchors the clock within this window, mpv has
// stalled without (or before) flagging paused-for-cache — stop interpolating so
// the scrub bar can't fabricate forward motion past the frozen frame. Healthy
// playback re-anchors on every time-pos change (well under this), so smoothing is
// untouched during normal play; this is purely the stall backstop.
constexpr double kStaleSec = 1.0;
}

PlaybackClock& PlaybackClock::instance()
{
    static PlaybackClock clock;
    return clock;
}

PlaybackClock::PlaybackClock(QObject* parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(16);                       // ~60Hz
    connect(m_timer, &QTimer::timeout, this, &PlaybackClock::tick);
    m_since.start();
}

void PlaybackClock::onSnapshot(double positionSec, double bufferedSec, bool playing,
                               double rate, bool buffering)
{
    m_anchorPos  = positionSec;
    m_displayPos = positionSec;                     // snap to the real tick
    m_buffered   = bufferedSec;
    m_rate       = rate > 0 ? rate : 1.0;
    // Only interpolate when genuinely advancing: "not user-paused" AND "not cache-
    // stalled". paused-for-cache leaves mpv's pause=false (status Playing) while the
    // frame is frozen — without this gate the timer would ghost-advance the bar.
    m_playing    = playing && !buffering;
    m_since.restart();
    if (m_playing) { if (!m_timer->isActive()) m_timer->start(); }
    else           { m_timer->stop(); }
    emit changed();
}

void PlaybackClock::tick()
{
    if (!m_playing) return;
    // Freshness cap: never extrapolate more than kStaleSec past the last real anchor.
    // Healthy playback re-anchors continuously so this never bites; a silent stall
    // (no fresh time-pos) gets clamped instead of running the bar forward forever.
    const double elapsed = m_since.elapsed() / 1000.0;
    m_displayPos = m_anchorPos + (elapsed < kStaleSec ? elapsed : kStaleSec) * m_rate;
    emit changed();
}

void PlaybackClock::reset()
{
    m_timer->stop();
    m_anchorPos = m_displayPos = m_buffered = 0;
    m_rate = 1.0;
    m_playing = false;
    emit changed();
}

#include "engine/PlaybackClock.h"
#include <QTimer>

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

void PlaybackClock::onSnapshot(double positionSec, double bufferedSec, bool playing, double rate)
{
    m_anchorPos  = positionSec;
    m_displayPos = positionSec;                     // snap to the real tick
    m_buffered   = bufferedSec;
    m_rate       = rate > 0 ? rate : 1.0;
    m_playing    = playing;
    m_since.restart();
    if (playing) { if (!m_timer->isActive()) m_timer->start(); }
    else         { m_timer->stop(); }
    emit changed();
}

void PlaybackClock::tick()
{
    if (!m_playing) return;
    m_displayPos = m_anchorPos + (m_since.elapsed() / 1000.0) * m_rate;
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

#include "QuickPlayerBridge.h"

#include "engine/MpvController.h"
#include "engine/PlaybackClock.h"

#include <algorithm>
#include <cmath>

namespace {
QString statusName(PlayerSnapshot::Status status)
{
    switch (status) {
    case PlayerSnapshot::Idle: return QStringLiteral("idle");
    case PlayerSnapshot::Loading: return QStringLiteral("loading");
    case PlayerSnapshot::Ready: return QStringLiteral("ready");
    case PlayerSnapshot::Playing: return QStringLiteral("playing");
    case PlayerSnapshot::Paused: return QStringLiteral("paused");
    case PlayerSnapshot::Ended: return QStringLiteral("ended");
    case PlayerSnapshot::Error: return QStringLiteral("error");
    }
    return QStringLiteral("idle");
}
} // namespace

QuickPlayerBridge::QuickPlayerBridge(QObject* parent) : QObject(parent)
{
    m_controller = new MpvController(this);
    m_controller->initialize();

    connect(m_controller, &MpvController::snapshotChanged,
            this, &QuickPlayerBridge::applySnapshot);

    m_positionTimer.setInterval(16);
    connect(&m_positionTimer, &QTimer::timeout, this, &QuickPlayerBridge::tickPosition);
    m_positionTimer.start();
}

mpv_handle* QuickPlayerBridge::handle() const
{
    return m_controller ? m_controller->handle() : nullptr;
}

QString QuickPlayerBridge::status() const
{
    return statusName(m_snapshot.status);
}

bool QuickPlayerBridge::playing() const
{
    return m_snapshot.status == PlayerSnapshot::Playing;
}

void QuickPlayerBridge::setTitle(const QString& title)
{
    if (m_title == title)
        return;
    m_title = title;
    emit titleChanged();
}

void QuickPlayerBridge::setSubtitle(const QString& subtitle)
{
    if (m_subtitle == subtitle)
        return;
    m_subtitle = subtitle;
    emit titleChanged();
}

void QuickPlayerBridge::load(const QString& url, double startSec)
{
    m_pendingUrl = url;
    m_pendingStartSec = std::max(0.0, startSec);
    maybeLoadPending();
}

void QuickPlayerBridge::togglePlayPause()
{
    if (!m_controller)
        return;
    if (playing())
        m_controller->pause();
    else
        m_controller->play();
}

void QuickPlayerBridge::seek(double sec)
{
    if (!m_controller || m_snapshot.durationSec <= 0.0)
        return;
    m_controller->seek(std::clamp(sec, 0.0, m_snapshot.durationSec));
}

void QuickPlayerBridge::seekStep(double delta)
{
    seek(m_displayPositionSec + delta);
}

void QuickPlayerBridge::setVolume(double value)
{
    if (!m_controller)
        return;
    m_controller->setVolume(std::clamp(value, 0.0, 6.0));
}

void QuickPlayerBridge::toggleMuted()
{
    if (!m_controller)
        return;
    m_controller->setMuted(!m_snapshot.muted);
}

void QuickPlayerBridge::setRate(double value)
{
    if (!m_controller)
        return;
    m_controller->setRate(std::clamp(value, 0.25, 3.0));
}

void QuickPlayerBridge::setRenderReady()
{
    m_renderReady = true;
    maybeLoadPending();
}

void QuickPlayerBridge::applySnapshot(const PlayerSnapshot& snapshot)
{
    m_snapshot = snapshot;
    PlaybackClock::instance().onSnapshot(
        snapshot.positionSec,
        snapshot.bufferedSec,
        snapshot.status == PlayerSnapshot::Playing,
        snapshot.rate,
        snapshot.buffering);
    tickPosition();
    emit snapshotChanged();
}

void QuickPlayerBridge::maybeLoadPending()
{
    if (!m_controller || !m_renderReady || m_pendingUrl.isEmpty())
        return;

    const QString url = m_pendingUrl;
    const double start = m_pendingStartSec;
    m_pendingUrl.clear();
    m_pendingStartSec = 0.0;
    m_controller->load(url, start);
}

void QuickPlayerBridge::tickPosition()
{
    const double next = m_snapshot.durationSec > 0.0
        ? std::clamp(PlaybackClock::instance().position(), 0.0, m_snapshot.durationSec)
        : std::max(0.0, PlaybackClock::instance().position());
    if (std::abs(next - m_displayPositionSec) < 0.01)
        return;
    m_displayPositionSec = next;
    emit positionChanged();
}

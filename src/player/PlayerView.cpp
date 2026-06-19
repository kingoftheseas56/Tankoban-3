#include "player/PlayerView.h"
#include "player/HotkeyDispatcher.h"
#include "chrome/TransportBar.h"
#include "engine/MpvController.h"
#include "engine/MpvGlWidget.h"
#include "engine/PlaybackClock.h"
#include "player/PlayerLoadingOverlay.h"

#include <QApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QTimer>

namespace {

QRect clampToAvailable(const QRect& wanted, const QRect& available)
{
    if (!wanted.isValid() || !available.isValid()) return available;

    const int width = std::min(wanted.width(), available.width());
    const int height = std::min(wanted.height(), available.height());
    int x = wanted.x();
    int y = wanted.y();

    if (x < available.left()) x = available.left();
    if (y < available.top()) y = available.top();
    if (x + width > available.right() + 1) x = available.right() + 1 - width;
    if (y + height > available.bottom() + 1) y = available.bottom() + 1 - height;

    return QRect(x, y, width, height);
}

QRect fitWindowedRect(const QRect& wanted, const QRect& available)
{
    if (!wanted.isValid() || !available.isValid()) return available;

    if (wanted.width() <= available.width() && wanted.height() <= available.height())
        return clampToAvailable(wanted, available);

    QRect inset = available.adjusted(48, 32, -48, -48);
    if (!inset.isValid()) inset = available;

    QSize size = wanted.size();
    size.scale(inset.size(), Qt::KeepAspectRatio);
    if (size.width() <= 0 || size.height() <= 0) return available;

    QRect fitted(QPoint(0, 0), size);
    fitted.moveCenter(inset.center());
    return fitted;
}

} // namespace

PlayerView::PlayerView(QWidget* parent) : QWidget(parent)
{
    setStyleSheet(QStringLiteral("background:#000;"));
    m_controller = new MpvController(this);
    m_controller->initialize();

    m_video = new MpvGlWidget(m_controller, this);
    m_video->setAttribute(Qt::WA_OpaquePaintEvent);   // no bg-erase flash on resize
    m_video->setGeometry(rect());
    m_video->installEventFilter(this);                // click/dbl-click on the video itself

    m_clickTimer = new QTimer(this);
    m_clickTimer->setSingleShot(true);
    m_clickTimer->setInterval(QApplication::doubleClickInterval());
    connect(m_clickTimer, &QTimer::timeout, this, &PlayerView::playPauseToggle);

    connect(m_controller, &MpvController::snapshotChanged, this, [this](const PlayerSnapshot& s) {
        // Sticky everPlayed: real frames are flowing once we have a duration AND the
        // play head has advanced past the first moment (Harbor use-ever-played.ts).
        if (s.durationSec > 0.0 && s.positionSec > 0.3)
            m_everPlayed = true;
        PlaybackClock::instance().onSnapshot(
            s.positionSec, s.bufferedSec, s.status == PlayerSnapshot::Playing, s.rate, s.buffering);
        if (m_chrome) m_chrome->setSnapshot(s);
        if (m_loader)
            m_loader->applySnapshot(m_everPlayed, s.buffering, s.bufferedSec, s.durationSec,
                                    s.status == PlayerSnapshot::Ended,
                                    s.status == PlayerSnapshot::Error);
    });

    // Parent to the GL widget so Qt composites the chrome on top of the video.
    m_chrome = new TransportBar(m_video);
    m_chrome->setGeometry(m_video->rect());
    m_chrome->raise();
    m_chrome->installEventFilter(this);
    connect(m_chrome, &TransportBar::backRequested, this, [this] { emit backRequested(); });
    connect(m_chrome, &TransportBar::playPauseRequested, this, &PlayerView::playPauseToggle);
    connect(m_chrome, &TransportBar::seekRequested, m_controller, &MpvController::seek);
    connect(m_chrome, &TransportBar::seekStepRequested, this, [this](double delta) {
        m_controller->seek(std::max(0.0, PlaybackClock::instance().position() + delta));
    });
    connect(m_chrome, &TransportBar::volumeRequested, m_controller, &MpvController::setVolume);
    connect(m_chrome, &TransportBar::muteRequested, m_controller, &MpvController::setMuted);
    connect(m_chrome, &TransportBar::audioTrackRequested, m_controller, &MpvController::setAudioTrack);
    connect(m_chrome, &TransportBar::audioDelayRequested, m_controller, &MpvController::setAudioDelay);
    connect(m_chrome, &TransportBar::subtitleTrackRequested, m_controller, &MpvController::setSubtitleTrack);
    connect(m_chrome, &TransportBar::subtitleDelayRequested, m_controller, &MpvController::setSubDelay);
    connect(m_chrome, &TransportBar::speedRequested, m_controller, &MpvController::setRate);
    connect(m_chrome, &TransportBar::fullscreenRequested, this, &PlayerView::toggleFullscreen);

    // Loading / buffering cover, above the chrome so the warm-up takeover hides it.
    m_loader = new tankoban::PlayerLoadingOverlay(m_video);
    m_loader->setGeometry(m_video->rect());
    m_loader->raise();
    connect(m_loader, &tankoban::PlayerLoadingOverlay::cancelRequested,
            this, [this] { emit backRequested(); });

    qApp->installEventFilter(new HotkeyDispatcher(this, this));
}

void PlayerView::play(const QString& url, double startSec)
{
    m_everPlayed = false;
    if (m_loader) {
        // Local engine / proxy URL (torrent stream) reads "Preparing stream"; a remote
        // direct URL reads "Connecting". Show the warm-up cover immediately so there is
        // never a black/empty stage between handoff and the first frame.
        m_loader->setLocalEngine(url.contains(QStringLiteral("127.0.0.1"))
                                 || url.contains(QStringLiteral("localhost")));
        m_loader->applySnapshot(/*everPlayed=*/false, /*buffering=*/false, /*bufferedSec=*/0.0,
                                /*durationSec=*/0.0, /*ended=*/false, /*error=*/false);
    }
    m_controller->load(url, startSec);
}
void PlayerView::setTitleInfo(const QString& title, const QString& subtitle)
{
    if (m_chrome) m_chrome->setTitleInfo(title, subtitle, false);
    if (m_loader) m_loader->setTitle(title, subtitle);
}
PlayerSnapshot PlayerView::snap() const { return m_controller->snapshot(); }

void PlayerView::playPauseToggle()
{
    if (m_controller->snapshot().status == PlayerSnapshot::Playing) m_controller->pause();
    else m_controller->play();
}

void PlayerView::toggleFullscreen()
{
    // Borderless fullscreen via GEOMETRY (window is frameless) — never
    // showFullScreen(). This sidesteps Windows' exclusive-fullscreen transition
    // (the title-bar restore flash + multi-resize settle + QOpenGLWidget flicker
    // we've fought since Tankoban 2's applyFramelessWin32Style fix).
    QWidget* w = window();
    QScreen* screen = w->screen();
    const QRect available = screen ? screen->availableGeometry() : w->geometry();
    if (m_fakeFs) {
        w->setGeometry(fitWindowedRect(m_savedGeom, available));
        m_fakeFs = false;
    } else {
        m_savedGeom = fitWindowedRect(w->geometry(), available);
        // Use the working area, not the raw screen bounds, so the Windows taskbar
        // never obscures the bottom transport chrome in borderless mode.
        w->setGeometry(available);
        m_fakeFs = true;
    }
    if (m_chrome) m_chrome->setFullscreen(m_fakeFs);
    if (m_video) m_video->update();
}

void PlayerView::escapePressed()
{
    // Mirrors the chrome Back button, but steps out of borderless fullscreen
    // first if we're in it (conventional player UX). Never closes the window
    // directly — the host owns lifecycle via backRequested().
    if (m_fakeFs) toggleFullscreen();
    else emit backRequested();
}

void PlayerView::resizeEvent(QResizeEvent* e)
{
    if (m_video) m_video->setGeometry(rect());
    if (m_chrome && m_video) m_chrome->setGeometry(m_video->rect());
    if (m_loader && m_video) m_loader->setGeometry(m_video->rect());
    QWidget::resizeEvent(e);
}

bool PlayerView::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_video || obj == m_chrome) {
        if (obj == m_chrome && ev->type() == QEvent::MouseMove) m_chrome->wakeChrome();
        if (ev->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(ev);
            if (me->button() == Qt::LeftButton) {
                if (m_swallowRelease) m_swallowRelease = false;   // 2nd release of a dbl-click
                else m_clickTimer->start();                       // wait for a possible dbl-click
            }
        } else if (ev->type() == QEvent::MouseButtonDblClick) {
            auto* me = static_cast<QMouseEvent*>(ev);
            if (me->button() == Qt::LeftButton) {
                m_clickTimer->stop();
                m_swallowRelease = true;
                toggleFullscreen();
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

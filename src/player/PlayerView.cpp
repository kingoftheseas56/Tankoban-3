#include "player/PlayerView.h"
#include "player/HotkeyDispatcher.h"
#include "chrome/SeekBar.h"
#include "engine/MpvController.h"
#include "engine/MpvGlWidget.h"
#include "engine/PlaybackClock.h"

#include <QApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QTimer>

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

    connect(m_controller, &MpvController::snapshotChanged, this, [](const PlayerSnapshot& s) {
        PlaybackClock::instance().onSnapshot(
            s.positionSec, s.bufferedSec, s.status == PlayerSnapshot::Playing, s.rate);
    });

    // TEMP (Plan 2 T1 smoke): drop the SeekBar at the bottom; T3 moves it into TransportBar.
    // Parent to the GL widget so Qt composites it ON TOP of the video (a sibling gets
    // painted over between the chrome's own repaints).
    m_seek = new SeekBar(m_video);
    m_seek->raise();
    connect(m_seek, &SeekBar::seekRequested, m_controller, &MpvController::seek);
    connect(m_controller, &MpvController::snapshotChanged, this,
            [this](const PlayerSnapshot& s) { m_seek->setDuration(s.durationSec); });

    qApp->installEventFilter(new HotkeyDispatcher(this, this));
}

void PlayerView::play(const QString& url, double startSec) { m_controller->load(url, startSec); }
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
    if (m_fakeFs) {
        w->setGeometry(m_savedGeom);
        m_fakeFs = false;
    } else {
        m_savedGeom = w->geometry();
        QRect g = w->screen()->geometry();
        g.setHeight(g.height() + 1);   // +1px dodges Windows' exclusive-fullscreen path (no flicker)
        w->setGeometry(g);
        m_fakeFs = true;
    }
    if (m_video) m_video->update();
}

void PlayerView::resizeEvent(QResizeEvent* e)
{
    if (m_video) m_video->setGeometry(rect());
    if (m_seek)  m_seek->setGeometry(40, height() - 70, width() - 80, 48);
    QWidget::resizeEvent(e);
}

bool PlayerView::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_video) {
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

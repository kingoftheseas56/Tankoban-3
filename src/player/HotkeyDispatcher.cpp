#include "player/HotkeyDispatcher.h"
#include "player/PlayerView.h"
#include "engine/MpvController.h"
#include "engine/PlaybackClock.h"

#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <cmath>

HotkeyDispatcher::HotkeyDispatcher(PlayerView* view, QObject* parent)
    : QObject(parent), m_view(view), m_mpv(view->controller())
{
    m_holdTimer = new QTimer(this);
    m_holdTimer->setSingleShot(true);
    connect(m_holdTimer, &QTimer::timeout, this, &HotkeyDispatcher::onSpaceHold);
}

bool HotkeyDispatcher::eventFilter(QObject* /*obj*/, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        QWidget* fw = QApplication::focusWidget();
        if (qobject_cast<QLineEdit*>(fw) || qobject_cast<QTextEdit*>(fw) || qobject_cast<QComboBox*>(fw))
            return false;                               // typing-target gate (§9.1)
        const bool shift = ke->modifiers() & Qt::ShiftModifier;
        const int key = ke->key();

        if (key >= Qt::Key_1 && key <= Qt::Key_9) {     // digit seek to N*10%
            double d = m_mpv->snapshot().durationSec;
            if (d > 0) seekTo(d * (key - Qt::Key_0) / 10.0);
            return true;
        }
        switch (key) {
        case Qt::Key_Space:
            if (!ke->isAutoRepeat()) {                  // §9.1a: arm hold timer once
                m_spaceDown = true; m_holdActive = false;
                m_baseRate = m_mpv->snapshot().rate;
                m_holdTimer->start(350);
            }
            return true;
        case Qt::Key_Escape:       m_view->window()->close();             return true;
        case Qt::Key_F:            m_view->toggleFullscreen();            return true;
        case Qt::Key_Left:         seekStep(-10);                         return true;
        case Qt::Key_Right:        seekStep(10);                          return true;
        case Qt::Key_Comma:        seekStep(-30);                         return true;
        case Qt::Key_Period:       seekStep(30);                          return true;
        case Qt::Key_Home:         seekTo(0);                             return true;
        case Qt::Key_End: { double d = m_mpv->snapshot().durationSec; if (d > 0) seekTo(d - 0.5); return true; }
        case Qt::Key_0:            seekTo(0);                             return true;
        case Qt::Key_Up:           changeVolume(shift ? 0.5 : 0.05);      return true;
        case Qt::Key_Down:         changeVolume(shift ? -0.5 : -0.05);    return true;
        case Qt::Key_M:            m_mpv->setMuted(!m_mpv->snapshot().muted); return true;
        case Qt::Key_S:
        case Qt::Key_C:            cycleSubtitles();                      return true;
        case Qt::Key_Z:            changeSubDelay(shift ? -0.05 : -0.1);  return true;
        case Qt::Key_X:            changeSubDelay(shift ? 0.05 : 0.1);    return true;
        case Qt::Key_BracketLeft:  changeSpeed(-0.25);                    return true;
        case Qt::Key_BracketRight: changeSpeed(0.25);                     return true;
        // v (fill cycle) / = / - (zoom) land with VideoFill in Plan 3
        default: break;
        }
    } else if (ev->type() == QEvent::KeyRelease) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() == Qt::Key_Space && !ke->isAutoRepeat()) {
            m_spaceDown = false;
            if (m_holdTimer->isActive()) {              // released < 350ms = tap
                m_holdTimer->stop();
                m_view->playPauseToggle();
            } else if (m_holdActive) {                  // was holding -> restore base rate
                m_mpv->setRate(m_baseRate);
                m_holdActive = false;
            }
            return true;
        }
    }
    return false;
}

void HotkeyDispatcher::onSpaceHold()
{
    if (!m_spaceDown) return;
    m_holdActive = true;
    m_mpv->setRate(std::max(2.0, m_baseRate));          // §9.1a: hold -> >= 2x
}

void HotkeyDispatcher::seekStep(double delta)
{
    seekTo(std::max(0.0, PlaybackClock::instance().position() + delta));   // reads the live clock (§9.1)
}
void HotkeyDispatcher::seekTo(double sec) { m_mpv->seek(sec); }

void HotkeyDispatcher::changeVolume(double delta)
{
    m_mpv->setVolume(std::clamp(m_mpv->snapshot().volume + delta, 0.0, 6.0));   // persistence -> Plan 3
}
void HotkeyDispatcher::changeSubDelay(double delta)
{
    m_mpv->setSubDelay(std::round((m_mpv->snapshot().subDelaySec + delta) * 100.0) / 100.0);
}
void HotkeyDispatcher::changeSpeed(double delta)
{
    m_mpv->setRate(std::clamp(std::round((m_mpv->snapshot().rate + delta) * 100.0) / 100.0, 0.25, 3.0));
}
void HotkeyDispatcher::cycleSubtitles()
{
    const auto subs = m_mpv->snapshot().subtitleTracks;
    if (subs.isEmpty()) return;
    int sel = -1;
    for (int i = 0; i < subs.size(); ++i) if (subs[i].selected) { sel = i; break; }
    if (sel < 0)                    m_mpv->setSubtitleTrack(subs[0].id);
    else if (sel + 1 < subs.size()) m_mpv->setSubtitleTrack(subs[sel + 1].id);
    else                            m_mpv->setSubtitleTrack(QString());   // OFF
}

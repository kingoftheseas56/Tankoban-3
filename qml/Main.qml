import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Tankoban3Quick

// App shell: one window, one navigation stack. This IS the experiment —
// every screen is pushed/popped here, so the Home -> Series -> Sources ->
// Player walk runs through this single StackView. The litmus test lives here.
ApplicationWindow {
    id: win
    width: Math.min(1280, Screen.desktopAvailableWidth)
    height: Math.min(800, Screen.desktopAvailableHeight)
    visible: true
    title: "Tankoban 3 - Qt Quick"
    color: "#0b0d12"
    flags: Qt.Window | Qt.FramelessWindowHint

    // App-lifetime mpv engine: ONE core for the whole app. The player page only
    // owns a per-visit render surface (MpvObject). Popping the player frees that
    // surface, NOT the core — so the render context is always freed before the
    // core (which only dies at app exit), avoiding the mpv teardown use-after-free.
    QuickPlayerBridge { id: appPlayer }

    // Borderless fullscreen via GEOMETRY — never Window.FullScreen, which triggers
    // Windows' exclusive-fullscreen transition (the blink/flash on toggle, QTBUG-51093).
    // The window is frameless (flags above), so one geometry swap covers the screen with
    // zero OS transition. The +1px height dodges Windows' exclusive-fullscreen optimization
    // path (Qt-forum-confirmed trick). See feedback_qt_fullscreen_blink_borderless_geometry_fix.
    property bool fakeFullscreen: false
    property rect savedGeometry: Qt.rect(0, 0, 0, 0)
    function toggleFullscreen() {
        if (!fakeFullscreen) {
            savedGeometry = Qt.rect(win.x, win.y, win.width, win.height)
            win.x = Screen.virtualX
            win.y = Screen.virtualY
            win.width = Screen.width
            win.height = Screen.height + 1
            fakeFullscreen = true
        } else {
            win.x = savedGeometry.x
            win.y = savedGeometry.y
            win.width = savedGeometry.width
            win.height = savedGeometry.height
            fakeFullscreen = false
        }
    }

    header: ToolBar {
        visible: !(stack.currentItem && stack.currentItem.playerSurface)
        height: visible ? 48 : 0
        background: Rectangle { color: "#11141b" }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 16
            spacing: 12
            ToolButton {
                text: "<  Back"
                enabled: stack.depth > 1
                onClicked: stack.pop()
            }
            Label {
                Layout.fillWidth: true
                text: stack.currentItem && stack.currentItem.crumb ? stack.currentItem.crumb : ""
                color: "#e7c66b"
                font.pixelSize: 16
                elide: Text.ElideRight
            }
            Label {
                text: "Qt Quick - navigation skeleton"
                color: "#56607a"
                font.pixelSize: 12
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: homeComp   // Home -> Series -> Sources -> Player (in-flow, never in isolation)

        // Smooth page slide — the kind of transition that's nearly free in Quick.
        pushEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 180 }
            PropertyAnimation { property: "x"; from: 40; to: 0; duration: 220; easing.type: Easing.OutCubic }
        }
    }

    Component { id: homeComp;    HomePage    { onOpenSeries:  stack.push(seriesComp) } }
    Component { id: seriesComp;  SeriesPage  { onOpenSources: stack.push(sourcesComp) } }
    Component { id: sourcesComp; SourcesPage { onPlay:        stack.push(playerComp) } }
    Component { id: playerComp;  PlayerPage  { player: appPlayer; onBack: stack.pop() } }
}

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Tankoban3Quick

Item {
    id: page
    signal back()
    property string crumb: "Now Playing"
    property bool playerSurface: true
    property QuickPlayerBridge player   // app-lifetime engine, injected by Main.qml
    Component.onDestruction: {
        if (player && player.playing)
            player.togglePlayPause()    // pause audio when leaving the player surface
    }
    property bool compact: width < 1000
    property bool tight: width < 600
    property bool chromeVisible: true
    property bool seeking: false
    property real seekPreview: player.positionSec
    property real normalFraction: 0.6
    property real maxVolume: 6.0
    property string gold: "#d8ac35"
    property string smokeUrl: "https://download.blender.org/peach/bigbuckbunny_movies/BigBuckBunny_320x180.mp4"

    function fmtTime(sec) {
        if (!isFinite(sec) || sec < 0)
            return "0:00"
        var total = Math.floor(sec)
        var h = Math.floor(total / 3600)
        var m = Math.floor((total % 3600) / 60)
        var s = total % 60
        var ss = s < 10 ? "0" + s : "" + s
        if (h > 0) {
            var mm = m < 10 ? "0" + m : "" + m
            return h + ":" + mm + ":" + ss
        }
        return m + ":" + ss
    }

    function fractionFromVolume(v) {
        var clamped = Math.max(0, Math.min(maxVolume, v))
        if (clamped <= 1)
            return clamped * normalFraction
        return normalFraction + ((clamped - 1) / (maxVolume - 1)) * (1 - normalFraction)
    }

    function volumeFromFraction(f) {
        var clamped = Math.max(0, Math.min(1, f))
        if (clamped <= normalFraction)
            return clamped / normalFraction
        return 1 + ((clamped - normalFraction) / (1 - normalFraction)) * (maxVolume - 1)
    }

    function boostColor(v) {
        if (v <= 1)
            return "#ffffff"
        var t = Math.min(1, (v - 1) / (maxVolume - 1))
        var r = Math.round(249 - t * 29)
        var g = Math.round(115 - t * 77)
        var b = Math.round(22 + t * 16)
        return Qt.rgba(r / 255, g / 255, b / 255, 1)
    }

    function wakeChrome() {
        chromeVisible = true
        hideTimer.restart()
    }

    function toggleFullscreen() {
        // Delegates to the window's geometry-based borderless fullscreen (no OS
        // Window.FullScreen → no Windows exclusive-fullscreen blink).
        var w = page.Window.window
        if (w)
            w.toggleFullscreen()
    }


    Timer {
        id: hideTimer
        interval: player.playing ? 1800 : 4500
        repeat: false
        running: true
        onTriggered: {
            if (player.playing && !seeking)
                chromeVisible = false
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    MpvObject {
        id: video
        anchors.fill: parent
        player: page.player
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onPositionChanged: wakeChrome()
        onClicked: player.togglePlayPause()
    }

    Component.onCompleted: {
        player.title = "Big Buck Bunny"
        player.subtitle = "Qt Quick player litmus"
        player.load(smokeUrl, 0)
        wakeChrome()
    }

    Item {
        id: chrome
        anchors.fill: parent
        opacity: chromeVisible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 128
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#8c000000" }
                GradientStop { position: 0.55; color: "#26000000" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 210
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 0.42; color: "#40000000" }
                GradientStop { position: 1.0; color: "#b3000000" }
            }
        }

        Button {
            id: backButton
            x: 28
            y: 16
            width: 48
            height: 48
            flat: true
            onClicked: { console.log("[NAVDBG] PLAYER back pressed"); page.back() }
            background: Rectangle {
                radius: 24
                color: backButton.hovered ? "#cc000000" : "#8c000000"
            }
            contentItem: IconGlyph { kind: "back"; iconColor: "#ffffff" }
        }

        Column {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 17
            anchors.rightMargin: 30
            spacing: 2
            width: Math.min(520, parent.width - 130)

            Text {
                width: parent.width
                text: player.title
                color: "#ffffff"
                font.pixelSize: 19
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                style: Text.Raised
                styleColor: "#99000000"
            }

            Text {
                width: parent.width
                text: player.subtitle
                visible: text.length > 0
                color: "#b3ffffff"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                style: Text.Raised
                styleColor: "#99000000"
            }
        }

        Item {
            id: bottomChrome
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: tight ? 12 : 28
            anchors.rightMargin: tight ? 12 : 28
            anchors.bottomMargin: tight ? 12 : 20
            height: tight ? 116 : 148

            RowLayout {
                id: seekRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 48
                spacing: 12

                Text {
                    visible: !tight
                    text: fmtTime(seeking ? seekPreview : player.positionSec)
                    color: "#d9ffffff"
                    font.family: "Consolas"
                    font.pixelSize: 13
                    Layout.preferredWidth: 58
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }

                Item {
                    id: seekBar
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    property bool hovered: false
                    property real value: Math.max(0, Math.min(1, player.durationSec > 0 ? ((seeking ? seekPreview : player.positionSec) / player.durationSec) : 0))

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: seekBar.hovered || seeking ? 8 : 6
                        radius: height / 2
                        color: "#26ffffff"
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * seekBar.value
                        height: seekBar.hovered || seeking ? 8 : 6
                        radius: height / 2
                        color: gold
                    }

                    Rectangle {
                        x: parent.width * seekBar.value - width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        width: seeking ? 20 : 16
                        height: width
                        radius: width / 2
                        color: gold
                        border.color: "#66000000"
                        border.width: 1
                    }

                    Rectangle {
                        visible: seekBar.hovered && !seeking
                        x: Math.max(0, Math.min(parent.width - width, mouseArea.mouseX - width / 2))
                        y: -30
                        width: tooltipText.implicitWidth + 16
                        height: 28
                        radius: 6
                        color: "#e6000000"
                        border.color: "#1affffff"
                        Text {
                            id: tooltipText
                            anchors.centerIn: parent
                            text: fmtTime(seekPreview)
                            color: "#ffffff"
                            font.family: "Consolas"
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: {
                            seekBar.hovered = true
                            wakeChrome()
                        }
                        onExited: {
                            seekBar.hovered = false
                            if (!seeking)
                                seekPreview = player.positionSec
                        }
                        onPositionChanged: {
                            seekPreview = player.durationSec * Math.max(0, Math.min(1, mouseX / Math.max(1, width)))
                            wakeChrome()
                        }
                        onPressed: {
                            seeking = true
                            seekPreview = player.durationSec * Math.max(0, Math.min(1, mouseX / Math.max(1, width)))
                            wakeChrome()
                        }
                        onReleased: {
                            player.seek(seekPreview)
                            seeking = false
                            wakeChrome()
                        }
                    }
                }

                Text {
                    visible: !tight
                    text: fmtTime(player.durationSec)
                    color: "#a6ffffff"
                    font.family: "Consolas"
                    font.pixelSize: 13
                    Layout.preferredWidth: 58
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }
            }

            RowLayout {
                id: controlRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 64
                spacing: compact ? 8 : 16

                VolumeControl {
                    visible: !tight
                    Layout.preferredWidth: player.volume > 1.001 ? 220 : 182
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                }

                Item { Layout.fillWidth: true; visible: !compact }

                Row {
                    Layout.alignment: Qt.AlignCenter
                    spacing: 6

                    CircleButton {
                        width: 56
                        height: 56
                        label: "10"
                        kind: "seekBack"
                        onClicked: player.seekStep(-10)
                    }

                    CircleButton {
                        width: tight ? 48 : compact ? 56 : 64
                        height: width
                        kind: player.playing ? "pause" : "play"
                        hero: true
                        onClicked: player.togglePlayPause()
                    }

                    CircleButton {
                        width: 56
                        height: 56
                        label: "10"
                        kind: "seekForward"
                        onClicked: player.seekStep(10)
                    }
                }

                Item { Layout.fillWidth: true }

                Row {
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    spacing: 6

                    AudioMenuButton {
                        visible: !tight
                    }
                    SubtitleMenuButton {
                    }
                    SpeedMenuButton {
                        visible: !compact
                    }
                    CircleButton {
                        width: 48
                        height: 48
                        kind: (page.Window.window && page.Window.window.fakeFullscreen) ? "minimize" : "maximize"
                        onClicked: toggleFullscreen()
                    }
                }
            }
        }
    }

    component CircleButton: Button {
        id: button
        property string kind: ""
        property string label: ""
        property bool hero: false

        flat: true
        hoverEnabled: true
        scale: down ? 0.95 : 1
        Behavior on scale { NumberAnimation { duration: 80 } }

        background: Rectangle {
            radius: width / 2
            color: !button.enabled ? "#00000000"
                : button.hero ? (button.hovered ? "#38ffffff" : "#1fffffff")
                : button.hovered ? "#1affffff" : "#00000000"
        }

        contentItem: Item {
            anchors.fill: parent

            IconGlyph {
                anchors.fill: parent
                kind: button.kind
                label: button.label
                hero: button.hero
                iconColor: button.enabled ? "#e6ffffff" : "#4dffffff"
            }
        }
    }

    component IconGlyph: Canvas {
        id: glyph
        property string kind: ""
        property string label: ""
        property bool hero: false
        property color iconColor: "#e6ffffff"

        antialiasing: true
        onKindChanged: requestPaint()
        onLabelChanged: requestPaint()
        onHeroChanged: requestPaint()
        onIconColorChanged: requestPaint()
        onPaint: {
            // Every glyph is a faithful port of the proven QWidget TransportBar
            // geometry (src/chrome/TransportBar.cpp, VolumeControl/AudioMenu/
            // SubtitleMenu/SpeedMenu). lucide's 24x24 grid is mapped into a
            // centred box sized to match the QWidget glyph-to-button ratio, and
            // pen widths are authored at the 48px button (penBase scales them).
            var ctx = getContext("2d")
            var w = width
            var h = height
            var s = Math.min(w, h)
            var cx = w / 2
            var cy = h / 2
            var penBase = s / 48

            ctx.clearRect(0, 0, w, h)
            ctx.strokeStyle = iconColor
            ctx.fillStyle = iconColor
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            // lucide-grid binding for stroked glyphs (box = gscale * s, centred).
            var ox = 0, oy = 0, unit = 1
            function bind(gscale) {
                var box = gscale * s
                unit = box / 24
                ox = (w - box) / 2
                oy = (h - box) / 2
            }
            function px(x) { return ox + x * unit }
            function py(y) { return oy + y * unit }
            function line(x1, y1, x2, y2) {
                ctx.beginPath()
                ctx.moveTo(px(x1), py(y1))
                ctx.lineTo(px(x2), py(y2))
                ctx.stroke()
            }
            function roundRectPath(x, y, ww, hh, r) {
                ctx.beginPath()
                ctx.moveTo(x + r, y)
                ctx.lineTo(x + ww - r, y)
                ctx.arcTo(x + ww, y, x + ww, y + r, r)
                ctx.lineTo(x + ww, y + hh - r)
                ctx.arcTo(x + ww, y + hh, x + ww - r, y + hh, r)
                ctx.lineTo(x + r, y + hh)
                ctx.arcTo(x, y + hh, x, y + hh - r, r)
                ctx.lineTo(x, y + r)
                ctx.arcTo(x, y, x + r, y, r)
                ctx.closePath()
            }

            if (kind === "play") {
                // QWidget: filled triangle, no circle (hero 64px fractions).
                ctx.beginPath()
                ctx.moveTo(cx - 0.078 * s, cy - 0.172 * s)
                ctx.lineTo(cx - 0.078 * s, cy + 0.172 * s)
                ctx.lineTo(cx + 0.203 * s, cy)
                ctx.closePath()
                ctx.fill()
            } else if (kind === "pause") {
                // QWidget: two filled rounded bars, no circle.
                var bw = 0.094 * s, bh = 0.375 * s, br = 0.031 * s
                var topY = cy - bh / 2
                roundRectPath(cx - 0.156 * s, topY, bw, bh, br); ctx.fill()
                roundRectPath(cx + 0.063 * s, topY, bw, bh, br); ctx.fill()
            } else if (kind === "back") {
                bind(0.46)
                ctx.lineWidth = 2.2 * penBase
                ctx.beginPath()
                ctx.moveTo(px(15), py(18))
                ctx.lineTo(px(9), py(12))
                ctx.lineTo(px(15), py(6))
                ctx.stroke()
            } else if (kind === "seekBack" || kind === "seekForward") {
                // QWidget: 270deg circular arrow + arrowhead + step number.
                var fwd = kind === "seekForward"
                var r = 0.268 * s
                ctx.lineWidth = 2.0 * penBase
                ctx.beginPath()
                if (fwd)
                    ctx.arc(cx, cy, r, 325 * Math.PI / 180, 55 * Math.PI / 180, true)
                else
                    ctx.arc(cx, cy, r, 215 * Math.PI / 180, 125 * Math.PI / 180, false)
                ctx.stroke()
                ctx.beginPath()
                if (fwd) {
                    ctx.moveTo(cx + 0.232 * s, cy - 0.232 * s)
                    ctx.lineTo(cx + 0.357 * s, cy - 0.196 * s)
                    ctx.lineTo(cx + 0.268 * s, cy - 0.089 * s)
                } else {
                    ctx.moveTo(cx - 0.232 * s, cy - 0.232 * s)
                    ctx.lineTo(cx - 0.357 * s, cy - 0.196 * s)
                    ctx.lineTo(cx - 0.268 * s, cy - 0.089 * s)
                }
                ctx.stroke()
                ctx.fillStyle = iconColor
                ctx.font = "700 " + Math.round(s * 0.18) + "px Consolas"
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText(label, cx, cy + s * 0.02)
            } else if (kind === "volume" || kind === "mute") {
                bind(0.5)
                ctx.lineWidth = 1.75 * penBase
                ctx.beginPath()
                ctx.moveTo(px(3), py(10))
                ctx.lineTo(px(7), py(10))
                ctx.lineTo(px(12), py(6))
                ctx.lineTo(px(12), py(18))
                ctx.lineTo(px(7), py(14))
                ctx.lineTo(px(3), py(14))
                ctx.closePath()
                ctx.stroke()
                if (kind === "mute") {
                    line(16, 9, 21, 14)
                    line(21, 9, 16, 14)
                } else {
                    ctx.beginPath()
                    ctx.moveTo(px(16), py(8))
                    ctx.bezierCurveTo(px(18.5), py(10), px(18.5), py(14), px(16), py(16))
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(px(14), py(10))
                    ctx.bezierCurveTo(px(15.5), py(11), px(15.5), py(13), px(14), py(14))
                    ctx.stroke()
                }
            } else if (kind === "maximize" || kind === "minimize") {
                bind(0.46)
                ctx.lineWidth = 1.9 * penBase
                if (kind === "maximize") {
                    line(8, 3, 5, 3); line(5, 3, 3, 5); line(3, 5, 3, 8)
                    line(16, 3, 19, 3); line(19, 3, 21, 5); line(21, 5, 21, 8)
                    line(3, 16, 3, 19); line(3, 19, 5, 21); line(5, 21, 8, 21)
                    line(16, 21, 19, 21); line(19, 21, 21, 19); line(21, 19, 21, 16)
                } else {
                    line(8, 3, 8, 6); line(8, 6, 6, 8); line(6, 8, 3, 8)
                    line(16, 3, 16, 6); line(16, 6, 18, 8); line(18, 8, 21, 8)
                    line(3, 16, 6, 16); line(6, 16, 8, 18); line(8, 18, 8, 21)
                    line(16, 21, 16, 18); line(16, 18, 18, 16); line(18, 16, 21, 16)
                }
            } else if (kind === "audio") {
                // lucide Languages (QWidget AudioMenu trigger).
                bind(0.42)
                ctx.lineWidth = 1.95 * penBase
                ctx.beginPath()
                ctx.moveTo(px(5), py(7))
                ctx.lineTo(px(10), py(17))
                ctx.lineTo(px(15), py(7))
                ctx.stroke()
                line(7.3, 12.4, 12.7, 12.4)
                line(17, 7, 21, 7); line(19, 7, 19, 17)
                line(16.5, 10.5, 21.5, 10.5); line(16.5, 13.8, 21.5, 13.8)
            } else if (kind === "subtitle") {
                // lucide Captions (QWidget SubtitleMenu trigger).
                bind(0.42)
                ctx.lineWidth = 1.9 * penBase
                roundRectPath(px(3), py(5), 18 * unit, 14 * unit, 3 * unit)
                ctx.stroke()
                line(7, 15, 11, 15); line(15, 15, 17, 15)
                line(7, 11, 9, 11); line(13, 11, 17, 11)
            } else if (kind === "speed") {
                // lucide Gauge (Harbor: size 22, strokeWidth 1.9): 240deg dial + needle.
                bind(0.46)
                ctx.lineWidth = 1.9 * penBase
                var dcx = px(12), dcy = py(14), dr = 10 * unit
                ctx.beginPath()
                ctx.arc(dcx, dcy, dr, 150 * Math.PI / 180, 30 * Math.PI / 180, false)
                ctx.stroke()
                line(12, 14, 16, 10)
            } else {
                ctx.fillStyle = iconColor
                ctx.font = "700 " + Math.round(s * 0.22) + "px Arial"
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText(label, cx, cy)
            }
        }
    }

    component VolumeControl: Row {
        id: volumeControl
        spacing: 8

        function setFromX(x) {
            var f = Math.max(0, Math.min(1, x / Math.max(1, volumeTrack.width)))
            player.setVolume(Math.round(volumeFromFraction(f) * 100) / 100)
            wakeChrome()
        }

        CircleButton {
            width: 48
            height: 48
            kind: player.muted || player.volume <= 0.001 ? "mute" : "volume"
            onClicked: player.toggleMuted()
        }

        Item {
            id: volumeTrack
            width: 120
            height: 48

            property real volumeValue: player.muted ? 0 : player.volume
            property real filled: fractionFromVolume(volumeValue)
            property color colorValue: boostColor(volumeValue)

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 8
                radius: 4
                color: "#26ffffff"
            }

            Rectangle {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width * volumeTrack.filled
                height: 8
                radius: 4
                color: volumeTrack.colorValue
            }

            Rectangle {
                x: parent.width * volumeTrack.filled - 7
                anchors.verticalCenter: parent.verticalCenter
                width: 14
                height: 14
                radius: 7
                color: volumeTrack.colorValue
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onPressed: volumeControl.setFromX(mouseX)
                onPositionChanged: {
                    if (pressed)
                        volumeControl.setFromX(mouseX)
                    else
                        wakeChrome()
                }
            }
        }

        Text {
            visible: !player.muted && player.volume > 1.001
            anchors.verticalCenter: parent.verticalCenter
            text: Math.round(player.volume * 100) + "%"
            color: boostColor(player.volume)
            font.pixelSize: 12
            font.weight: Font.DemiBold
            width: 36
        }
    }

    // SpeedMenuButton — faithful port of the QWidget SpeedMenu/SpeedPopup
    // (src/chrome/SpeedMenu.cpp): a Gauge trigger that grows into a pill showing
    // the current rate when rate != 1x, opening a 400px popover with the 7 preset
    // speed rows. Harbor token palette matches AudioMenu/SubtitleMenu.
    component SpeedMenuButton: Item {
        id: sRoot
        property bool active: Math.abs(player.rate - 1) > 0.01
        // Mirrors QWidget rateLabel(): "1.5", "1.25", "2" + the × sign.
        function rateLabel(r) {
            var n = Math.round(r * 100) / 100
            return n + "×"
        }
        implicitWidth: trigger.width
        implicitHeight: 48
        width: implicitWidth
        height: 48

        Button {
            id: trigger
            height: 48
            // 48px gauge box (left) + label at x40 + right pad when active; 48 otherwise.
            width: sRoot.active ? (54 + rateText.implicitWidth) : 48
            flat: true
            hoverEnabled: true
            scale: down ? 0.95 : 1
            Behavior on scale { NumberAnimation { duration: 80 } }

            background: Rectangle {
                radius: height / 2
                color: (sRoot.active || speedPopup.opened)
                       ? (trigger.hovered ? "#4cffffff" : "#38ffffff")   // white/30 : white/22
                       : (trigger.hovered ? "#1affffff" : "#00000000")   // white/10 : transparent
            }

            contentItem: Item {
                // Gauge rendered on a full 48px canvas (same as the audio/subtitle/
                // fullscreen icons) so size + stroke match; centred when at rest,
                // left-aligned when the rate pill grows.
                IconGlyph {
                    width: 48
                    height: 48
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    kind: "speed"
                    iconColor: (sRoot.active || speedPopup.opened || trigger.hovered) ? "#ffffff" : "#e6ffffff"
                }
                Text {
                    id: rateText
                    visible: sRoot.active
                    x: 40
                    anchors.verticalCenter: parent.verticalCenter
                    text: sRoot.rateLabel(player.rate)
                    color: (sRoot.active || trigger.hovered) ? "#ffffff" : "#e6ffffff"
                    font.family: "Inter"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                }
            }

            onClicked: speedPopup.opened ? speedPopup.close() : speedPopup.open()
        }

        Popup {
            id: speedPopup
            width: 400
            height: 334                 // padding(16) + title(24) + 7*40 + 7 gaps*2
            x: trigger.width - width     // right-aligned to the trigger
            y: -height - 10              // floats above the trigger
            padding: 8
            closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside | Popup.CloseOnEscape

            background: Rectangle {
                radius: 16
                color: "#f7111419"          // bg-elevated/97
                border.color: "#16ffffff"   // border-edge
                border.width: 1
            }

            contentItem: Column {
                spacing: 2

                Item {
                    width: 384
                    height: 24
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Playback speed"
                        font.family: "Inter"
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1.6
                        color: "#a4acb7"
                    }
                }

                Repeater {
                    model: [0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0]
                    delegate: Rectangle {
                        id: speedRow
                        required property real modelData
                        width: 384
                        height: 40
                        radius: 8
                        property bool selected: Math.abs(modelData - player.rate) < 0.01
                        property bool isNormal: Math.abs(modelData - 1.0) < 0.01
                        color: selected ? "#1affffff" : (rowMouse.containsMouse ? "#0cffffff" : "#00000000")
                        border.color: selected ? "#16ffffff" : "#00000000"
                        border.width: 1

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: speedRow.isNormal ? "Normal" : sRoot.rateLabel(speedRow.modelData)
                            color: speedRow.selected ? "#f5f7fa" : "#838b97"
                            font.family: "Inter"
                            font.pixelSize: 14
                            font.weight: speedRow.selected ? Font.Medium : Font.Normal
                        }

                        Text {
                            visible: speedRow.isNormal
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: "default"
                            font.capitalization: Font.AllUppercase
                            color: "#a4acb7"
                            font.family: "Inter"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }

                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                player.setRate(speedRow.modelData)
                                speedPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }

    // Small ±delay step pill, shared by the audio + subtitle sync footers.
    component DelayStepButton: Button {
        id: delayStep
        flat: true
        hoverEnabled: true
        implicitHeight: 24
        implicitWidth: Math.max(36, stepText.implicitWidth + 16)
        background: Rectangle {
            radius: 6
            color: delayStep.hovered ? "#1fffffff" : "#12ffffff"
        }
        contentItem: Text {
            id: stepText
            text: delayStep.text
            color: "#f5f7fa"
            font.family: "Inter"
            font.pixelSize: 11
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // AudioMenuButton — single-column port of the QWidget AudioMenu/AudioPopup
    // (src/chrome/AudioMenu.cpp): track rows + a sync-delay footer. The leading
    // per-row language badge pill is folded into the meta line.
    component AudioMenuButton: Item {
        id: aRoot
        implicitWidth: 48
        implicitHeight: 48
        width: 48
        height: 48

        function trackTitle(t) {
            if (t.title && t.title.trim() !== "" && t.title !== t.lang) return t.title
            if (t.label && t.label.trim() !== "" && t.label !== t.lang) return t.label
            if (t.lang && t.lang.trim() !== "") return t.lang.toUpperCase()
            return "Track"
        }
        function trackMeta(t) {
            var parts = []
            if (t.lang && t.lang.trim() !== "") parts.push(t.lang.toUpperCase())
            if (t.codec && t.codec.trim() !== "") parts.push(t.codec.toUpperCase())
            if (t.channels && t.channels.trim() !== "") parts.push(t.channels)
            if (t.isDefault) parts.push("DEFAULT")
            return parts.join("   /   ")
        }

        Button {
            id: audioTrigger
            width: 48
            height: 48
            flat: true
            hoverEnabled: true
            scale: down ? 0.95 : 1
            Behavior on scale { NumberAnimation { duration: 80 } }
            background: Rectangle {
                radius: width / 2
                color: audioPopup.opened ? "#38ffffff" : (audioTrigger.hovered ? "#1affffff" : "#00000000")
            }
            contentItem: Item {
                IconGlyph {
                    anchors.fill: parent
                    kind: "audio"
                    iconColor: (audioPopup.opened || audioTrigger.hovered) ? "#ffffff" : "#e6ffffff"
                }
            }
            onClicked: audioPopup.opened ? audioPopup.close() : audioPopup.open()
        }

        Popup {
            id: audioPopup
            readonly property int trackCount: player.audioTracks.length
            readonly property int listH: trackCount > 0 ? Math.min(280, Math.max(56, trackCount * 48 + 12)) : 72
            width: 360
            height: 48 + listH + 40
            x: audioTrigger.width - width
            y: -height - 10
            padding: 0
            closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside | Popup.CloseOnEscape

            background: Rectangle {
                radius: 16
                color: "#f7111419"
                border.color: "#16ffffff"
                border.width: 1
            }

            contentItem: Item {
                // Header
                Item {
                    width: parent.width
                    height: 48
                    Text {
                        id: audioHeaderText
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Audio"
                        color: "#f5f7fa"
                        font.family: "Inter"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    Text {
                        anchors.left: audioHeaderText.right
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        visible: audioPopup.trackCount > 0
                        text: audioPopup.trackCount
                        color: "#a4acb7"
                        font.family: "Inter"
                        font.pixelSize: 12
                    }
                    Button {
                        id: audioClose
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: 28
                        height: 28
                        flat: true
                        hoverEnabled: true
                        background: Rectangle { radius: 14; color: audioClose.hovered ? "#12ffffff" : "#00000000" }
                        contentItem: Item {
                            Rectangle { anchors.centerIn: parent; width: 12; height: 1.6; radius: 1; color: "#a4acb7"; rotation: 45 }
                            Rectangle { anchors.centerIn: parent; width: 12; height: 1.6; radius: 1; color: "#a4acb7"; rotation: -45 }
                        }
                        onClicked: audioPopup.close()
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#12ffffff" }
                }

                // Track list / empty state
                Item {
                    y: 48
                    width: parent.width
                    height: audioPopup.listH

                    Text {
                        visible: audioPopup.trackCount === 0
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.top: parent.top
                        anchors.topMargin: 14
                        width: parent.width - 32
                        wrapMode: Text.WordWrap
                        text: "This file has one audio track."
                        color: "#838b97"
                        font.family: "Inter"
                        font.pixelSize: 13
                    }

                    ListView {
                        visible: audioPopup.trackCount > 0
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds
                        model: player.audioTracks
                        delegate: Rectangle {
                            id: aRow
                            required property var modelData
                            width: ListView.view.width
                            height: 46
                            radius: 8
                            property bool sel: modelData.selected === true
                            color: sel ? "#1affffff" : (aRowHover.containsMouse ? "#0affffff" : "#00000000")
                            border.color: sel ? "#16ffffff" : "#00000000"
                            border.width: 1

                            Rectangle {
                                id: aDot
                                x: 10
                                y: 15
                                width: 16
                                height: 16
                                radius: 8
                                color: aRow.sel ? page.gold : "#12ffffff"
                                Text {
                                    anchors.centerIn: parent
                                    visible: aRow.sel
                                    text: "✓"
                                    color: "#13151b"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }
                            Text {
                                anchors.left: aDot.right
                                anchors.leftMargin: 11
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                y: 7
                                text: aRoot.trackTitle(aRow.modelData)
                                elide: Text.ElideRight
                                color: "#f5f7fa"
                                font.family: "Inter"
                                font.pixelSize: 13
                                font.weight: Font.Medium
                            }
                            Text {
                                anchors.left: aDot.right
                                anchors.leftMargin: 11
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                y: 25
                                text: aRoot.trackMeta(aRow.modelData)
                                elide: Text.ElideRight
                                color: "#a4acb7"
                                font.family: "Inter"
                                font.pixelSize: 11
                            }
                            MouseArea {
                                id: aRowHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { player.setAudioTrack(aRow.modelData.id); audioPopup.close() }
                            }
                        }
                    }
                }

                // Sync-delay footer
                Item {
                    y: audioPopup.height - 40
                    width: parent.width
                    height: 40
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#12ffffff" }
                    RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 12
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6
                        Text {
                            text: "SYNC"
                            color: "#838b97"
                            font.family: "Inter"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        DelayStepButton {
                            text: "-0.1"
                            onClicked: player.setAudioDelay(Math.round((player.audioDelaySec - 0.1) * 100) / 100)
                        }
                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: (player.audioDelaySec >= 0 ? "+" : "") + player.audioDelaySec.toFixed(2) + "s"
                            color: "#f5f7fa"
                            font.family: "Consolas"
                            font.pixelSize: 13
                        }
                        DelayStepButton {
                            text: "+0.1"
                            onClicked: player.setAudioDelay(Math.round((player.audioDelaySec + 0.1) * 100) / 100)
                        }
                        Button {
                            id: audioReset
                            visible: Math.abs(player.audioDelaySec) > 0.0001
                            flat: true
                            hoverEnabled: true
                            implicitWidth: 24
                            implicitHeight: 24
                            background: Rectangle { radius: 12; color: audioReset.hovered ? "#12ffffff" : "#00000000" }
                            contentItem: Text {
                                text: "↺"
                                color: "#a4acb7"
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: player.setAudioDelay(0)
                        }
                    }
                }
            }
        }
    }

    // SubtitleMenuButton — single-column port of the QWidget SubtitleMenu
    // (src/chrome/SubtitleMenu.cpp): an Off row + embedded track rows + a sync
    // footer. The two-column language rail is deferred to a real multi-sub file.
    component SubtitleMenuButton: Item {
        id: sbRoot
        implicitWidth: 48
        implicitHeight: 48
        width: 48
        height: 48

        readonly property bool subOn: {
            var list = player.subtitleTracks
            for (var i = 0; i < list.length; i++)
                if (list[i].selected === true) return true
            return false
        }

        function trackTitle(t) {
            if (t.title && t.title.trim() !== "") return t.title
            return t.external ? "External subtitle" : "Embedded track"
        }
        function trackMeta(t) {
            var parts = []
            if (t.lang && t.lang.trim() !== "") parts.push(t.lang.toUpperCase())
            parts.push(t.external ? "External" : "Embedded")
            if (t.codec && t.codec.trim() !== "") parts.push(t.codec.toUpperCase())
            if (t.forced) parts.push("FORCED")
            if (t.hearingImpaired) parts.push("HI/SDH")
            if (t.isDefault) parts.push("DEFAULT")
            return parts.join("   /   ")
        }

        Button {
            id: subTrigger
            width: 48
            height: 48
            flat: true
            hoverEnabled: true
            scale: down ? 0.95 : 1
            Behavior on scale { NumberAnimation { duration: 80 } }
            background: Rectangle {
                radius: width / 2
                color: subPopup.opened ? "#38ffffff" : (subTrigger.hovered ? "#1affffff" : "#00000000")
            }
            contentItem: Item {
                IconGlyph {
                    anchors.fill: parent
                    kind: "subtitle"
                    iconColor: (subPopup.opened || subTrigger.hovered) ? "#ffffff" : "#e6ffffff"
                }
                // emerald active dot when a subtitle is on
                Rectangle {
                    visible: sbRoot.subOn
                    width: 6
                    height: 6
                    radius: 3
                    color: "#34d399"
                    x: parent.width - 14
                    y: 10
                }
            }
            onClicked: subPopup.opened ? subPopup.close() : subPopup.open()
        }

        Popup {
            id: subPopup
            readonly property int trackCount: player.subtitleTracks.length
            readonly property int listH: trackCount > 0 ? Math.min(220, trackCount * 48 + 8) : 56
            width: 380
            height: 48 + 8 + 34 + 6 + listH + 40
            x: subTrigger.width - width
            y: -height - 10
            padding: 0
            closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside | Popup.CloseOnEscape

            background: Rectangle {
                radius: 16
                color: "#f7111419"
                border.color: "#16ffffff"
                border.width: 1
            }

            contentItem: Item {
                // Header
                Item {
                    width: parent.width
                    height: 48
                    Text {
                        id: subHeaderText
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Subtitles"
                        color: "#f5f7fa"
                        font.family: "Inter"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    Text {
                        anchors.left: subHeaderText.right
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        visible: subPopup.trackCount > 0
                        text: subPopup.trackCount
                        color: "#a4acb7"
                        font.family: "Inter"
                        font.pixelSize: 12
                    }
                    Button {
                        id: subClose
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: 28
                        height: 28
                        flat: true
                        hoverEnabled: true
                        background: Rectangle { radius: 14; color: subClose.hovered ? "#12ffffff" : "#00000000" }
                        contentItem: Item {
                            Rectangle { anchors.centerIn: parent; width: 12; height: 1.6; radius: 1; color: "#a4acb7"; rotation: 45 }
                            Rectangle { anchors.centerIn: parent; width: 12; height: 1.6; radius: 1; color: "#a4acb7"; rotation: -45 }
                        }
                        onClicked: subPopup.close()
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#12ffffff" }
                }

                // Off row
                Rectangle {
                    id: offRow
                    y: 56
                    x: 8
                    width: parent.width - 16
                    height: 34
                    radius: 6
                    color: !sbRoot.subOn ? "#1affffff" : (offHover.containsMouse ? "#0affffff" : "#00000000")
                    border.color: !sbRoot.subOn ? "#16ffffff" : "#00000000"
                    border.width: 1

                    Rectangle {
                        id: offDot
                        x: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: 16
                        height: 16
                        radius: 8
                        color: !sbRoot.subOn ? page.gold : "#12ffffff"
                        Text {
                            anchors.centerIn: parent
                            visible: !sbRoot.subOn
                            text: "✓"
                            color: "#13151b"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                    Text {
                        anchors.left: offDot.right
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Off"
                        color: !sbRoot.subOn ? "#f5f7fa" : "#a4acb7"
                        font.family: "Inter"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }
                    MouseArea {
                        id: offHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { player.setSubtitleTrack(""); subPopup.close() }
                    }
                }

                // Track list / empty state
                Item {
                    y: 96
                    width: parent.width
                    height: subPopup.listH

                    Text {
                        visible: subPopup.trackCount === 0
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.top: parent.top
                        anchors.topMargin: 8
                        width: parent.width - 32
                        wrapMode: Text.WordWrap
                        text: "No embedded subtitles in this file."
                        color: "#838b97"
                        font.family: "Inter"
                        font.pixelSize: 13
                    }

                    ListView {
                        visible: subPopup.trackCount > 0
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        anchors.topMargin: 2
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds
                        model: player.subtitleTracks
                        delegate: Rectangle {
                            id: sbRow
                            required property var modelData
                            width: ListView.view.width
                            height: 46
                            radius: 8
                            property bool sel: modelData.selected === true
                            color: sel ? "#1affffff" : (sbRowHover.containsMouse ? "#0affffff" : "#00000000")
                            border.color: sel ? "#16ffffff" : "#00000000"
                            border.width: 1

                            Rectangle {
                                id: sbDot
                                x: 10
                                y: 15
                                width: 16
                                height: 16
                                radius: 8
                                color: sbRow.sel ? page.gold : "#12ffffff"
                                Text {
                                    anchors.centerIn: parent
                                    visible: sbRow.sel
                                    text: "✓"
                                    color: "#13151b"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }
                            Text {
                                anchors.left: sbDot.right
                                anchors.leftMargin: 11
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                y: 7
                                text: sbRoot.trackTitle(sbRow.modelData)
                                elide: Text.ElideRight
                                color: "#f5f7fa"
                                font.family: "Inter"
                                font.pixelSize: 13
                                font.weight: Font.Medium
                            }
                            Text {
                                anchors.left: sbDot.right
                                anchors.leftMargin: 11
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                y: 25
                                text: sbRoot.trackMeta(sbRow.modelData)
                                elide: Text.ElideRight
                                color: "#a4acb7"
                                font.family: "Inter"
                                font.pixelSize: 11
                            }
                            MouseArea {
                                id: sbRowHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { player.setSubtitleTrack(sbRow.modelData.id); subPopup.close() }
                            }
                        }
                    }
                }

                // Sync-delay footer
                Item {
                    y: subPopup.height - 40
                    width: parent.width
                    height: 40
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#12ffffff" }
                    RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 12
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6
                        Text {
                            text: "SYNC"
                            color: "#838b97"
                            font.family: "Inter"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        DelayStepButton {
                            text: "-0.1"
                            onClicked: player.setSubDelay(Math.round((player.subDelaySec - 0.1) * 100) / 100)
                        }
                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: (player.subDelaySec >= 0 ? "+" : "") + player.subDelaySec.toFixed(2) + "s"
                            color: "#f5f7fa"
                            font.family: "Consolas"
                            font.pixelSize: 13
                        }
                        DelayStepButton {
                            text: "+0.1"
                            onClicked: player.setSubDelay(Math.round((player.subDelaySec + 0.1) * 100) / 100)
                        }
                        Button {
                            id: subReset
                            visible: Math.abs(player.subDelaySec) > 0.0001
                            flat: true
                            hoverEnabled: true
                            implicitWidth: 24
                            implicitHeight: 24
                            background: Rectangle { radius: 12; color: subReset.hovered ? "#12ffffff" : "#00000000" }
                            contentItem: Text {
                                text: "↺"
                                color: "#a4acb7"
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: player.setSubDelay(0)
                        }
                    }
                }
            }
        }
    }
}

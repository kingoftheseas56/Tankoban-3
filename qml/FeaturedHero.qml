import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Faithful Qt Quick recreation of TB3's Widgets FeaturedHero (src/ui/FeaturedHero.cpp):
// the large rotating billboard at the top of Home. Backdrop cross-fade (0.9 opacity,
// 700ms), art-melt scrims (horizontal + bottom 40%), rank pill, serif title, 3-line
// synopsis, Year/IMDb/Runtime stats, Play + Add buttons, 13s auto-advance that pauses
// on hover, drag/flick to change slides, and wide-pill dots (active 48 / rest 24).
Item {
    id: root
    implicitHeight: 560 + 20 + 8   // panel + gap + dots
    property int currentIndex: 0

    readonly property color canvas:  "#121317"
    readonly property color gold:    "#e8b923"
    readonly property color ink:     "#f3f1ea"
    readonly property color muted:   "#8b909b"

    // Demo featured slides (real metahub backdrops + plausible metadata).
    ListModel {
        id: slides
        ListElement { imdb: "tt1375666"; title: "Inception";        rank: "Movies"; pos: 1; year: "2010"; rating: "8.8"; runtime: "2h 28m"; synopsis: "A thief who steals corporate secrets through dream-sharing technology is given the inverse task of planting an idea into the mind of a C.E.O." }
        ListElement { imdb: "tt0468569"; title: "The Dark Knight";  rank: "Movies"; pos: 2; year: "2008"; rating: "9.0"; runtime: "2h 32m"; synopsis: "When the menace known as the Joker wreaks havoc on the people of Gotham, Batman must accept one of the greatest psychological and physical tests of his ability to fight injustice." }
        ListElement { imdb: "tt0816692"; title: "Interstellar";     rank: "Movies"; pos: 3; year: "2014"; rating: "8.7"; runtime: "2h 49m"; synopsis: "A team of explorers travel through a wormhole in space in an attempt to ensure humanity's survival as Earth becomes uninhabitable." }
        ListElement { imdb: "tt0133093"; title: "The Matrix";       rank: "Movies"; pos: 4; year: "1999"; rating: "8.7"; runtime: "2h 16m"; synopsis: "A computer hacker learns from mysterious rebels about the true nature of his reality and his role in the war against its controllers." }
        ListElement { imdb: "tt0167260"; title: "The Lord of the Rings: The Return of the King"; rank: "Movies"; pos: 5; year: "2003"; rating: "9.0"; runtime: "3h 21m"; synopsis: "Gandalf and Aragorn lead the World of Men against Sauron's army to draw his gaze from Frodo and Sam as they approach Mount Doom with the One Ring." }
    }

    function next() { currentIndex = (currentIndex + 1) % slides.count }
    function prev() { currentIndex = (currentIndex - 1 + slides.count) % slides.count }

    // 13s auto-advance, paused on hover (FeaturedHero m_autoTimer + onHoverEnter/Leave).
    Timer {
        interval: 13000; repeat: true
        running: slides.count > 1 && !panelHover.hovered
        onTriggered: root.next()
    }

    // ── The billboard panel ─────────────────────────────────────────
    Rectangle {
        id: panel
        width: parent.width
        height: 560
        radius: 28
        color: root.canvas
        clip: true

        // Backdrop cross-fade: all slides stacked, only the active at 0.9 opacity.
        Repeater {
            model: slides
            Image {
                anchors.fill: parent
                source: "https://images.metahub.space/background/large/" + model.imdb + "/img"
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                opacity: index === root.currentIndex ? 0.9 : 0.0
                Behavior on opacity { NumberAnimation { duration: 700; easing.type: Easing.OutCubic } }
            }
        }

        // Horizontal art-melt scrim: canvas → canvas@85% (mid) → transparent.
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#121317" }
                GradientStop { position: 0.5; color: "#d9121317" }
                GradientStop { position: 1.0; color: "#00121317" }
            }
        }
        // Bottom melt over the lower 40%.
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: parent.height * 0.4
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: "#00121317" }
                GradientStop { position: 0.5; color: "#b2121317" }
                GradientStop { position: 1.0; color: "#121317" }
            }
        }

        // Drag / flick to change slides (HeroPanel drag gesture). Taps fall through to buttons.
        DragHandler {
            target: null
            yAxis.enabled: false
            xAxis.enabled: true
            onActiveChanged: if (!active) {
                var dx = centroid.position.x - centroid.pressPosition.x
                if (dx < -60) root.next()
                else if (dx > 60) root.prev()
            }
        }
        HoverHandler { id: panelHover }

        // ── Content (left, cross-fades on slide change) ─────────────
        Item {
            id: content
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 56
            width: Math.min(620, parent.width - 112)
            height: contentCol.implicitHeight

            Connections {
                target: root
                function onCurrentIndexChanged() { contentFade.restart() }
            }
            NumberAnimation { id: contentFade; target: content; property: "opacity"; from: 0; to: 1; duration: 450; easing.type: Easing.OutCubic }

            Column {
                id: contentCol
                width: parent.width
                spacing: 0

                // Rank pill
                Rectangle {
                    width: rankRow.implicitWidth + 24; height: 30; radius: 15
                    color: "#1e1f26"; border.color: "#2c2d36"; border.width: 1
                    Row {
                        id: rankRow
                        anchors.centerIn: parent
                        spacing: 6
                        Label { text: "↗"; color: root.gold; font.pixelSize: 14; font.bold: true }
                        Label {
                            text: "#" + slides.get(root.currentIndex).pos + " in " + slides.get(root.currentIndex).rank + " Today"
                            color: root.ink; font.pixelSize: 13; font.weight: Font.Medium
                        }
                    }
                }
                Item { width: 1; height: 20 }

                // Serif title
                Label {
                    width: parent.width
                    text: slides.get(root.currentIndex).title
                    color: root.ink
                    font.family: "Georgia"
                    font.pixelSize: 54
                    font.bold: true
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    lineHeight: 1.02
                }
                Item { width: 1; height: 22 }

                // 3-line synopsis
                Label {
                    width: Math.min(560, parent.width)
                    text: slides.get(root.currentIndex).synopsis
                    color: "#c3c7d0"
                    font.pixelSize: 16
                    lineHeight: 1.35
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }
                Item { width: 1; height: 22 }

                // Stat line: Year · IMDb · Runtime
                Row {
                    spacing: 26
                    Row { spacing: 6
                        Label { text: "Year:"; color: root.muted; font.pixelSize: 14 }
                        Label { text: slides.get(root.currentIndex).year; color: root.ink; font.pixelSize: 14 }
                    }
                    Row { spacing: 6
                        Label { text: "IMDb"; color: root.gold; font.pixelSize: 14; font.bold: true }
                        Label { text: slides.get(root.currentIndex).rating; color: root.ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                    }
                    Row { spacing: 6
                        Label { text: "Runtime:"; color: root.muted; font.pixelSize: 14 }
                        Label { text: slides.get(root.currentIndex).runtime; color: root.ink; font.pixelSize: 14 }
                    }
                }
                Item { width: 1; height: 34 }

                // Play + Add buttons
                Row {
                    spacing: 12
                    // Play (filled)
                    Rectangle {
                        width: 130; height: 48; radius: 8
                        color: playMA.containsMouse ? "#ffffff" : root.ink
                        Row {
                            anchors.centerIn: parent; spacing: 8
                            Label { text: "▶"; color: "#121317"; font.pixelSize: 15 }
                            Label { text: "Play"; color: "#121317"; font.pixelSize: 15; font.bold: true }
                        }
                        MouseArea { id: playMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
                    }
                    // Add to Watchlist (outline, toggles)
                    Rectangle {
                        id: addBtn
                        property bool inWatch: false
                        width: 220; height: 48; radius: 8
                        color: addMA.containsMouse ? "#1c2433" : "transparent"
                        Rectangle { anchors.fill: parent; radius: 8; color: "transparent"; border.color: "#55ffffff"; border.width: 1 }
                        Label {
                            anchors.centerIn: parent
                            text: addBtn.inWatch ? "✓  In Watchlist" : "＋  Add to Watchlist"
                            color: root.ink; font.pixelSize: 15; font.weight: Font.Medium
                        }
                        MouseArea {
                            id: addMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: addBtn.inWatch = !addBtn.inWatch
                        }
                    }
                }
            }
        }
    }

    // ── Wide-pill dots (active 48 gold / rest 24 muted) ─────────────
    Row {
        anchors.top: panel.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
        Repeater {
            model: slides
            Rectangle {
                width: index === root.currentIndex ? 48 : 24
                height: 6; radius: 3
                color: index === root.currentIndex ? root.gold : "#40ffffff"
                Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                Behavior on color { ColorAnimation { duration: 180 } }
                MouseArea {
                    anchors.fill: parent; anchors.margins: -6
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.currentIndex = index
                }
            }
        }
    }
}

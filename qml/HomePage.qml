import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

// Home: a faithful Qt Quick recreation of TB3's Widgets CatalogRow + PosterCard
// carousel (src/ui/CatalogRow.cpp). Matched 1:1: 144x216 cards with titles,
// hover-lift(8) + drop shadow, hidden scrollbar, "View all" header, 44px hover
// edge-arrows with paged glide (viewport*0.85, 300ms OutCubic), snap-to-stride,
// and DRAG-TO-PAN WITH MOMENTUM — which the Widgets version hand-rolled in a
// 60-line eventFilter but a Quick ListView gives for free (flick is the default).
Item {
    id: page
    property string crumb: "Home"
    signal openSeries()

    // Harbor/TB3 card geometry constants (PosterCard.h).
    readonly property int kPosterW: 144
    readonly property int kPosterH: 216
    readonly property int kHoverLift: 8
    readonly property int kGap: 20

    Flickable {
        anchors.fill: parent
        contentHeight: col.implicitHeight
        clip: true

        Column {
            id: col
            width: parent.width
            spacing: 28

            // ── FeaturedHero: the rotating billboard carousel ───────
            Item { width: 1; height: 8 }
            FeaturedHero {
                width: parent.width
            }

            // ── CatalogRow: header (title + View all) ───────────────
            Item {
                width: parent.width
                height: 28
                Label {
                    x: 40
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Trending"
                    color: "#f3f1ea"; font.pixelSize: 22; font.bold: true
                }
                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 40
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    Label {
                        id: viewAll
                        text: "View all"
                        color: vaHover.hovered ? "#f3f1ea" : "#8b8f99"
                        font.pixelSize: 13; font.weight: Font.Medium
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    Label { text: "›"; color: viewAll.color; font.pixelSize: 15 }
                    HoverHandler { id: vaHover }
                    TapHandler { onTapped: page.openSeries() }
                }
            }

            // ── CatalogRow: the track (carousel) ────────────────────
            Item {
                id: railBox
                width: parent.width
                height: page.kHoverLift + page.kPosterH + 10 + 38   // matches scroll fixedHeight

                function pageBy(dir) {
                    var step = (rail.width) * 0.85               // CatalogRow scrollByPage
                    var maxX = Math.max(0, rail.contentWidth - rail.width)
                    scrollAnim.stop()
                    scrollAnim.from = rail.contentX
                    scrollAnim.to = Math.max(0, Math.min(maxX, rail.contentX + dir * step))
                    scrollAnim.start()
                }

                ListView {
                    id: rail
                    anchors.fill: parent
                    orientation: ListView.Horizontal
                    spacing: page.kGap
                    leftMargin: 40
                    rightMargin: 40
                    clip: true
                    // Drag-to-pan + momentum is the DEFAULT here (interactive Flickable).
                    // SnapToItem gives the release-snap the Widgets code computed by hand.
                    snapMode: ListView.SnapToItem
                    boundsBehavior: Flickable.StopAtBounds
                    flickDeceleration: 2400
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AlwaysOff } // hidden, 1:1 Harbor

                    NumberAnimation {
                        id: scrollAnim
                        target: rail; property: "contentX"
                        duration: 300; easing.type: Easing.OutCubic
                    }
                    WheelHandler {
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                        onWheel: (event) => {
                            var d = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
                            var maxX = Math.max(0, rail.contentWidth - rail.width)
                            rail.contentX = Math.max(0, Math.min(maxX, rail.contentX - d))
                        }
                    }

                    model: ListModel {
                        ListElement { imdb: "tt0468569"; name: "The Dark Knight" }
                        ListElement { imdb: "tt1375666"; name: "Inception" }
                        ListElement { imdb: "tt0111161"; name: "The Shawshank Redemption" }
                        ListElement { imdb: "tt0137523"; name: "Fight Club" }
                        ListElement { imdb: "tt0109830"; name: "Forrest Gump" }
                        ListElement { imdb: "tt0110912"; name: "Pulp Fiction" }
                        ListElement { imdb: "tt0133093"; name: "The Matrix" }
                        ListElement { imdb: "tt0167260"; name: "The Lord of the Rings: The Return of the King" }
                        ListElement { imdb: "tt0816692"; name: "Interstellar" }
                        ListElement { imdb: "tt0080684"; name: "Star Wars: The Empire Strikes Back" }
                        ListElement { imdb: "tt0114369"; name: "Se7en" }
                        ListElement { imdb: "tt0102926"; name: "The Silence of the Lambs" }
                        ListElement { imdb: "tt0317248"; name: "City of God" }
                        ListElement { imdb: "tt0120737"; name: "The Lord of the Rings: The Fellowship of the Ring" }
                        ListElement { imdb: "tt0245429"; name: "Spirited Away" }
                        ListElement { imdb: "tt4154796"; name: "Avengers: Endgame" }
                        ListElement { imdb: "tt0076759"; name: "Star Wars: A New Hope" }
                        ListElement { imdb: "tt0099685"; name: "Goodfellas" }
                        ListElement { imdb: "tt0073486"; name: "One Flew Over the Cuckoo's Nest" }
                        ListElement { imdb: "tt0047478"; name: "Seven Samurai" }
                    }

                    // PosterCard: poster + 2-line title, hover-lift(8) + drop shadow.
                    delegate: Item {
                        width: page.kPosterW
                        height: page.kHoverLift + page.kPosterH + 10 + 38

                        Column {
                            id: cardInner
                            width: parent.width
                            y: page.kHoverLift                 // resting baseline (room to lift)
                            spacing: 10
                            transform: Translate { y: cardHover.hovered ? -page.kHoverLift : 0
                                Behavior on y { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } } }

                            Rectangle {
                                id: posterBox
                                width: page.kPosterW; height: page.kPosterH
                                radius: 8; clip: true; color: "#172033"
                                border.color: "#222c40"; border.width: 1

                                Label {
                                    anchors.centerIn: parent
                                    visible: poster.status !== Image.Ready
                                    text: poster.status === Image.Loading ? "…" : ""
                                    color: "#3c4660"; font.pixelSize: 22
                                }
                                Image {
                                    id: poster
                                    anchors.fill: parent
                                    source: "https://images.metahub.space/poster/medium/" + model.imdb + "/img"
                                    asynchronous: true; cache: true
                                    fillMode: Image.PreserveAspectCrop
                                    sourceSize.width: page.kPosterW; sourceSize.height: page.kPosterH
                                    opacity: status === Image.Ready ? 1 : 0
                                    Behavior on opacity { NumberAnimation { duration: 200 } }
                                }
                                // Soft drop shadow, enabled only on hover (PosterCard m_shadow).
                                layer.enabled: cardHover.hovered
                                layer.effect: MultiEffect {
                                    shadowEnabled: true
                                    shadowColor: "#aa000000"
                                    shadowBlur: 0.7
                                    shadowVerticalOffset: 6
                                }
                            }
                            Label {
                                width: page.kPosterW
                                text: model.name
                                color: cardHover.hovered ? "#f3f1ea" : "#aab2c5"
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                        }
                        HoverHandler { id: cardHover }
                        TapHandler { onTapped: page.openSeries() }   // tap activates; drag pans (Flickable wins)
                    }
                }

                HoverHandler { id: railHover }

                // ── Hover edge-arrows (44px circular, Harbor) ───────
                component Arrow : Rectangle {
                    property string glyph
                    property bool show
                    width: 44; height: 44; radius: 22
                    color: aHover.hovered ? "#e7c66b" : "#cc11141b"
                    // Vertically centered on the POSTER (matches ay = lift + posterH/2 - 22).
                    y: page.kHoverLift + page.kPosterH / 2 - 22
                    opacity: show ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 160 } }
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Label {
                        anchors.centerIn: parent
                        text: parent.glyph
                        color: aHover.hovered ? "#11141b" : "#f3f1ea"
                        font.pixelSize: 22; font.bold: true
                    }
                    HoverHandler { id: aHover }
                }

                Arrow {
                    glyph: "‹"
                    x: 8
                    show: railHover.hovered && rail.contentX > 2
                    TapHandler { onTapped: railBox.pageBy(-1) }
                }
                Arrow {
                    glyph: "›"
                    x: parent.width - 44 - 8
                    show: railHover.hovered && rail.contentX < (rail.contentWidth - rail.width - 2)
                    TapHandler { onTapped: railBox.pageBy(1) }
                }
            }
            Item { width: 1; height: 16 }
        }
    }
}

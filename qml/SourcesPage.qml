import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Sources: the text-heavy, multi-line list that fought us hardest in Widgets
// (real Torrentio rows are variable-height: name + seeders/size + language).
// In Quick a ListView with a variable-height delegate is the default, not a
// fight. Clicking a row advances to the player. (Increment 2 swaps this fake
// model for the real QAbstractListModel the backend already exposes.)
Item {
    id: page
    property string crumb: "Series  ▸  Sources"
    signal play()

    // Fake multi-line source rows — same SHAPE as real Torrentio/Stremio labels.
    ListModel {
        id: sources
        ListElement { provider: "Torrentio";  quality: "1080p"; title: "Spider-Man.Noir.2024.1080p.BluRay.x264";  meta: "👤 412 · 💾 2.1 GB · 🌐 English" }
        ListElement { provider: "Torrentio";  quality: "2160p"; title: "Spider-Man.Noir.2024.2160p.UHD.HDR.x265"; meta: "👤 198 · 💾 8.4 GB · 🌐 English" }
        ListElement { provider: "Comet";      quality: "1080p"; title: "Spider.Man.Noir.S01.COMPLETE.1080p.WEB"; meta: "👤 77 · 💾 5.0 GB · 🌐 Multi" }
        ListElement { provider: "ThePirateBay"; quality: "720p"; title: "SpiderManNoir.720p.WEBRip.AAC";          meta: "👤 31 · 💾 1.1 GB · 🌐 English" }
        ListElement { provider: "Jackett";    quality: "1080p"; title: "Spider-Man Noir (2024) 1080p AMZN WEB-DL"; meta: "👤 12 · 💾 3.3 GB · 🌐 English · 🇪🇸 sub" }
    }

    ListView {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 10
        clip: true
        model: sources

        header: Label {
            text: "Sources"
            color: "#e8ecf5"; font.pixelSize: 24; font.bold: true
            bottomPadding: 14
        }

        delegate: Rectangle {
            width: ListView.view.width
            // Variable height driven by content — the thing that broke the
            // Widgets delegate (it assumed fixed-height/single-line).
            height: rowCol.implicitHeight + 24
            radius: 10
            color: rowHover.hovered ? "#1b2740" : "#141b2b"
            border.color: rowHover.hovered ? "#e7c66b" : "#222c40"
            border.width: 1

            Column {
                id: rowCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 4

                Row {
                    spacing: 10
                    Label { id: provLabel; text: model.provider; color: "#e7c66b"; font.pixelSize: 14; font.bold: true }
                    Label { text: model.quality; color: "#7fd0a0"; font.pixelSize: 13 }
                }
                Label {
                    width: rowCol.width
                    text: model.title
                    color: "#dfe5f0"; font.pixelSize: 15
                    elide: Text.ElideRight
                }
                Label { text: model.meta; color: "#8a93a8"; font.pixelSize: 13 }
            }

            HoverHandler { id: rowHover }
            TapHandler { onTapped: page.play() }
        }
    }
}

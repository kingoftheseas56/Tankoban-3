import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Series View: image-forward detail layout (poster + synopsis + action).
// Tests the navigation-in transition and a detail composition. Clicking
// "Find Sources" advances toward the player — the seam that froze us in Widgets.
Item {
    id: page
    property string crumb: "Series  ▸  Spider-Man: Noir"
    signal openSources()

    Rectangle { anchors.fill: parent; color: "#0b0d12" }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 48
        spacing: 36

        // Poster
        Rectangle {
            Layout.preferredWidth: 300
            Layout.preferredHeight: 440
            Layout.alignment: Qt.AlignTop
            radius: 14
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2a2030" }
                GradientStop { position: 1.0; color: "#141a26" }
            }
            border.color: "#2a3450"; border.width: 1
            Label {
                anchors.centerIn: parent
                text: "POSTER"; color: "#5b6680"
                font.pixelSize: 20; font.bold: true
            }
        }

        // Meta column
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 18

            Label { text: "Spider-Man: Noir"; color: "#e8ecf5"; font.pixelSize: 38; font.bold: true }
            Label { text: "2024  ·  Action, Crime  ·  ★ 8.1"; color: "#8a93a8"; font.pixelSize: 15 }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "A hard-boiled private investigator in 1930s New York gains "
                    + "spider powers and wages a one-man war on crime. This screen "
                    + "exists to prove the detail layout + the navigation transition "
                    + "render natively in Qt Quick before we reach the player seam."
                color: "#aab2c5"; font.pixelSize: 16; lineHeight: 1.3
            }
            Button {
                text: "🔎  Find Sources"
                onClicked: page.openSources()
            }
        }
    }
}

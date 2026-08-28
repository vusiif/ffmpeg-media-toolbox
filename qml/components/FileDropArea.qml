import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

DropArea {
    id: root

    property string currentFile: ""
    property var mediaInfo: null

    onDropped: (drop) => {
        if (drop.hasUrls) {
            var url = drop.urls[0]
            currentFile = url.toString().replace("file:///", "")
        }
    }

    Rectangle {
        anchors.fill: parent
        color: parent.containsDrag ? Qt.rgba(0, 0.5, 1, 0.1) : "transparent"
        border.color: parent.containsDrag ? "#0078d4" : palette.mid
        border.width: parent.containsDrag ? 2 : 1
        radius: 8

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 8

            Label {
                text: "📁"
                font.pixelSize: 48
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTrId("home.dropHint")
                font.pixelSize: 14
                color: palette.placeholderText
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}

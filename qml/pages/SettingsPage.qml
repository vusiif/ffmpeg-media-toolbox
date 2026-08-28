import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: settingsPage

    

    

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTrId("settings.title")
            font.pixelSize: 24
            font.weight: Font.Bold
        }

        GroupBox {
            title: qsTrId("ffmpeg.status")
            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent
                columnSpacing: 16
                rowSpacing: 8

                Label { text: qsTrId("ffmpeg.ffmpegPath"); font.weight: Font.Medium }
                Label {
                    text: ffmpegLocator.ffmpegPath || qsTrId("Not found")
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                Label { text: qsTrId("ffmpeg.ffprobePath"); font.weight: Font.Medium }
                Label {
                    text: ffmpegLocator.ffprobePath || qsTrId("Not found")
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                Label { text: qsTrId("ffmpeg.version"); font.weight: Font.Medium }
                Label {
                    text: ffmpegLocator.ffmpegVersion || qsTrId("Unknown")
                    Layout.fillWidth: true
                }

                Item {}
                Button {
                    text: qsTrId("ffmpeg.rescan")
                    onClicked: {
                        ffmpegLocator.autoDetect()
                        ffmpegCaps.scan()
                    }
                }
            }
        }

        GroupBox {
            title: qsTrId("settings.language")
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 16

                Label { text: qsTrId("settings.language") + ":" }

                ComboBox {
                    model: ["English", "中文"]
                    currentIndex: Qt.uiLanguage.startsWith("zh") ? 1 : 0
                    onActivated: (index) => {
                        Qt.uiLanguage = index === 0 ? "en" : "zh_CN"
                    }
                    Layout.fillWidth: true
                }
            }
        }

        GroupBox {
            title: qsTrId("Capabilities")
            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent
                columnSpacing: 16
                rowSpacing: 8

                Label { text: qsTrId("Formats:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.formatCount : qsTrId("Not scanned") }

                Label { text: qsTrId("Encoders:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.encoderCount : qsTrId("Not scanned") }

                Label { text: qsTrId("Decoders:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.decoderCount : qsTrId("Not scanned") }

                Label { text: qsTrId("Filters:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.filterCount : qsTrId("Not scanned") }

                Label { text: qsTrId("HW Accels:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.hwAccelCount : qsTrId("Not scanned") }

                Item {}
                Button {
                    text: qsTrId("Scan Capabilities")
                    onClicked: ffmpegCaps.scan()
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}


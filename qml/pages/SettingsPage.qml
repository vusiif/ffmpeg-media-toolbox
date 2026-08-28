import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: settingsPage

    function t(key) { return t(key, i18n.languageVersion) }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: t("settings.title")
            font.pixelSize: 24
            font.weight: Font.Bold
        }

        GroupBox {
            title: t("ffmpeg.status")
            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent
                columnSpacing: 16
                rowSpacing: 8

                Label { text: t("ffmpeg.ffmpegPath"); font.weight: Font.Medium }
                Label {
                    text: ffmpegLocator.ffmpegPath || qsTr("Not found")
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                Label { text: t("ffmpeg.ffprobePath"); font.weight: Font.Medium }
                Label {
                    text: ffmpegLocator.ffprobePath || qsTr("Not found")
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                Label { text: t("ffmpeg.version"); font.weight: Font.Medium }
                Label {
                    text: ffmpegLocator.ffmpegVersion || qsTr("Unknown")
                    Layout.fillWidth: true
                }

                Item {}
                Button {
                    text: t("ffmpeg.rescan")
                    onClicked: {
                        ffmpegLocator.autoDetect()
                        ffmpegCaps.scan()
                    }
                }
            }
        }

        GroupBox {
            title: t("settings.language")
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 16

                Label { text: t("settings.language") + ":" }

                ComboBox {
                    model: ["English", "中文"]
                    currentIndex: i18n.currentLanguage === "zh" ? 1 : 0
                    onActivated: (index) => {
                        i18n.setLanguage(index === 0 ? "en" : "zh")
                    }
                    Layout.fillWidth: true
                }
            }
        }

        GroupBox {
            title: qsTr("Capabilities")
            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent
                columnSpacing: 16
                rowSpacing: 8

                Label { text: qsTr("Formats:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.formatCount : qsTr("Not scanned") }

                Label { text: qsTr("Encoders:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.encoderCount : qsTr("Not scanned") }

                Label { text: qsTr("Decoders:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.decoderCount : qsTr("Not scanned") }

                Label { text: qsTr("Filters:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.filterCount : qsTr("Not scanned") }

                Label { text: qsTr("HW Accels:"); font.weight: Font.Medium }
                Label { text: ffmpegCaps.loaded ? ffmpegCaps.hwAccelCount : qsTr("Not scanned") }

                Item {}
                Button {
                    text: qsTr("Scan Capabilities")
                    onClicked: ffmpegCaps.scan()
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}


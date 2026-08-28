import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qml/pages" as Pages

ApplicationWindow {
    id: root

    visible: true
    width: 1024
    height: 680
    minimumWidth: 800
    minimumHeight: 600
    title: i18n.tr("app.title")

    color: palette.window

    property string currentPage: "home"

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            Label {
                text: root.title
                font.pixelSize: 16
                font.weight: Font.Medium
                elide: Label.ElideRight
                Layout.fillWidth: true
                leftPadding: 12
            }

            ComboBox {
                id: langCombo
                model: ["English", "中文"]
                currentIndex: i18n.currentLanguage === "zh" ? 1 : 0
                onActivated: (index) => {
                    i18n.setLanguage(index === 0 ? "en" : "zh")
                }
                Layout.rightMargin: 12
            }
        }
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 0

            Repeater {
                model: [
                    { icon: "🏠", label: "nav.home", page: "home" },
                    { icon: "🔄", label: "nav.convert", page: "convert" },
                    { icon: "🖼️", label: "nav.image", page: "image" },
                    { icon: "🔗", label: "image.join", page: "join" },
                    { icon: "📋", label: "nav.queue", page: "queue" }
                ]

                delegate: AbstractButton {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    Layout.preferredHeight: 48

                    checked: root.currentPage === modelData.page
                    onClicked: root.currentPage = modelData.page

                    contentItem: ColumnLayout {
                        spacing: 2

                        Label {
                            text: modelData.icon
                            font.pixelSize: 18
                            horizontalAlignment: Text.AlignHCenter
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Label {
                            text: i18n.tr(modelData.label)
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            Layout.alignment: Qt.AlignHCenter
                            color: parent.parent.checked ? palette.highlight : palette.windowText
                        }
                    }

                    background: Rectangle {
                        color: parent.checked ? Qt.rgba(0, 0.5, 1, 0.1) :
                               parent.hovered ? Qt.rgba(0, 0, 0, 0.05) : "transparent"
                    }
                }
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: {
            switch(root.currentPage) {
            case "home": return 0
            case "convert": return 1
            case "image": return 2
            case "join": return 3
            case "queue": return 4
            default: return 0
            }
        }

        // Home page
        Page {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                Label {
                    text: i18n.tr("home.title")
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: i18n.tr("home.dropHint")
                    font.pixelSize: 14
                    color: palette.placeholderText
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }

                GroupBox {
                    title: i18n.tr("ffmpeg.status")
                    visible: ffmpegLocator.isValid
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 500

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        RowLayout {
                            Label {
                                text: i18n.tr("ffmpeg.version") + ":"
                                font.weight: Font.Medium
                            }
                            Label {
                                text: ffmpegLocator.ffmpegVersion
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        Button {
                            text: i18n.tr("ffmpeg.rescan")
                            onClicked: {
                                ffmpegLocator.autoDetect()
                                ffmpegCaps.scan()
                            }
                            Layout.alignment: Qt.AlignRight
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Convert page
        Pages.ConvertPage {}

        // Image page
        Pages.ImagePage {}

        // Join page
        Pages.JoinPage {}

        // Queue page
        Pages.QueuePage {}
    }
}

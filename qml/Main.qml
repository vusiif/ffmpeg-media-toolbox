import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root

    visible: true
    width: 1024
    height: 680
    minimumWidth: 800
    minimumHeight: 600
    title: i18n.tr("app.title")

    color: palette.window

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            Label {
                text: root.title
                font.pixelSize: 16
                font.weight: Font.Medium
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
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

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: homePage
    }

    Component {
        id: homePage

        Page {
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24

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
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 500

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Label {
                                text: i18n.tr("ffmpeg.found") + ":"
                                font.weight: Font.Medium
                            }
                            Label {
                                text: ffmpegLocator.isValid ? i18n.tr("ffmpeg.found") : i18n.tr("ffmpeg.notFound")
                                color: ffmpegLocator.isValid ? "green" : "red"
                            }
                        }

                        RowLayout {
                            visible: ffmpegLocator.isValid
                            Label {
                                text: i18n.tr("ffmpeg.ffmpegPath")
                                font.weight: Font.Medium
                            }
                            Label {
                                text: ffmpegLocator.ffmpegPath
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            visible: ffmpegLocator.isValid
                            Label {
                                text: i18n.tr("ffmpeg.ffprobePath")
                                font.weight: Font.Medium
                            }
                            Label {
                                text: ffmpegLocator.ffprobePath
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            visible: ffmpegLocator.isValid
                            Label {
                                text: i18n.tr("ffmpeg.version")
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
                            onClicked: ffmpegLocator.autoDetect()
                            Layout.alignment: Qt.AlignRight
                        }
                    }
                }
            }
        }
    }
}

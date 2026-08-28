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
    title: qsTr("FFmpeg Media Toolbox")

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
                    text: qsTr("FFmpeg Media Toolbox")
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: qsTr("Drop files here to get started")
                    font.pixelSize: 14
                    color: palette.placeholderText
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }

                GroupBox {
                    title: qsTr("FFmpeg Status")
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 500

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Label {
                                text: qsTr("Status:")
                                font.weight: Font.Medium
                            }
                            Label {
                                text: ffmpegLocator.isValid ? qsTr("Found") : qsTr("Not Found")
                                color: ffmpegLocator.isValid ? "green" : "red"
                            }
                        }

                        RowLayout {
                            visible: ffmpegLocator.isValid
                            Label {
                                text: qsTr("FFmpeg:")
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
                                text: qsTr("FFprobe:")
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
                                text: qsTr("Version:")
                                font.weight: Font.Medium
                            }
                            Label {
                                text: ffmpegLocator.ffmpegVersion
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        Button {
                            text: qsTr("Rescan")
                            onClicked: ffmpegLocator.autoDetect()
                            Layout.alignment: Qt.AlignRight
                        }
                    }
                }
            }
        }
    }
}

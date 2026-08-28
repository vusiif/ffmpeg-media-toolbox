import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import "qml/components" as Components

ApplicationWindow {
    id: root

    visible: true
    width: 1024
    height: 680
    minimumWidth: 800
    minimumHeight: 600
    title: i18n.tr("app.title")

    color: palette.window

    property string currentFile: ""
    property var mediaInfo: null

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

    FileDialog {
        id: fileDialog
        title: i18n.tr("common.open")
        nameFilters: ["All files (*)"]
        onAccepted: {
            root.currentFile = selectedFile.toString().replace("file:///", "")
            root.probeFile()
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: homePage
    }

    function probeFile() {
        if (currentFile === "") {
            mediaInfo = null
            return
        }
        mediaInfo = mediaProbe.probe(currentFile)
    }

    Component {
        id: homePage

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

                Components.FileDropArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150

                    onDropped: (drop) => {
                        if (drop.hasUrls) {
                            root.currentFile = drop.urls[0].toString().replace("file:///", "")
                            root.probeFile()
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: fileDialog.open()
                        cursorShape: Qt.PointingHandCursor
                    }
                }

                GroupBox {
                    title: i18n.tr("ffmpeg.status")
                    visible: ffmpegLocator.isValid
                    Layout.fillWidth: true

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

                GroupBox {
                    title: "Media Info"
                    visible: root.currentFile !== "" && root.mediaInfo !== null && root.mediaInfo.isValid()
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        RowLayout {
                            Label { text: "File:"; font.weight: Font.Medium }
                            Label { text: root.currentFile; elide: Text.ElideMiddle; Layout.fillWidth: true }
                        }

                        RowLayout {
                            Label { text: "Format:"; font.weight: Font.Medium }
                            Label { text: root.mediaInfo ? root.mediaInfo.formatName : "" }
                        }

                        RowLayout {
                            Label { text: "Duration:"; font.weight: Font.Medium }
                            Label {
                                text: root.mediaInfo ? formatDuration(root.mediaInfo.duration) : ""
                            }
                        }

                        RowLayout {
                            visible: root.mediaInfo && root.mediaInfo.hasVideo()
                            Label { text: "Video:"; font.weight: Font.Medium }
                            Label {
                                text: {
                                    if (!root.mediaInfo || !root.mediaInfo.hasVideo()) return ""
                                    var v = root.mediaInfo.primaryVideo()
                                    return v.codec.toUpperCase() + " " + v.width + "x" + v.height + " " + v.frameRate.toFixed(2) + "fps"
                                }
                            }
                        }

                        RowLayout {
                            visible: root.mediaInfo && root.mediaInfo.hasAudio()
                            Label { text: "Audio:"; font.weight: Font.Medium }
                            Label {
                                text: {
                                    if (!root.mediaInfo || !root.mediaInfo.hasAudio()) return ""
                                    var a = root.mediaInfo.primaryAudio()
                                    return a.codec.toUpperCase() + " " + a.sampleRate + "Hz " + a.channels + "ch"
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function formatDuration(seconds) {
        if (!seconds || seconds <= 0) return "0:00"
        var h = Math.floor(seconds / 3600)
        var m = Math.floor((seconds % 3600) / 60)
        var s = Math.floor(seconds % 60)
        if (h > 0) return h + ":" + String(m).padStart(2, '0') + ":" + String(s).padStart(2, '0')
        return m + ":" + String(s).padStart(2, '0')
    }
}

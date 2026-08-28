import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: joinPage

    property var inputFiles: []
    property string joinMode: "horizontal"
    property int spacing: 0
    property int padding: 0
    property string backgroundColor: "black"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: i18n.tr("image.join")
            font.pixelSize: 24
            font.weight: Font.Bold
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            GroupBox {
                title: i18n.tr("convert.input")
                Layout.preferredWidth: 300
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    ListView {
                        id: fileList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: joinPage.inputFiles

                        delegate: RowLayout {
                            width: fileList.width
                            spacing: 8

                            Image {
                                source: "file:///" + modelData
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                fillMode: Image.PreserveAspectFit
                            }

                            Label {
                                text: modelData.split("/").pop().split("\\").pop()
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Button {
                                text: "×"
                                onClicked: {
                                    var files = Array.from(joinPage.inputFiles)
                                    files.splice(index, 1)
                                    joinPage.inputFiles = files
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Button {
                            text: i18n.tr("common.browse")
                            onClicked: joinFileDialog.open()
                        }

                        Button {
                            text: i18n.tr("common.clear")
                            onClicked: joinPage.inputFiles = []
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Join Settings")
                Layout.fillWidth: true
                Layout.fillHeight: true

                GridLayout {
                    columns: 2
                    anchors.fill: parent
                    columnSpacing: 16
                    rowSpacing: 12

                    Label { text: qsTr("Mode:") }
                    ComboBox {
                        id: modeCombo
                        model: [
                            { text: qsTr("Horizontal"), value: "horizontal" },
                            { text: qsTr("Vertical"), value: "vertical" }
                        ]
                        textRole: "text"
                        currentIndex: 0
                        onCurrentIndexChanged: joinPage.joinMode = model[currentIndex].value
                        Layout.fillWidth: true
                    }

                    Label { text: qsTr("Spacing:") }
                    SpinBox {
                        from: 0; to: 100; value: joinPage.spacing
                        onValueModified: joinPage.spacing = value
                        Layout.fillWidth: true
                    }

                    Label { text: qsTr("Padding:") }
                    SpinBox {
                        from: 0; to: 100; value: joinPage.padding
                        onValueModified: joinPage.padding = value
                        Layout.fillWidth: true
                    }

                    Label { text: qsTr("Background:") }
                    ComboBox {
                        model: ["black", "white", "transparent"]
                        currentIndex: 0
                        onCurrentTextChanged: joinPage.backgroundColor = currentText
                        Layout.fillWidth: true
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 12

            Button {
                text: i18n.tr("convert.addToQueue")
                enabled: joinPage.inputFiles.length >= 2
                onClicked: {
                    if (joinPage.inputFiles.length < 2) return

                    var outputPath = joinPage.inputFiles[0].replace(/\.[^.]+$/, "_joined.png")

                    var args = ["-y"]
                    for (var i = 0; i < joinPage.inputFiles.length; i++) {
                        args.push("-i")
                        args.push(joinPage.inputFiles[i])
                    }

                    var filter = ""
                    if (joinPage.joinMode === "horizontal") {
                        filter = "hstack=inputs=" + joinPage.inputFiles.length
                    } else {
                        filter = "vstack=inputs=" + joinPage.inputFiles.length
                    }

                    if (joinPage.spacing > 0) {
                        filter = "pad=" + "iw+" + joinPage.spacing + ":ih+" + joinPage.spacing + ":" + (joinPage.spacing/2) + ":" + (joinPage.spacing/2) + ":color=" + joinPage.backgroundColor
                    }

                    args.push("-filter_complex")
                    args.push(filter)
                    args.push(outputPath)

                    var cmd = { program: ffmpegLocator.ffmpegPath, arguments: args }

                    var job = Qt.createQmlObject('import ffmedia; Job {}', joinPage)
                    job.name = "Join " + joinPage.inputFiles.length + " images"
                    job.command = cmd

                    jobQueue.addJob(job)
                }
            }
        }
    }

    FileDialog {
        id: joinFileDialog
        title: i18n.tr("image.join")
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.webp *.bmp *.tiff *.gif)", "All files (*)"]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            var newFiles = []
            for (var i = 0; i < selectedFiles.length; i++) {
                newFiles.push(selectedFiles[i].toString().replace("file:///", ""))
            }
            joinPage.inputFiles = newFiles
        }
    }
}

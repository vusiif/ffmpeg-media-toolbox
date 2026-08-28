import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: batchPage

    

    

    property var inputFiles: []
    property string outputFormat: "mp4"
    property string videoCodec: ""
    property string audioCodec: ""
    property int quality: 23
    property bool useSmartCopy: false
    property string outputDirectory: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("Batch Processing")
            font.pixelSize: 24
            font.weight: Font.Bold
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            GroupBox {
                title: qsTrId("convert.input")
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
                        model: batchPage.inputFiles

                        delegate: RowLayout {
                            width: fileList.width
                            spacing: 8

                            Label {
                                text: (index + 1) + "."
                                Layout.preferredWidth: 30
                            }

                            Label {
                                text: modelData.split("/").pop().split("\\").pop()
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Button {
                                text: "×"
                                onClicked: {
                                    var files = Array.from(batchPage.inputFiles)
                                    files.splice(index, 1)
                                    batchPage.inputFiles = files
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Button {
                            text: qsTrId("common.browse")
                            onClicked: batchFileDialog.open()
                        }

                        Button {
                            text: qsTrId("common.clear")
                            onClicked: batchPage.inputFiles = []
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Batch Settings")
                Layout.fillWidth: true
                Layout.fillHeight: true

                GridLayout {
                    columns: 2
                    anchors.fill: parent
                    columnSpacing: 16
                    rowSpacing: 12

                    Label { text: qsTrId("convert.outputFormat") + ":" }
                    ComboBox {
                        model: ["mp4", "mkv", "mov", "webm", "avi", "png", "jpeg", "webp"]
                        currentIndex: 0
                        onCurrentTextChanged: batchPage.outputFormat = currentText
                        Layout.fillWidth: true
                    }

                    Label { text: qsTrId("convert.codec") + ":" }
                    ComboBox {
                        model: ["auto", "copy", "libx264", "libx265", "libvpx-vp9", "libaom-av1"]
                        currentIndex: 0
                        onCurrentTextChanged: {
                            if (currentText === "copy") {
                                batchPage.useSmartCopy = true
                                batchPage.videoCodec = ""
                            } else if (currentText === "auto") {
                                batchPage.useSmartCopy = false
                                batchPage.videoCodec = ""
                            } else {
                                batchPage.useSmartCopy = false
                                batchPage.videoCodec = currentText
                            }
                        }
                        Layout.fillWidth: true
                    }

                    Label { text: qsTrId("convert.quality") + ":" }
                    RowLayout {
                        Slider {
                            from: 0; to: 51; value: 23; stepSize: 1
                            Layout.fillWidth: true
                            onValueChanged: batchPage.quality = Math.round(value)
                        }
                        Label {
                            text: batchPage.quality
                            Layout.preferredWidth: 30
                        }
                    }

                    Label { text: qsTr("Output Dir:") }
                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: batchPage.outputDirectory || qsTr("Same as input")
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        Button {
                            text: qsTrId("common.browse")
                            onClicked: outputDirDialog.open()
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 12

            Label {
                text: batchPage.inputFiles.length + " " + qsTr("files selected")
                color: palette.placeholderText
            }

            Button {
                text: qsTr("Add All to Queue")
                enabled: batchPage.inputFiles.length > 0
                onClicked: {
                    for (var i = 0; i < batchPage.inputFiles.length; i++) {
                        var inputFile = batchPage.inputFiles[i]
                        var outputPath = ""

                        if (batchPage.outputDirectory !== "") {
                            var fileName = inputFile.split("/").pop().split("\\").pop()
                            outputPath = batchPage.outputDirectory + "/" + fileName.replace(/\.[^.]+$/, "." + batchPage.outputFormat)
                        } else {
                            outputPath = inputFile.replace(/\.[^.]+$/, "." + batchPage.outputFormat)
                        }

                        var args = commandBuilder.buildConversion(
                            inputFile,
                            outputPath,
                            batchPage.outputFormat,
                            batchPage.videoCodec,
                            "",
                            batchPage.quality,
                            batchPage.useSmartCopy
                        )

                        var job = Qt.createQmlObject('import ffmedia; Job {}', batchPage)
                        job.name = inputFile.split("/").pop().split("\\").pop()
                        job.setCommand(ffmpegLocator.ffmpegPath, args)

                        jobQueue.addJob(job)
                    }
                }
            }
        }
    }

    FileDialog {
        id: batchFileDialog
        title: qsTr("Select Files for Batch Processing")
        nameFilters: ["All files (*)"]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            var newFiles = []
            for (var i = 0; i < selectedFiles.length; i++) {
                newFiles.push(selectedFiles[i].toString().replace("file:///", ""))
            }
            batchPage.inputFiles = newFiles
        }
    }

    FolderDialog {
        id: outputDirDialog
        title: qsTr("Select Output Directory")
        onAccepted: {
            batchPage.outputDirectory = selectedFolder.toString().replace("file:///", "")
        }
    }
}


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: convertPage

    

    

    property string inputFile: ""
    property string outputFormat: "mp4"
    property string videoCodec: ""
    property string audioCodec: ""
    property int quality: 23
    property bool useSmartCopy: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            //% "Format Conversion"
            text: qsTrId("convert.title")
            font.pixelSize: 24
            font.weight: Font.Bold
        }

        GroupBox {
            //% "Input File"
            title: qsTrId("convert.input")
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: convertPage.inputFile !== "" ? convertPage.inputFile : qsTr("No file selected")
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }

                    Button {
                        //% "Browse..."
                        text: qsTrId("common.browse")
                        onClicked: inputFileDialog.open()
                    }
                }

                Label {
                    visible: convertPage.inputFile !== ""
                    text: {
                        if (convertPage.inputFile === "") return ""
                        var info = mediaProbe.probe(convertPage.inputFile)
                        if (!info || !info.isValid()) return "Invalid file"
                        var parts = []
                        parts.push(info.formatName)
                        if (info.hasVideo()) {
                            var v = info.primaryVideo()
                            parts.push(v.width + "x" + v.height)
                        }
                        return parts.join(" | ")
                    }
                    color: palette.placeholderText
                    font.pixelSize: 12
                }
            }
        }

        GroupBox {
            //% "Output Format"
            title: qsTrId("convert.outputFormat")
            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent
                columnSpacing: 16
                rowSpacing: 8

                //% "Output Format"
                Label { text: qsTrId("convert.outputFormat") + ":" }
                ComboBox {
                    id: formatCombo
                    model: ["mp4", "mkv", "mov", "webm", "avi"]
                    currentIndex: 0
                    onCurrentTextChanged: convertPage.outputFormat = currentText
                    Layout.fillWidth: true
                }

                //% "Codec"
                Label { text: qsTrId("convert.codec") + ":" }
                ComboBox {
                    id: codecCombo
                    model: ["auto", "copy", "libx264", "libx265", "libvpx-vp9", "libaom-av1"]
                    currentIndex: 0
                    onCurrentTextChanged: {
                        if (currentText === "copy") {
                            convertPage.useSmartCopy = true
                            convertPage.videoCodec = ""
                        } else if (currentText === "auto") {
                            convertPage.useSmartCopy = false
                            convertPage.videoCodec = ""
                        } else {
                            convertPage.useSmartCopy = false
                            convertPage.videoCodec = currentText
                        }
                    }
                    Layout.fillWidth: true
                }

                //% "Quality"
                Label { text: qsTrId("convert.quality") + ":" }
                RowLayout {
                    Slider {
                        id: qualitySlider
                        from: 0
                        to: 51
                        value: 23
                        stepSize: 1
                        Layout.fillWidth: true
                        onValueChanged: convertPage.quality = Math.round(value)
                    }
                    Label {
                        text: convertPage.quality
                        Layout.preferredWidth: 30
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 12

            Button {
                //% "Add to Queue"
                text: qsTrId("convert.addToQueue")
                enabled: convertPage.inputFile !== ""
                onClicked: {
                    var info = mediaProbe.probe(convertPage.inputFile)
                    if (!info || !info.isValid()) return

                    var job = Qt.createQmlObject('import ffmedia; Job {}', convertPage)
                    job.name = info.path.split("/").pop().split("\\").pop()

                    var outputPath = convertPage.inputFile.replace(/\.[^.]+$/, "." + convertPage.outputFormat)
                    var args = commandBuilder.buildConversion(
                        convertPage.inputFile,
                        outputPath,
                        convertPage.outputFormat,
                        convertPage.videoCodec,
                        "",
                        convertPage.quality,
                        convertPage.useSmartCopy
                    )

                    job.setCommand(ffmpegLocator.ffmpegPath, args)
                    job.totalDuration = info.duration

                    jobQueue.addJob(job)
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    FileDialog {
        id: inputFileDialog
        //% "Input File"
        title: qsTrId("convert.input")
        nameFilters: ["Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv)", "All files (*)"]
        onAccepted: {
            convertPage.inputFile = selectedFile.toString().replace("file:///", "")
        }
    }
}

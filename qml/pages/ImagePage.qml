import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: imagePage

    property string inputFile: ""
    property string outputFormat: "png"
    property int quality: 90

    property int cropX: 0
    property int cropY: 0
    property int cropW: 0
    property int cropH: 0
    property int resizeW: 0
    property int resizeH: 0
    property int rotation: 0
    property bool flipH: false
    property bool flipV: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: i18n.tr("image.title")
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

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
                        color: palette.base
                        border.color: palette.mid
                        border.width: 1
                        radius: 4

                        Image {
                            anchors.fill: parent
                            anchors.margins: 4
                            source: imagePage.inputFile ? "file:///" + imagePage.inputFile : ""
                            fillMode: Image.PreserveAspectFit
                            visible: imagePage.inputFile !== ""
                        }

                        Label {
                            anchors.centerIn: parent
                            text: qsTr("No image selected")
                            color: palette.placeholderText
                            visible: imagePage.inputFile === ""
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: imageFileDialog.open()
                            cursorShape: Qt.PointingHandCursor
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: imagePage.inputFile !== "" ? imagePage.inputFile.split("/").pop().split("\\").pop() : ""
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Button {
                            text: i18n.tr("common.browse")
                            onClicked: imageFileDialog.open()
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Operations")
                Layout.fillWidth: true
                Layout.fillHeight: true

                ScrollView {
                    anchors.fill: parent
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        GroupBox {
                            title: i18n.tr("image.crop")
                            Layout.fillWidth: true
                            checkable: true
                            checked: false

                            GridLayout {
                                columns: 4
                                anchors.fill: parent
                                columnSpacing: 8
                                rowSpacing: 4

                                Label { text: "X:" }
                                SpinBox {
                                    from: 0; to: 9999; value: imagePage.cropX
                                    onValueModified: imagePage.cropX = value
                                    Layout.fillWidth: true
                                }
                                Label { text: "Y:" }
                                SpinBox {
                                    from: 0; to: 9999; value: imagePage.cropY
                                    onValueModified: imagePage.cropY = value
                                    Layout.fillWidth: true
                                }
                                Label { text: "W:" }
                                SpinBox {
                                    from: 0; to: 9999; value: imagePage.cropW
                                    onValueModified: imagePage.cropW = value
                                    Layout.fillWidth: true
                                }
                                Label { text: "H:" }
                                SpinBox {
                                    from: 0; to: 9999; value: imagePage.cropH
                                    onValueModified: imagePage.cropH = value
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        GroupBox {
                            title: i18n.tr("image.resize")
                            Layout.fillWidth: true
                            checkable: true
                            checked: false

                            GridLayout {
                                columns: 2
                                anchors.fill: parent
                                columnSpacing: 8
                                rowSpacing: 4

                                Label { text: "W:" }
                                SpinBox {
                                    from: 0; to: 9999; value: imagePage.resizeW
                                    onValueModified: imagePage.resizeW = value
                                    Layout.fillWidth: true
                                }
                                Label { text: "H:" }
                                SpinBox {
                                    from: 0; to: 9999; value: imagePage.resizeH
                                    onValueModified: imagePage.resizeH = value
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        GroupBox {
                            title: i18n.tr("image.rotate")
                            Layout.fillWidth: true

                            RowLayout {
                                anchors.fill: parent
                                spacing: 8

                                ButtonGroup { id: rotateGroup }

                                RadioButton {
                                    text: "0°"
                                    checked: true
                                    ButtonGroup.group: rotateGroup
                                    onCheckedChanged: if (checked) imagePage.rotation = 0
                                }
                                RadioButton {
                                    text: "90°"
                                    ButtonGroup.group: rotateGroup
                                    onCheckedChanged: if (checked) imagePage.rotation = 90
                                }
                                RadioButton {
                                    text: "180°"
                                    ButtonGroup.group: rotateGroup
                                    onCheckedChanged: if (checked) imagePage.rotation = 180
                                }
                                RadioButton {
                                    text: "270°"
                                    ButtonGroup.group: rotateGroup
                                    onCheckedChanged: if (checked) imagePage.rotation = 270
                                }
                            }
                        }

                        GroupBox {
                            title: i18n.tr("image.flip")
                            Layout.fillWidth: true

                            RowLayout {
                                anchors.fill: parent
                                spacing: 16

                                CheckBox {
                                    text: qsTr("Horizontal")
                                    checked: imagePage.flipH
                                    onCheckedChanged: imagePage.flipH = checked
                                }
                                CheckBox {
                                    text: qsTr("Vertical")
                                    checked: imagePage.flipV
                                    onCheckedChanged: imagePage.flipV = checked
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label { text: i18n.tr("convert.outputFormat") + ":" }
            ComboBox {
                model: ["png", "jpeg", "webp", "bmp", "tiff"]
                currentIndex: 0
                onCurrentTextChanged: imagePage.outputFormat = currentText
            }

            Label { text: i18n.tr("convert.quality") + ":" }
            SpinBox {
                from: 1; to: 100; value: imagePage.quality
                onValueModified: imagePage.quality = value
            }

            Item { Layout.fillWidth: true }

            Button {
                text: i18n.tr("convert.addToQueue")
                enabled: imagePage.inputFile !== ""
                onClicked: {
                    var ext = imagePage.outputFormat === "jpeg" ? "jpg" : imagePage.outputFormat
                    var outputPath = imagePage.inputFile.replace(/\.[^.]+$/, "." + ext)

                    var settings = {
                        inputPath: imagePage.inputFile,
                        outputPath: outputPath,
                        cropX: imagePage.cropX,
                        cropY: imagePage.cropY,
                        cropW: imagePage.cropW,
                        cropH: imagePage.cropH,
                        resizeW: imagePage.resizeW,
                        resizeH: imagePage.resizeH,
                        rotate: imagePage.rotation,
                        flipH: imagePage.flipH,
                        flipV: imagePage.flipV,
                        outputFormat: imagePage.outputFormat,
                        quality: imagePage.quality
                    }

                    var builder = Qt.createQmlObject('import ffmedia; FFmpegCommandBuilder {}', imagePage)
                    var cmd = builder.buildImageOperation(settings)

                    var job = Qt.createQmlObject('import ffmedia; Job {}', imagePage)
                    job.name = "Image: " + imagePage.inputFile.split("/").pop().split("\\").pop()
                    job.command = cmd

                    jobQueue.addJob(job)
                }
            }
        }
    }

    FileDialog {
        id: imageFileDialog
        title: i18n.tr("image.title")
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.webp *.bmp *.tiff *.gif)", "All files (*)"]
        onAccepted: {
            imagePage.inputFile = selectedFile.toString().replace("file:///", "")
        }
    }
}

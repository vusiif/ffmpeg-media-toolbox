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
    property int rotateAngle: 0
    property bool flipH: false
    property bool flipV: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTrId("image.title")
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
                            text: qsTrId("common.browse")
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
                            title: qsTrId("image.crop")
                            Layout.fillWidth: true

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
                            title: qsTrId("image.resize")
                            Layout.fillWidth: true

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
                            title: qsTrId("image.rotate")
                            Layout.fillWidth: true

                            RowLayout {
                                anchors.fill: parent
                                spacing: 8

                                ButtonGroup { id: rotateGroup }

                                RadioButton {
                                    text: "0°"
                                    checked: true
                                    ButtonGroup.group: rotateGroup
                                    onCheckedChanged: if (checked) imagePage.rotateAngle = 0
                                }
                                RadioButton {
                                    text: "90°"
                                    ButtonGroup.group: rotateGroup
                                    onCheckedChanged: if (checked) imagePage.rotateAngle = 90
                                }
                                RadioButton {
                                    text: "180°"
                                    ButtonGroup.group: rotateGroup
                                    onCheckedChanged: if (checked) imagePage.rotateAngle = 180
                                }
                                RadioButton {
                                    text: "270°"
                                    ButtonGroup.group: rotateGroup
                                    onCheckedChanged: if (checked) imagePage.rotateAngle = 270
                                }
                            }
                        }

                        GroupBox {
                            title: qsTrId("image.flip")
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

            Label { text: qsTrId("convert.outputFormat") + ":" }
            ComboBox {
                model: ["png", "jpeg", "webp", "bmp", "tiff"]
                currentIndex: 0
                onCurrentTextChanged: imagePage.outputFormat = currentText
            }

            Label { text: qsTrId("convert.quality") + ":" }
            SpinBox {
                from: 1; to: 100; value: imagePage.quality
                onValueModified: imagePage.quality = value
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTrId("convert.addToQueue")
                enabled: imagePage.inputFile !== ""
                onClicked: {
                    var ext = imagePage.outputFormat === "jpeg" ? "jpg" : imagePage.outputFormat
                    var outputPath = imagePage.inputFile.replace(/\.[^.]+$/, "." + ext)

                    var args = commandBuilder.buildImageOperation(
                        imagePage.inputFile,
                        outputPath,
                        imagePage.cropX, imagePage.cropY, imagePage.cropW, imagePage.cropH,
                        imagePage.resizeW, imagePage.resizeH,
                        imagePage.rotateAngle,
                        imagePage.flipH, imagePage.flipV,
                        imagePage.outputFormat,
                        imagePage.quality
                    )

                    var job = Qt.createQmlObject('import ffmedia; Job {}', imagePage)
                    job.name = "Image: " + imagePage.inputFile.split("/").pop().split("\\").pop()
                    job.setCommand(ffmpegLocator.ffmpegPath, args)

                    jobQueue.addJob(job)
                }
            }
        }
    }

    FileDialog {
        id: imageFileDialog
        title: qsTrId("image.title")
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.webp *.bmp *.tiff *.gif)", "All files (*)"]
        onAccepted: {
            imagePage.inputFile = selectedFile.toString().replace("file:///", "")
        }
    }
}


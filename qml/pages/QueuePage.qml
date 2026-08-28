import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: queuePage

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: i18n.tr("queue.title")
                font.pixelSize: 24
                font.weight: Font.Bold
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Clear Completed")
                onClicked: jobQueue.clearCompleted()
            }
        }

        ListView {
            id: jobListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: jobModel
            clip: true
            spacing: 8

            delegate: Rectangle {
                width: jobListView.width
                height: 80
                radius: 8
                color: palette.base
                border.color: palette.mid
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: model.jobName
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: {
                                switch(model.jobStatus) {
                                case 0: return i18n.tr("queue.pending")
                                case 1: return qsTr("Preparing")
                                case 2: return i18n.tr("queue.running")
                                case 3: return i18n.tr("queue.completed")
                                case 4: return i18n.tr("queue.failed")
                                case 5: return i18n.tr("queue.cancel")
                                default: return ""
                                }
                            }
                            color: {
                                switch(model.jobStatus) {
                                case 2: return "#0078d4"
                                case 3: return "green"
                                case 4: return "red"
                                default: return palette.placeholderText
                                }
                            }
                        }
                    }

                    ProgressBar {
                        from: 0
                        to: 1
                        value: model.jobProgress
                        visible: model.jobStatus === 2
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: model.jobError || ""
                            visible: model.jobStatus === 4
                            color: "red"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: i18n.tr("queue.cancel")
                            visible: model.jobStatus === 0 || model.jobStatus === 2
                            onClicked: jobQueue.cancelJob(model.jobId)
                        }

                        Button {
                            text: i18n.tr("queue.retry")
                            visible: model.jobStatus === 4 || model.jobStatus === 5
                            onClicked: jobQueue.retryJob(model.jobId)
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                text: qsTr("No tasks in queue")
                color: palette.placeholderText
                visible: jobModel.count === 0
            }
        }
    }
}

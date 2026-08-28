import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: queuePage

    function t(key) { return t(key, i18n.languageVersion) }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: t("queue.title")
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
                                case 0: return t("queue.pending")
                                case 1: return qsTr("Preparing")
                                case 2: return t("queue.running")
                                case 3: return t("queue.completed")
                                case 4: return t("queue.failed")
                                case 5: return t("queue.cancel")
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
                            text: t("queue.cancel")
                            visible: model.jobStatus === 0 || model.jobStatus === 2
                            onClicked: jobQueue.cancelJob(model.jobId)
                        }

                        Button {
                            text: t("queue.retry")
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


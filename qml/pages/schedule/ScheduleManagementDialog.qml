import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: scheduleMgmtDialog
    title: "课表管理"
    width: 500
    height: 420
    modal: true

    property var scheduleList: []  // List of {id, semesterName, totalWeeks, isCurrent}

    signal switchSchedule(string scheduleId)
    signal newSchedule(string name)
    signal renameSchedule(string scheduleId, string newName)
    signal deleteScheduleRequested(string scheduleId)

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text: "选择课表："
            font.pixelSize: 14
            font.bold: true
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: scheduleList

            delegate: Rectangle {
                width: listView.width
                height: 60
                color: modelData.isCurrent ? "#e8f5e9" : (index % 2 === 0 ? "#fafafa" : "#ffffff")
                border.color: "#e0e0e0"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: modelData.semesterName
                            font.pixelSize: 15
                            font.bold: modelData.isCurrent
                        }
                        Label {
                            text: "共 " + modelData.totalWeeks + " 周"
                            font.pixelSize: 12
                            color: "#999"
                        }
                    }

                    Label {
                        text: "当前"
                        font.pixelSize: 12
                        color: "#4CAF50"
                        visible: modelData.isCurrent
                    }

                    ToolButton {
                        text: "✏"
                        onClicked: {
                            renameDialog.scheduleId = modelData.id
                            renameDialog.oldName = modelData.semesterName
                            renameDialog.open()
                        }
                    }

                    ToolButton {
                        text: "🗑"
                        onClicked: deleteScheduleRequested(modelData.id)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (!modelData.isCurrent) {
                            switchSchedule(modelData.id)
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "新建课表"
                onClicked: newScheduleDialog.open()
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "关闭"
                onClicked: scheduleMgmtDialog.close()
            }
        }
    }

    // New schedule dialog
    Dialog {
        id: newScheduleDialog
        title: "新建课表"
        width: 350
        height: 160
        modal: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label { text: "课表名称：" }
            TextField {
                id: newNameField
                Layout.fillWidth: true
                placeholderText: "请输入课表名称"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Item { Layout.fillWidth: true }

                Button {
                    text: "取消"
                    onClicked: newScheduleDialog.close()
                }
                Button {
                    text: "创建"
                    highlighted: true
                    enabled: newNameField.text.trim() !== ""
                    onClicked: {
                        newSchedule(newNameField.text.trim())
                        newNameField.text = ""
                        newScheduleDialog.close()
                    }
                }
            }
        }
    }

    // Rename dialog
    Dialog {
        id: renameDialog
        title: "重命名课表"
        width: 350
        height: 160
        modal: true

        property string scheduleId: ""
        property string oldName: ""

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label { text: "新名称：" }
            TextField {
                id: renameField
                Layout.fillWidth: true
                text: renameDialog.oldName
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Item { Layout.fillWidth: true }

                Button {
                    text: "取消"
                    onClicked: renameDialog.close()
                }
                Button {
                    text: "确定"
                    highlighted: true
                    onClicked: {
                        renameSchedule(renameDialog.scheduleId, renameField.text.trim())
                        renameDialog.close()
                    }
                }
            }
        }
    }
}

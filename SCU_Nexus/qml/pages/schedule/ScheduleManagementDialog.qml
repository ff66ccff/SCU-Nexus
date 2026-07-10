pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../styles"

Dialog {
    id: root

    title: "课表管理"
    width: 520
    height: 440
    modal: true

    property var scheduleList: []
    signal switchSchedule(string scheduleId)
    signal newSchedule(string name)
    signal renameSchedule(string scheduleId, string newName)
    signal deleteScheduleRequested(string scheduleId)

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.scheduleList
            spacing: 4

            delegate: Rectangle {
                required property int index
                required property var modelData
                width: listView.width
                height: 62
                radius: 5
                color: modelData.isCurrent ? Theme.primarySoft : Theme.surfaceMuted
                border.color: Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: modelData.semesterName
                            color: Theme.text
                            font.weight: Theme.weightStrong
                            elide: Text.ElideRight
                        }
                        Text {
                            text: "共 " + modelData.totalWeeks + " 周"
                            color: Theme.mutedText
                            font.pixelSize: Theme.fontCaption
                        }
                    }
                    Label {
                        text: "当前"
                        visible: modelData.isCurrent
                        color: Theme.success
                    }
                    ToolButton {
                        text: "重命名"
                        onClicked: {
                            renameDialog.scheduleId = modelData.id
                            renameField.text = modelData.semesterName
                            renameDialog.open()
                        }
                    }
                    ToolButton {
                        text: "删除"
                        onClicked: root.deleteScheduleRequested(modelData.id)
                    }
                }

                MouseArea {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: 170
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!parent.modelData.isCurrent)
                            root.switchSchedule(parent.modelData.id)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: root.scheduleList.length === 0
                text: "暂无课表"
                color: Theme.mutedText
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Button { text: "新建课表"; onClicked: newDialog.open() }
            Item { Layout.fillWidth: true }
            Button { text: "关闭"; onClicked: root.close() }
        }
    }

    Dialog {
        id: newDialog
        parent: Overlay.overlay
        title: "新建课表"
        modal: true
        width: 360
        standardButtons: Dialog.NoButton
        contentItem: ColumnLayout {
            spacing: 10
            TextField {
                id: newNameField
                Layout.fillWidth: true
                placeholderText: "请输入课表名称"
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button { text: "取消"; onClicked: newDialog.close() }
                Button {
                    text: "创建"
                    highlighted: true
                    enabled: newNameField.text.trim().length > 0
                    onClicked: {
                        root.newSchedule(newNameField.text.trim())
                        newNameField.clear()
                        newDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: renameDialog
        parent: Overlay.overlay
        title: "重命名课表"
        modal: true
        width: 360
        property string scheduleId: ""
        standardButtons: Dialog.NoButton
        contentItem: ColumnLayout {
            spacing: 10
            TextField { id: renameField; Layout.fillWidth: true }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button { text: "取消"; onClicked: renameDialog.close() }
                Button {
                    text: "确定"
                    highlighted: true
                    enabled: renameField.text.trim().length > 0
                    onClicked: {
                        root.renameSchedule(renameDialog.scheduleId, renameField.text.trim())
                        renameDialog.close()
                    }
                }
            }
        }
    }
}

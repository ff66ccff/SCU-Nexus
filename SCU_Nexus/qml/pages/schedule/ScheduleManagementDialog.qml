pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Dialog {
    id: root

    title: "课表管理"
    width: Math.min(parent ? parent.width - 48 : 560, 560)
    height: Math.min(parent ? parent.height - 48 : 520, 520)
    modal: true
    standardButtons: Dialog.NoButton

    property var scheduleList: []

    signal switchSchedule(string scheduleId)
    signal newSchedule(string name)
    signal renameSchedule(string scheduleId, string newName)
    signal deleteScheduleRequested(string scheduleId)

    contentItem: ColumnLayout {
        spacing: Theme.spacing16

        Text {
            Layout.fillWidth: true
            text: "新建、切换或整理保存在本机的课表"
            font.pixelSize: Theme.fontCaption
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.scheduleList
            spacing: Theme.spacing8

            delegate: Rectangle {
                id: scheduleItem

                required property var modelData

                width: listView.width
                height: 68
                radius: Theme.cardRadius
                color: modelData.isCurrent ? Theme.primarySoft : Theme.surfaceMuted
                border.color: modelData.isCurrent ? Theme.accent : Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacing12
                    spacing: Theme.spacing8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spacing8

                            Text {
                                Layout.fillWidth: true
                                text: scheduleItem.modelData.semesterName
                                color: Theme.text
                                font.pixelSize: Theme.fontBody
                                font.weight: Theme.weightStrong
                                elide: Text.ElideRight
                            }

                            Text {
                                visible: scheduleItem.modelData.isCurrent
                                text: "当前"
                                color: Theme.success
                                font.pixelSize: Theme.fontCaption
                                font.weight: Theme.weightStrong
                            }
                        }

                        Text {
                            text: "共 " + scheduleItem.modelData.totalWeeks + " 周"
                            color: Theme.mutedText
                            font.pixelSize: Theme.fontCaption
                        }
                    }

                    ToolButton {
                        text: "切换"
                        visible: !scheduleItem.modelData.isCurrent
                        onClicked: root.switchSchedule(scheduleItem.modelData.id)
                    }

                    ToolButton {
                        text: "重命名"
                        onClicked: {
                            renameDialog.scheduleId = scheduleItem.modelData.id
                            renameField.text = scheduleItem.modelData.semesterName
                            renameDialog.open()
                        }
                    }

                    ToolButton {
                        text: "删除"
                        onClicked: root.deleteScheduleRequested(scheduleItem.modelData.id)
                    }
                }

            }

            EmptyView {
                anchors.fill: parent
                visible: root.scheduleList.length === 0
                title: "暂无课表"
                description: "创建空白课表，或从教务系统导入"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing8

            AppButton {
                text: "新建课表"
                onClicked: newDialog.open()
            }

            Item { Layout.fillWidth: true }

            AppButton {
                text: "关闭"
                type: "secondary"
                onClicked: root.close()
            }
        }
    }

    Dialog {
        id: newDialog
        parent: Overlay.overlay
        title: "新建课表"
        modal: true
        width: Math.min(parent ? parent.width - 48 : 380, 380)
        standardButtons: Dialog.NoButton

        contentItem: ColumnLayout {
            spacing: Theme.spacing12

            TextField {
                id: newNameField
                Layout.fillWidth: true
                placeholderText: "请输入课表名称"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing8
                Item { Layout.fillWidth: true }
                AppButton {
                    text: "取消"
                    type: "secondary"
                    onClicked: newDialog.close()
                }
                AppButton {
                    text: "创建"
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
        width: Math.min(parent ? parent.width - 48 : 380, 380)
        standardButtons: Dialog.NoButton

        property string scheduleId: ""

        contentItem: ColumnLayout {
            spacing: Theme.spacing12

            TextField {
                id: renameField
                Layout.fillWidth: true
                placeholderText: "请输入新的课表名称"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing8
                Item { Layout.fillWidth: true }
                AppButton {
                    text: "取消"
                    type: "secondary"
                    onClicked: renameDialog.close()
                }
                AppButton {
                    text: "确定"
                    enabled: renameField.text.trim().length > 0
                    onClicked: {
                        root.renameSchedule(renameDialog.scheduleId,
                                            renameField.text.trim())
                        renameDialog.close()
                    }
                }
            }
        }
    }
}

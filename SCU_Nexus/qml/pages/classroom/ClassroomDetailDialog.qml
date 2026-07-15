pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Dialog {
    id: root

    property var room: ({})
    property string campusName: ""
    property string buildingName: ""
    property string queryDate: ""
    property int teachingWeek: 0

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    title: room.name || room.number || "教室详情"
    standardButtons: Dialog.Close
    width: Math.min(parent ? parent.width - 48 : 660, 660)
    height: Math.min(parent ? parent.height - 48 : 720, 720)

    function statusColor(key) {
        if (key === "free") return Theme.success
        if (key === "inClass") return Theme.danger
        if (key === "exam") return "#d97706"
        if (key === "experiment") return "#8b5cf6"
        return "#ca8a04"
    }

    function statusColorWithAlpha(key, alpha) {
        const value = root.statusColor(key)
        return Qt.rgba(value.r, value.g, value.b, alpha)
    }

    contentItem: ScrollView {
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: Math.max(parent.width - Theme.spacing8, 300)
            spacing: Theme.spacing12

            Card {
                Layout.fillWidth: true
                implicitHeight: info.implicitHeight + 2 * Theme.spacing16

                ColumnLayout {
                    id: info
                    anchors.fill: parent
                    anchors.margins: Theme.spacing16
                    spacing: Theme.spacing8

                    Text {
                        Layout.fillWidth: true
                        text: root.room.name || root.room.number
                        font.pixelSize: Theme.fontTitle
                        font.weight: Theme.weightStrong
                        color: Theme.text
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        text: root.buildingName + " · " + root.campusName + "校区"
                        font.pixelSize: Theme.fontBody
                        color: Theme.mutedText
                    }
                    Text {
                        text: "座位数：" + (root.room.seats || 0)
                              + " · " + (root.room.borrowable === "是" ? "可借用" : "不可借用")
                        font.pixelSize: Theme.fontLabel
                        color: Theme.text
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "备注：" + (root.room.remark || "无")
                        font.pixelSize: Theme.fontLabel
                        color: Theme.mutedText
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        text: "查询日期：" + root.queryDate
                              + (root.teachingWeek > 0 ? " · 教学第 " + root.teachingWeek + " 周" : "")
                        font.pixelSize: Theme.fontCaption
                        color: Theme.mutedText
                    }
                }
            }

            Text {
                text: "第 1–12 节占用详情"
                font.pixelSize: Theme.fontSection
                font.weight: Theme.weightStrong
                color: Theme.text
            }

            Repeater {
                model: root.room.periods || []

                delegate: Rectangle {
                    id: periodRow
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: 46
                    radius: Theme.smallRadius
                    color: root.statusColorWithAlpha(periodRow.modelData.statusKey, 0.13)
                    border.width: 1
                    border.color: root.statusColorWithAlpha(periodRow.modelData.statusKey, 0.35)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing16
                        anchors.rightMargin: Theme.spacing16

                        Text {
                            Layout.fillWidth: true
                            text: "第 " + periodRow.modelData.period + " 节"
                            font.pixelSize: Theme.fontBody
                            font.weight: Theme.weightStrong
                            color: Theme.text
                        }
                        Text {
                            text: periodRow.modelData.statusText
                            font.pixelSize: Theme.fontBody
                            font.weight: Theme.weightStrong
                            color: root.statusColor(periodRow.modelData.statusKey)
                        }
                    }
                }
            }
        }
    }
}

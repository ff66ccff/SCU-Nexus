pragma ComponentBehavior: Bound

import QtQuick 2.15
import "../styles"

// Fluent 分段控件（Segmented control）：一条圆角轨道内均分若干段，
// 选中段用品牌红填充、白字；悬浮段有细微填充。
//   SegmentedControl {
//       model: [ { label: "跟随系统", value: "system" }, ... ]
//       value: themeManager.mode
//       onActivated: themeManager.setMode(value)
//   }
Item {
    id: root

    property var model: []
    property string value: ""
    signal activated(string newValue)

    implicitHeight: Theme.controlHeight
    implicitWidth: Math.max(240, root.model.length * 96)

    readonly property int _gap: 3
    readonly property int _pad: 3

    // 轨道
    Rectangle {
        anchors.fill: parent
        radius: Theme.smallRadius + 3
        color: Theme.control
        border.width: 1
        border.color: Theme.border
    }

    Row {
        id: segRow
        anchors.fill: parent
        anchors.margins: root._pad
        spacing: root._gap

        Repeater {
            model: root.model

            delegate: Rectangle {
                id: seg

                required property var modelData

                readonly property bool selected: root.value === seg.modelData.value
                height: parent.height
                width: (root.width - 2 * root._pad - (root.model.length - 1) * root._gap)
                       / Math.max(1, root.model.length)
                radius: Theme.smallRadius
                color: seg.selected ? Theme.accent
                      : (segMouse.containsMouse ? Theme.subtleHover : "transparent")

                Text {
                    anchors.centerIn: parent
                    text: seg.modelData.label
                    font.pixelSize: Theme.fontLabel
                    font.weight: seg.selected ? Theme.weightStrong : Theme.weightNormal
                    color: seg.selected ? Theme.accentText : Theme.text
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: segMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.activated(seg.modelData.value)
                }
            }
        }
    }
}

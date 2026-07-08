import QtQuick 2.15
import QtQuick.Controls 2.15
import "../styles"

ToolButton {
    id: root

    property string tooltip: ""

    implicitWidth: Theme.controlHeight
    implicitHeight: Theme.controlHeight

    ToolTip.text: tooltip
    ToolTip.visible: hovered && tooltip.length > 0
    ToolTip.delay: 500

    background: Rectangle {
        radius: Theme.smallRadius
        color: root.down ? Theme.controlPressed : (root.hovered ? Theme.control : "transparent")
        border.color: root.hovered ? Theme.border : "transparent"
    }
}

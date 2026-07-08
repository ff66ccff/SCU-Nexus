import QtQuick 2.15
import QtQuick.Controls 2.15
import "../styles"

Button {
    id: root

    property bool busy: false
    property string type: "primary"
    property string iconText: ""

    implicitHeight: Theme.controlHeight
    enabled: !busy
    opacity: enabled ? 1 : 0.62

    contentItem: Row {
        spacing: 8
        anchors.centerIn: parent

        BusyIndicator {
            width: 16
            height: 16
            running: root.busy
            visible: root.busy
        }

        Text {
            visible: root.iconText.length > 0 && !root.busy
            text: root.iconText
            font.pixelSize: Theme.fontBody
            color: root.type === "secondary" || root.type === "ghost" ? Theme.primary : "white"
        }

        Text {
            text: root.busy ? "处理中..." : root.text
            font.pixelSize: Theme.fontBody
            font.weight: Theme.weightStrong
            color: {
                if (root.type === "secondary") return Theme.text
                if (root.type === "ghost") return Theme.primary
                return "white"
            }
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        radius: Theme.smallRadius
        color: {
            if (root.type === "danger") return root.down ? "#b42318" : Theme.danger
            if (root.type === "secondary") return root.down ? Theme.controlPressed : Theme.control
            if (root.type === "ghost") return root.down ? Theme.controlPressed : "transparent"
            return root.down ? Theme.primaryPressed : Theme.primary
        }
        border.width: root.type === "secondary" || root.type === "ghost" ? 1 : 0
        border.color: root.type === "danger" ? Theme.danger : Theme.border
    }
}

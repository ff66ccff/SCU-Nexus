import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: root
    height: 40

    property bool busy: false
    property string type: "primary"  // primary, secondary, danger, ghost

    text: busy ? "处理中..." : root.text

    enabled: !busy

    background: Rectangle {
        color: {
            if (!root.enabled) return "#cccccc"
            switch(root.type) {
                case "primary": return root.down ? "#0d47a1" : "#1976d2"
                case "secondary": return root.down ? "#e0e0e0" : "#f5f5f5"
                case "danger": return root.down ? "#b71c1c" : "#e53935"
                case "ghost": return "transparent"
                default: return "#1976d2"
            }
        }
        radius: 4
        border.color: root.type === "ghost" ? "#1976d2" : "transparent"
        border.width: root.type === "ghost" ? 1 : 0
    }

    contentItem: Text {
        text: root.text
        color: {
            if (!root.enabled) return "#999999"
            switch(root.type) {
                case "primary": return "#ffffff"
                case "secondary": return "#333333"
                case "danger": return "#ffffff"
                case "ghost": return "#1976d2"
                default: return "#ffffff"
            }
        }
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
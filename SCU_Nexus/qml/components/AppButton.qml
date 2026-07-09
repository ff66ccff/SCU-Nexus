import QtQuick 2.15
import QtQuick.Controls 2.15
import "../styles"

// 语义化按钮：让 FluentWinUI3 负责绘制背景/焦点环/悬浮态，
// 仅通过 highlighted / flat / palette.accent 表达 primary / secondary / danger / ghost，
// 并保留 busy 状态（内嵌 BusyIndicator）与可选前置图标字符。
Button {
    id: root

    property bool busy: false
    property string type: "primary"      // primary | secondary | danger | ghost
    property string iconText: ""

    enabled: !busy
    implicitHeight: Math.max(implicitContentHeight + topPadding + bottomPadding, Theme.controlHeight)

    // primary/danger 走强调填充；ghost 走扁平（透明+悬浮）；secondary 为标准按钮。
    highlighted: type === "primary" || type === "danger"
    flat: type === "ghost"

    // danger 复用强调机制，只把强调色换成危险红。
    palette.accent: type === "danger" ? Theme.danger : Theme.accent

    contentItem: Row {
        spacing: 8

        BusyIndicator {
            anchors.verticalCenter: parent.verticalCenter
            width: 16
            height: 16
            running: root.busy
            visible: root.busy
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.iconText.length > 0 && !root.busy
            text: root.iconText
            font.pixelSize: Theme.fontBody
            color: label.color
        }

        Text {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            text: root.busy ? "处理中..." : root.text
            font.pixelSize: Theme.fontBody
            font.weight: Theme.weightMedium
            color: root.highlighted ? Theme.accentText
                 : (root.type === "ghost" ? Theme.accent : Theme.text)
        }
    }
}

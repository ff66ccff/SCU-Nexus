import QtQuick 2.15
import QtQuick.Controls 2.15
import "../styles"

// 图标/字符工具按钮：使用 FluentWinUI3 的 ToolButton 悬浮/按下态，
// 仅补充 tooltip 与统一尺寸。
ToolButton {
    id: root

    property string tooltip: ""

    implicitWidth: Theme.controlHeight
    implicitHeight: Theme.controlHeight
    font.pixelSize: Theme.fontSection

    ToolTip.text: tooltip
    ToolTip.visible: hovered && tooltip.length > 0
    ToolTip.delay: 500
}

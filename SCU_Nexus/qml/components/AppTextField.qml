import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../styles"

// 带标签/错误提示的输入框。内层 TextField 交给 FluentWinUI3 绘制
// （底部强调线、悬浮/焦点态、圆角），仅保留可清除按钮与错误态：
// 错误时把 palette.accent 换成危险红，让 Fluent 的焦点线变红。
ColumnLayout {
    id: root

    property alias text: field.text
    property string label: ""
    property alias placeholderText: field.placeholderText
    property string errorText: ""
    property bool passwordMode: false
    property bool clearable: false
    property bool readOnly: false

    signal accepted()

    spacing: 6

    Text {
        visible: root.label.length > 0
        text: root.label
        font.pixelSize: Theme.fontLabel
        color: Theme.mutedText
    }

    TextField {
        id: field
        Layout.fillWidth: true
        implicitHeight: Theme.controlHeight
        font.pixelSize: Theme.fontBody
        echoMode: root.passwordMode ? TextInput.Password : TextInput.Normal
        readOnly: root.readOnly
        rightPadding: clearButton.visible ? 34 : 12
        selectByMouse: true
        palette.accent: root.errorText.length > 0 ? Theme.danger : Theme.accent
        onAccepted: root.accepted()

        ToolButton {
            id: clearButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 4
            width: 28
            height: 28
            flat: true
            visible: root.clearable && field.text.length > 0 && !root.readOnly
            text: "×"
            onClicked: field.clear()
        }
    }

    Text {
        visible: root.errorText.length > 0
        text: root.errorText
        font.pixelSize: Theme.fontCaption
        color: Theme.danger
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}

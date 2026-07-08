import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
    id: root
    width: parent.width
    height: 40

    property string label: ""
    property string errorText: ""
    property bool passwordMode: false
    property bool clearable: false

    placeholderText: label
    echoMode: passwordMode ? TextInput.Password : TextInput.Normal

    background: Rectangle {
        color: "#f5f5f5"
        radius: 4
        border.color: root.errorText !== "" ? "#e53935" : (root.activeFocus ? "#1976d2" : "#e0e0e0")
        border.width: root.activeFocus ? 2 : 1
    }

    // 清除按钮
    Button {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 4
        visible: root.clearable && root.text !== ""
        text: "✕"
        flat: true
        onClicked: root.clear()
    }

    // 错误提示文字
    Text {
        anchors.top: parent.bottom
        anchors.topMargin: 4
        text: root.errorText
        font.pixelSize: 12
        color: "#e53935"
        visible: root.errorText !== ""
    }
}
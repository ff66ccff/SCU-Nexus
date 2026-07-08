import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../styles"

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
        font.pixelSize: 13
        color: Theme.mutedText
    }

    TextField {
        id: field
        Layout.fillWidth: true
        implicitHeight: Theme.controlHeight
        echoMode: root.passwordMode ? TextInput.Password : TextInput.Normal
        readOnly: root.readOnly
        rightPadding: clearButton.visible ? 34 : 12
        color: Theme.text
        placeholderTextColor: Theme.placeholder
        selectByMouse: true
        onAccepted: root.accepted()

        background: Rectangle {
            radius: Theme.smallRadius
            color: Theme.control
            border.width: field.activeFocus || root.errorText.length > 0 ? 2 : 1
            border.color: root.errorText.length > 0 ? Theme.danger : (field.activeFocus ? Theme.primary : Theme.border)
        }

        ToolButton {
            id: clearButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 4
            width: 28
            height: 28
            visible: root.clearable && field.text.length > 0 && !root.readOnly
            text: "×"
            onClicked: field.clear()
        }
    }

    Text {
        visible: root.errorText.length > 0
        text: root.errorText
        font.pixelSize: 12
        color: Theme.danger
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}

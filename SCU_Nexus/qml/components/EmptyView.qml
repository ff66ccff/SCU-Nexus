import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles"

Item {
    id: root

    property string title: "暂无数据"
    property string description: ""
    property string actionText: ""
    signal actionTriggered()

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 360)
        spacing: 12

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            radius: 24
            color: Theme.control
            border.color: Theme.border

            Text {
                anchors.centerIn: parent
                text: "—"
                font.pixelSize: 24
                color: Theme.mutedText
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.title
            font.pixelSize: 18
            font.weight: Theme.weightStrong
            color: Theme.text
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            text: root.description
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: root.description.length > 0
        }

        AppButton {
            Layout.alignment: Qt.AlignHCenter
            visible: root.actionText.length > 0
            text: root.actionText
            type: "secondary"
            onClicked: root.actionTriggered()
        }
    }
}

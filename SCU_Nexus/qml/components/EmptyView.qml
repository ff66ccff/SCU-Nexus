import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "transparent"

    property string title: "暂无数据"
    property string description: ""
    property string actionText: ""
    signal actionTriggered()

    Column {
        anchors.centerIn: parent
        spacing: 12

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "📭"
            font.pixelSize: 48
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.title
            font.pixelSize: 18
            font.bold: true
            color: "#333333"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.description
            font.pixelSize: 14
            color: "#999999"
            visible: root.description !== ""
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.actionText
            visible: root.actionText !== ""
            onClicked: root.actionTriggered()
        }
    }
}
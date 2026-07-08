import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "transparent"

    property string title: "加载失败"
    property string message: ""
    property bool canRetry: true
    property string retryText: "重试"
    signal retry()

    Column {
        anchors.centerIn: parent
        spacing: 12

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "❌"
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
            text: root.message
            font.pixelSize: 14
            color: "#999999"
            visible: root.message !== ""
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.retryText
            visible: root.canRetry
            onClicked: root.retry()
        }
    }
}
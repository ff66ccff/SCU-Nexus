import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "transparent"

    property string message: "请先登录后查看"
    signal loginRequested()

    Column {
        anchors.centerIn: parent
        spacing: 12

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "🔒"
            font.pixelSize: 48
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.message
            font.pixelSize: 16
            color: "#666666"
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "去登录"
            onClicked: root.loginRequested()
        }
    }
}
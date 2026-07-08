import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    color: "transparent"

    Column {
        anchors.centerIn: parent
        spacing: 12

        Text {
            text: "📊"
            font.pixelSize: 48
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "教务成绩"
            font.pixelSize: 24
            font.bold: true
            color: "#333333"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "待 D 同学实现"
            font.pixelSize: 14
            color: "#999999"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
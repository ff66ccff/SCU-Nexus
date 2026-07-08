import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "transparent"

    property string text: "加载中..."
    property bool compact: false

    Column {
        anchors.centerIn: parent
        spacing: compact ? 8 : 16

        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            running: true
            width: compact ? 24 : 48
            height: compact ? 24 : 48
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.text
            font.pixelSize: compact ? 12 : 14
            color: "#666666"
            visible: root.text !== ""
        }
    }
}
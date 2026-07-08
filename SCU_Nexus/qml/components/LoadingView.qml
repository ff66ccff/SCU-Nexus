import QtQuick 2.15
import QtQuick.Controls 2.15
import "../styles"

Item {
    id: root

    property string text: "加载中..."
    property bool compact: false

    implicitWidth: compact ? 160 : 260
    implicitHeight: compact ? 64 : 160

    Column {
        anchors.centerIn: parent
        spacing: root.compact ? 8 : 14

        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            running: true
            width: root.compact ? 24 : 42
            height: width
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.text
            font.pixelSize: root.compact ? Theme.fontCaption : Theme.fontBody
            color: Theme.mutedText
            visible: root.text.length > 0
        }
    }
}

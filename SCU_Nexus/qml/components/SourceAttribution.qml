import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles"

RowLayout {
    id: root

    property string sourceUrl: ""

    spacing: 0

    Text {
        text: "数据来自"
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontMicro
        color: Theme.mutedText
    }

    Text {
        id: linkText

        Layout.fillWidth: true
        text: root.sourceUrl
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontMicro
        font.underline: linkMouse.containsMouse
        color: Theme.accent
        elide: Text.ElideRight

        MouseArea {
            id: linkMouse

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: Qt.openUrlExternally(root.sourceUrl)
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../styles"

Rectangle {
    id: root
    property int conflictCount: 0

    width: 20
    height: 20
    radius: 10
    visible: conflictCount > 1
    color: Theme.danger

    Text {
        anchors.centerIn: parent
        text: root.conflictCount
        font.pixelSize: 10
        font.bold: true
        color: "white"
    }

    ToolTip.visible: hover.hovered
    ToolTip.text: root.conflictCount + " 门课程冲突"
    HoverHandler { id: hover }
}

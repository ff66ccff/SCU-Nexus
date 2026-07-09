import QtQuick 2.15

// Small badge indicating course conflicts

Rectangle {
    id: conflictBadge

    property int conflictCount: 1
    property bool visible: conflictCount > 1

    width: 20
    height: 20
    radius: 10
    color: "#ff9800"
    border.color: "#fff"
    border.width: 1

    Label {
        anchors.centerIn: parent
        text: conflictCount
        font.pixelSize: 10
        font.bold: true
        color: "#fff"
    }

    ToolTip {
        visible: parent.visible && hovered
        text: conflictCount + " 门课程冲突"
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
    }
}

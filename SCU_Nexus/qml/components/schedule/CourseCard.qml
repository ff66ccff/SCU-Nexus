import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../styles"

Rectangle {
    id: root

    property string courseId: ""
    property string courseName: ""
    property string courseLocation: ""
    property string courseTeacher: ""
    property color cardColor: Theme.accent
    property bool isActive: true
    property bool hasConflict: false
    signal clicked(string courseId)

    radius: Theme.smallRadius
    color: isActive ? cardColor : Theme.controlPressed
    border.width: hasConflict ? 2 : 1
    border.color: hasConflict ? Theme.danger : Qt.darker(color, 1.18)
    opacity: isActive ? 1.0 : 0.62
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 1

        Text {
            Layout.fillWidth: true
            text: root.courseName
            font.pixelSize: 11
            font.weight: Theme.weightStrong
            color: root.isActive ? "white" : Theme.text
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            maximumLineCount: 2
        }

        Text {
            Layout.fillWidth: true
            visible: text.length > 0
            text: root.courseLocation
            font.pixelSize: 9
            color: root.isActive ? "#f2f2f2" : Theme.mutedText
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            visible: text.length > 0
            text: root.courseTeacher
            font.pixelSize: 9
            color: root.isActive ? "#f2f2f2" : Theme.mutedText
            elide: Text.ElideRight
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 3
        width: 15
        height: 15
        radius: 7.5
        visible: root.hasConflict
        color: Theme.danger

        Text {
            anchors.centerIn: parent
            text: "!"
            color: "white"
            font.pixelSize: 9
            font.bold: true
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked(root.courseId)
    }
}

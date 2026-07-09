import QtQuick 2.15
import QtQuick.Controls 2.15

// A single course card displayed in the grid

Rectangle {
    id: courseCard

    property string courseId: ""
    property string courseName: ""
    property string courseLocation: ""
    property string courseTeacher: ""
    property color cardColor: "#42A5F5"
    property bool isActive: true
    property bool hasConflict: false
    property int trackIndex: 0
    property int totalTracks: 1

    radius: 6
    border.width: 1
    border.color: Qt.darker(cardColor, 1.2)

    color: isActive ? cardColor : "#ccc"

    // Opacity for inactive courses
    opacity: isActive ? 1.0 : 0.5

    // Conflict badge
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        width: 18
        height: 18
        radius: 9
        color: "#f44336"
        visible: hasConflict && isActive
        border.color: "#fff"
        border.width: 1

        Label {
            anchors.centerIn: parent
            text: "!"
            color: "#fff"
            font.pixelSize: 11
            font.bold: true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 2

        Label {
            text: courseName
            font.pixelSize: 11
            font.bold: true
            color: isActive ? "#fff" : "#666"
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        Label {
            text: courseLocation
            font.pixelSize: 9
            color: isActive ? "#e0e0e0" : "#888"
            elide: Text.ElideRight
            visible: text !== ""
            Layout.fillWidth: true
        }

        Label {
            text: courseTeacher
            font.pixelSize: 9
            color: isActive ? "#e0e0e0" : "#888"
            elide: Text.ElideRight
            visible: text !== ""
            Layout.fillWidth: true
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            // Open edit dialog for this course
            // courseEditDialog.openForEdit(courseId)
        }
    }
}

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../styles"

Rectangle {
    property var course
    property bool selected: false
    property bool selectable: false

    signal toggled(string key)

    Layout.fillWidth: true
    implicitHeight: body.implicitHeight + 24
    radius: Theme.cardRadius
    color: selected ? Theme.primarySoft : Theme.surface
    border.color: selected ? Theme.primary : Theme.border

    ColumnLayout {
        id: body
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: course.courseName
                font.pixelSize: Theme.fontBody
                font.weight: Theme.weightStrong
                color: Theme.text
                elide: Text.ElideRight
            }

            Text {
                text: course.gradeName
                font.pixelSize: Theme.fontBody
                font.weight: Theme.weightStrong
                color: course.passed ? Theme.primary : Theme.danger
            }
        }

        Text {
            Layout.fillWidth: true
            visible: course.englishCourseName && course.englishCourseName.length > 0
            text: course.englishCourseName
            font.pixelSize: Theme.fontCaption
            color: Theme.mutedText
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: [course.courseAttributeName, course.credit + " 学分", "绩点 " + course.gradePointScore, "分数 " + course.courseScore].join(" · ")
            font.pixelSize: Theme.fontCaption
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: parent.selectable
        cursorShape: Qt.PointingHandCursor
        onClicked: parent.toggled(course.key)
    }
}

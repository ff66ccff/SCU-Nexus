import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Card {
    id: root
    objectName: "gradeCourseCard"

    property var course: ({})
    property bool selected: false
    property bool selectable: false

    signal toggled(string key)

    Layout.fillWidth: true
    implicitHeight: body.implicitHeight + 2 * Theme.spacing12
    color: selected ? Theme.primarySoft : Theme.surface
    elevation: selected ? 0 : 1

    RowLayout {
        id: body
        anchors.fill: parent
        anchors.margins: Theme.spacing12
        spacing: Theme.spacing16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing4

            Text {
                Layout.fillWidth: true
                text: root.course.courseName || "未命名课程"
                font.pixelSize: Theme.fontBody
                font.weight: Theme.weightStrong
                color: Theme.text
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                visible: root.course.englishCourseName
                         && root.course.englishCourseName.length > 0
                text: root.course.englishCourseName || ""
                font.pixelSize: Theme.fontCaption
                color: Theme.mutedText
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: [root.course.courseAttributeName,
                       root.course.credit + " 学分",
                       root.course.rawScore !== undefined
                           && root.course.rawScore !== null
                           && root.course.rawScore !== ""
                           ? "原始成绩 " + root.course.rawScore : "",
                       "绩点 " + root.course.gradePointScore]
                      .filter(function(value) {
                          return value && value.length > 0
                      }).join(" · ")
                font.pixelSize: Theme.fontCaption
                color: Theme.mutedText
                wrapMode: Text.WordWrap
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 84
            Layout.alignment: Qt.AlignVCenter
            spacing: Theme.spacing4

            Text {
                Layout.fillWidth: true
                text: root.course.gradeName || "—"
                font.pixelSize: Theme.fontSection
                font.weight: Theme.weightStrong
                color: root.course.passed ? Theme.accent : Theme.danger
                horizontalAlignment: Text.AlignRight
            }

            Text {
                Layout.fillWidth: true
                text: "分数 " + root.course.courseScore
                font.pixelSize: Theme.fontCaption
                color: Theme.mutedText
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: root.selectable
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(root.course.key)
    }
}

pragma ComponentBehavior: Bound

import QtQuick 2.15
import "../../styles"

Item {
    id: root

    property var courseModel
    property var timeSlots: []
    property int sectionsPerDay: 12
    property int timeColumnWidth: 76
    property int sectionHeight: 64
    property int headerHeight: 38
    readonly property real dayWidth: (width - timeColumnWidth) / 7

    signal emptySlotClicked(int dayOfWeek, int section)
    signal courseClicked(string courseId)

    implicitWidth: 980
    implicitHeight: headerHeight + sectionsPerDay * sectionHeight

    function colorFromArgb(value) {
        var number = Number(value)
        var red = Math.floor(number / 65536) % 256
        var green = Math.floor(number / 256) % 256
        var blue = number % 256
        return Qt.rgba(red / 255, green / 255, blue / 255, 1)
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
        border.color: Theme.border
    }

    Rectangle {
        x: 0
        y: 0
        width: root.timeColumnWidth
        height: root.headerHeight
        color: Theme.surfaceMuted
        border.color: Theme.border
        Text {
            anchors.centerIn: parent
            text: "节次"
            color: Theme.mutedText
            font.pixelSize: Theme.fontCaption
        }
    }

    Repeater {
        model: ["周一", "周二", "周三", "周四", "周五", "周六", "周日"]
        delegate: Rectangle {
            required property int index
            required property string modelData
            x: root.timeColumnWidth + index * root.dayWidth
            y: 0
            width: root.dayWidth
            height: root.headerHeight
            color: Theme.surfaceMuted
            border.color: Theme.border
            Text {
                anchors.centerIn: parent
                text: modelData
                color: index >= 5 ? Theme.mutedText : Theme.text
                font.pixelSize: Theme.fontLabel
                font.weight: Theme.weightStrong
            }
        }
    }

    Repeater {
        model: root.sectionsPerDay
        delegate: Rectangle {
            required property int index
            x: 0
            y: root.headerHeight + index * root.sectionHeight
            width: root.timeColumnWidth
            height: root.sectionHeight
            color: Theme.surfaceMuted
            border.color: Theme.border

            Column {
                anchors.centerIn: parent
                spacing: 1
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: index + 1
                    color: Theme.text
                    font.pixelSize: Theme.fontCaption
                    font.bold: true
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: index < root.timeSlots.length
                    text: visible ? root.timeSlots[index].startTime + "\n" + root.timeSlots[index].endTime : ""
                    horizontalAlignment: Text.AlignHCenter
                    color: Theme.mutedText
                    font.pixelSize: 9
                }
            }
        }
    }

    Repeater {
        model: 7 * root.sectionsPerDay
        delegate: Rectangle {
            required property int index
            readonly property int dayIndex: Math.floor(index / root.sectionsPerDay)
            readonly property int sectionIndex: index % root.sectionsPerDay
            x: root.timeColumnWidth + dayIndex * root.dayWidth
            y: root.headerHeight + sectionIndex * root.sectionHeight
            width: root.dayWidth
            height: root.sectionHeight
            color: cellMouse.containsMouse ? Theme.subtleHover : "transparent"
            border.color: Theme.border

            MouseArea {
                id: cellMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onDoubleClicked: root.emptySlotClicked(parent.dayIndex + 1,
                                                        parent.sectionIndex + 1)
            }
        }
    }

    Repeater {
        model: root.courseModel
        delegate: Item {
            id: courseDelegate
            required property string courseId
            required property string courseName
            required property string teacher
            required property string location
            required property int dayOfWeek
            required property int startSection
            required property int endSection
            required property var colorValue
            required property bool active
            required property bool conflict
            required property int track
            required property int totalTracks

            x: root.timeColumnWidth
               + (dayOfWeek - 1) * root.dayWidth
               + track * root.dayWidth / Math.max(1, totalTracks) + 2
            y: root.headerHeight + (startSection - 1) * root.sectionHeight + 2
            width: root.dayWidth / Math.max(1, totalTracks) - 4
            height: (endSection - startSection + 1) * root.sectionHeight - 4
            z: 2

            CourseCard {
                anchors.fill: parent
                courseId: courseDelegate.courseId
                courseName: courseDelegate.courseName
                courseTeacher: courseDelegate.teacher
                courseLocation: courseDelegate.location
                cardColor: root.colorFromArgb(courseDelegate.colorValue)
                isActive: courseDelegate.active
                hasConflict: courseDelegate.conflict
                onClicked: function(id) { root.courseClicked(id) }
            }
        }
    }
}

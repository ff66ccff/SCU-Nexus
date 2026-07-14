import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Card {
    id: root

    property var exam: ({})

    Layout.fillWidth: true
    implicitHeight: content.implicitHeight + 2 * Theme.spacing16
    elevation: exam.isPast ? 0 : 1
    opacity: exam.isPast ? 0.68 : 1

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 1
        width: 4
        radius: 2
        color: root.exam.isPast ? Theme.placeholder : Theme.accent
    }

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing20
        anchors.rightMargin: Theme.spacing16
        anchors.topMargin: Theme.spacing16
        anchors.bottomMargin: Theme.spacing16
        spacing: Theme.spacing8

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing12

            Text {
                Layout.fillWidth: true
                text: root.exam.courseName || "未命名考试"
                font.pixelSize: Theme.fontSection
                font.weight: Theme.weightStrong
                color: Theme.text
                elide: Text.ElideRight
            }

            Text {
                text: root.exam.isPast ? "已结束" : "待考试"
                font.pixelSize: Theme.fontCaption
                font.weight: Theme.weightStrong
                color: root.exam.isPast ? Theme.mutedText : Theme.accent
            }
        }

        Text {
            Layout.fillWidth: true
            text: [root.exam.week, root.exam.date, root.exam.weekday,
                   root.exam.timeRange].filter(function(value) {
                       return value && value.length > 0
                   }).join(" · ")
            font.pixelSize: Theme.fontBody
            color: Theme.text
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            text: "地点：" + (root.exam.location || "待定")
                  + (root.exam.seatNumber
                     ? "  ·  座位：" + root.exam.seatNumber : "")
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            visible: root.exam.ticketNumber
                     && root.exam.ticketNumber.length > 0
            text: "准考证号：" + root.exam.ticketNumber
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            visible: root.exam.tip && root.exam.tip.length > 0
                     && root.exam.tip !== "无"
            text: root.exam.tip
            font.pixelSize: Theme.fontLabel
            color: Theme.accent
            wrapMode: Text.WordWrap
        }
    }
}

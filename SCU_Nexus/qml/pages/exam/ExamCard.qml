import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../styles"

Rectangle {
    id: root

    property var exam

    Layout.fillWidth: true
    implicitHeight: content.implicitHeight + 28
    radius: Theme.cardRadius
    color: Theme.surface
    border.color: Theme.border
    opacity: exam.isPast ? 0.62 : 1

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                Layout.fillWidth: true
                text: exam.courseName
                font.pixelSize: Theme.fontSection
                font.weight: Theme.weightStrong
                color: Theme.text
                elide: Text.ElideRight
            }

            Text {
                text: exam.isPast ? "已结束" : "未开始"
                font.pixelSize: Theme.fontCaption
                color: exam.isPast ? Theme.mutedText : Theme.primary
            }
        }

        Text {
            Layout.fillWidth: true
            text: [exam.week, exam.date, exam.weekday, exam.timeRange].filter(function(v) { return v && v.length > 0 }).join(" · ")
            font.pixelSize: Theme.fontBody
            color: Theme.text
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            text: "地点 " + exam.location + (exam.seatNumber ? " · 座位 " + exam.seatNumber : "")
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            visible: exam.ticketNumber && exam.ticketNumber.length > 0
            text: "准考证号 " + exam.ticketNumber
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            visible: exam.tip && exam.tip.length > 0 && exam.tip !== "无"
            text: exam.tip
            font.pixelSize: Theme.fontLabel
            color: Theme.primary
            wrapMode: Text.WordWrap
        }
    }
}

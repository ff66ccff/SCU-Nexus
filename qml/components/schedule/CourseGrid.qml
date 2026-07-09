import QtQuick 2.15
import QtQuick.Layouts 1.15

// Weekly course grid with time columns on the left and day columns

Item {
    id: courseGrid

    property int displayWeek: 1
    property int sectionsPerDay: 12
    property int timeColumnWidth: 80
    property int sectionHeight: 64

    // Header with day names
    RowLayout {
        id: headerRow
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 36
        spacing: 0

        // Time column header
        Rectangle {
            Layout.preferredWidth: timeColumnWidth
            Layout.fillHeight: true
            color: "#e8e8e8"
            border.color: "#d0d0d0"
            Label {
                anchors.centerIn: parent
                text: "时间"
                font.pixelSize: 12
                font.bold: true
            }
        }

        // Day headers
        Repeater {
            model: ["周一", "周二", "周三", "周四", "周五", "周六", "周日"]
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#e8e8e8"
                border.color: "#d0d0d0"

                Label {
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 13
                    font.bold: true
                    color: index >= 5 ? "#999" : "#333"
                }
            }
        }
    }

    // Body
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: headerRow.bottom
        anchors.bottom: parent.bottom
        spacing: 0

        // Time slots column
        ColumnLayout {
            Layout.preferredWidth: timeColumnWidth
            Layout.fillHeight: true
            spacing: 0

            Repeater {
                model: sectionsPerDay
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // Calculate proportional height
                    Layout.preferredHeight: courseGrid.height / sectionsPerDay

                    color: "#fafafa"
                    border.color: "#e8e8e8"
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: (index + 1)
                        font.pixelSize: 12
                        color: "#999"
                    }
                }
            }
        }

        // Day columns
        Repeater {
            model: 7

            DayColumn {
                Layout.fillWidth: true
                Layout.fillHeight: true
                dayIndex: index + 1
                displayWeek: courseGrid.displayWeek
                sectionHeight: courseGrid.sectionHeight
            }
        }
    }
}

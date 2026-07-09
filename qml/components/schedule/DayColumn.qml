import QtQuick 2.15
import QtQuick.Layouts 1.15

// A single day column containing course cards positioned by section

Item {
    id: dayColumn

    property int dayIndex: 1
    property int displayWeek: 1
    property int sectionHeight: 64
    property int sectionsPerDay: 12

    // Background section grid
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Repeater {
            model: sectionsPerDay
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#ffffff"
                border.color: "#e8e8e8"
                border.width: 1

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        // Open add course dialog at this section
                        // courseEditDialog.openForAdd(dayColumn.dayIndex, index + 1)
                    }
                }
            }
        }
    }

    // Course cards are overlaid via a Repeater bound to the model,
    // filtered by dayOfWeek. In production, this would use a QML
    // view/model that exposes courses for this specific day.
    // For now, course cards are placed by the parent CourseGrid.
}

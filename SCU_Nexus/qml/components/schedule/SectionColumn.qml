pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Section/time column on the left side of the grid

Item {
    id: sectionColumn

    property int sectionsPerDay: 12
    property var timeSlots: []  // List of {startTime, endTime}

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Repeater {
            model: sectionColumn.sectionsPerDay

            Rectangle {
                id: sectionDelegate

                required property int index

                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#f5f5f5"
                border.color: "#e0e0e0"
                border.width: 1

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 0

                    Label {
                        text: sectionDelegate.index + 1
                        font.pixelSize: 11
                        font.bold: true
                        color: "#555"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        text: {
                            if (sectionDelegate.index < sectionColumn.timeSlots.length) {
                                var s = sectionColumn.timeSlots[sectionDelegate.index]
                                return s.startTime || ""
                            }
                            return ""
                        }
                        font.pixelSize: 9
                        color: "#999"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }
    }
}

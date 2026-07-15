pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Card {
    id: root

    property var room: ({})
    signal clicked(var room)

    Layout.fillWidth: true
    implicitHeight: content.implicitHeight + 2 * Theme.spacing16
    elevation: 1

    function statusColor(key) {
        if (key === "free") return Theme.success
        if (key === "inClass") return Theme.danger
        if (key === "exam") return "#d97706"
        if (key === "experiment") return "#8b5cf6"
        return "#ca8a04"
    }

    function statusColorWithAlpha(key, alpha) {
        const value = root.statusColor(key)
        return Qt.rgba(value.r, value.g, value.b, alpha)
    }

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: Theme.spacing16
        spacing: Theme.spacing12

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing4

                Text {
                    Layout.fillWidth: true
                    text: root.room.name || root.room.number || "未命名教室"
                    font.pixelSize: Theme.fontSection
                    font.weight: Theme.weightStrong
                    color: Theme.text
                    elide: Text.ElideRight
                }

                Text {
                    text: (root.room.seats || 0) + " 座"
                          + (root.room.borrowable === "是" ? " · 可借用" : "")
                    font.pixelSize: Theme.fontCaption
                    color: Theme.mutedText
                }
            }

            Text {
                text: "›"
                font.pixelSize: 24
                color: Theme.mutedText
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 12
            columnSpacing: Theme.spacing4
            rowSpacing: Theme.spacing4

            Repeater {
                model: root.room.periods || []

                delegate: Rectangle {
                    id: periodCell
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.minimumWidth: 26
                    implicitHeight: 42
                    radius: Theme.smallRadius
                    color: root.statusColorWithAlpha(periodCell.modelData.statusKey, 0.14)
                    border.width: 1
                    border.color: root.statusColorWithAlpha(periodCell.modelData.statusKey, 0.42)

                    Column {
                        anchors.centerIn: parent
                        spacing: 1

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: periodCell.modelData.period
                            font.pixelSize: Theme.fontCaption
                            font.weight: Theme.weightStrong
                            color: Theme.text
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: periodCell.modelData.statusText
                            font.pixelSize: 9
                            color: root.statusColor(periodCell.modelData.statusKey)
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked(root.room)
    }
}

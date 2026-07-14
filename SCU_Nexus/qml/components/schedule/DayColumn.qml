pragma ComponentBehavior: Bound

import QtQuick 2.15
import "../../styles"

Item {
    id: root
    property int dayOfWeek: 1
    property int sectionsPerDay: 12
    property int sectionHeight: 64
    signal slotActivated(int dayOfWeek, int section)

    implicitHeight: sectionsPerDay * sectionHeight

    Repeater {
        model: root.sectionsPerDay
        delegate: Rectangle {
            id: slotCell

            required property int index
            y: slotCell.index * root.sectionHeight
            width: root.width
            height: root.sectionHeight
            color: cell.containsMouse ? Theme.subtleHover : Theme.surface
            border.color: Theme.border
            MouseArea {
                id: cell
                anchors.fill: parent
                hoverEnabled: true
                onDoubleClicked: root.slotActivated(root.dayOfWeek,
                                                    slotCell.index + 1)
            }
        }
    }
}

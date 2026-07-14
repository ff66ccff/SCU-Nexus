pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

GridLayout {
    id: root

    property var summary
    property var metrics: []

    columns: root.width >= 900 ? 4 : (root.width >= 520 ? 2 : 1)
    rowSpacing: Theme.spacing12
    columnSpacing: Theme.spacing12

    Repeater {
        model: root.metrics

        Card {
            id: metricCard

            required property var modelData

            Layout.fillWidth: true
            implicitHeight: 86

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacing12
                spacing: Theme.spacing4

                Text {
                    Layout.fillWidth: true
                    text: metricCard.modelData.label
                    font.pixelSize: Theme.fontCaption
                    color: Theme.mutedText
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: root.format(root.summary
                                      ? root.summary[metricCard.modelData.key]
                                      : 0)
                    font.pixelSize: 20
                    font.weight: Theme.weightStrong
                    color: Theme.text
                    elide: Text.ElideRight
                }
            }
        }
    }

    function format(value) {
        if (value === undefined || value === null) return "0"
        if (typeof value === "number") return Math.round(value * 100) / 100
        return value
    }
}

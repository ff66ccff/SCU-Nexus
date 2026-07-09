import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

GridLayout {
    id: root

    property var summary
    property var metrics: []

    columns: width > 720 ? 4 : 2
    rowSpacing: 10
    columnSpacing: 10

    Repeater {
        model: metrics

        Card {
            Layout.fillWidth: true
            implicitHeight: 74

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: modelData.label
                    font.pixelSize: Theme.fontCaption
                    color: Theme.mutedText
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: format(summary ? summary[modelData.key] : 0)
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

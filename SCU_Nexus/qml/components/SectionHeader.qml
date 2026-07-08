import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles"

RowLayout {
    id: root

    property string title: ""
    property string description: ""

    spacing: 10

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Text {
            Layout.fillWidth: true
            text: root.title
            font.pixelSize: Theme.fontSection
            font.weight: Theme.weightStrong
            color: Theme.text
        }

        Text {
            Layout.fillWidth: true
            visible: root.description.length > 0
            text: root.description
            font.pixelSize: Theme.fontCaption
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }
    }
}

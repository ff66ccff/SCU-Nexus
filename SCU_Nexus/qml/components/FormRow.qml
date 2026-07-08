import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles"

RowLayout {
    id: root

    property string label: ""
    property string helperText: ""

    spacing: 16

    ColumnLayout {
        Layout.preferredWidth: 180
        spacing: 2

        Text {
            text: root.label
            font.pixelSize: Theme.fontBody
            color: Theme.text
        }

        Text {
            visible: root.helperText.length > 0
            text: root.helperText
            font.pixelSize: Theme.fontCaption
            color: Theme.mutedText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}

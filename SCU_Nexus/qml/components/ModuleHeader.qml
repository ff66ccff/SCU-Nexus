import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles"

ColumnLayout {
    id: root

    property string title: ""
    property string subtitle: ""

    spacing: 4

    Text {
        Layout.fillWidth: true
        text: root.title
        font.pixelSize: Theme.fontTitle
        font.weight: Theme.weightStrong
        color: Theme.text
    }

    Text {
        Layout.fillWidth: true
        visible: root.subtitle.length > 0
        text: root.subtitle
        font.pixelSize: Theme.fontLabel
        color: Theme.mutedText
        wrapMode: Text.WordWrap
    }
}

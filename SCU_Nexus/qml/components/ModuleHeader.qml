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
        font.pixelSize: 22
        font.weight: Font.DemiBold
        color: Theme.text
    }

    Text {
        Layout.fillWidth: true
        visible: root.subtitle.length > 0
        text: root.subtitle
        font.pixelSize: 13
        color: Theme.mutedText
        wrapMode: Text.WordWrap
    }
}

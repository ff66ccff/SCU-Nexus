import QtQuick 2.15
import "../../styles"

Text {
    property var value

    text: value && value.toString().length > 0
          ? "更新于 " + value.toLocaleString(Qt.locale(), Locale.ShortFormat)
          : "尚未更新"
    font.pixelSize: Theme.fontCaption
    color: Theme.mutedText
    elide: Text.ElideRight
}

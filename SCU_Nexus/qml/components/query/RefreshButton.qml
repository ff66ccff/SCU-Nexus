import QtQuick 2.15
import ".."

AppButton {
    property bool loading: false
    signal refreshRequested()

    text: "刷新"
    iconText: "↻"
    type: "secondary"
    busy: loading
    onClicked: refreshRequested()
}

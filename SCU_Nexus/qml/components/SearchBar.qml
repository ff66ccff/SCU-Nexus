import QtQuick 2.15

AppTextField {
    id: root

    property string query: text
    signal searchRequested(string query)

    placeholderText: "搜索"
    clearable: true
    onAccepted: root.searchRequested(root.text)
}

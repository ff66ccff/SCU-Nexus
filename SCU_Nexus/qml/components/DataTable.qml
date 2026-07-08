import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../styles"

Item {
    id: root

    property var columns: []
    property var rows: []
    property int rowHeight: 40

    implicitHeight: 240

    Rectangle {
        anchors.fill: parent
        radius: Theme.cardRadius
        color: Theme.surface
        border.color: Theme.border

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: Math.max(root.width, root.columns.length * 150)

            Column {
                width: parent.contentWidth

                Row {
                    width: parent.width
                    height: root.rowHeight

                    Repeater {
                        model: root.columns
                        delegate: Rectangle {
                            width: Math.max(150, root.width / Math.max(1, root.columns.length))
                            height: root.rowHeight
                            color: Theme.surfaceMuted
                            border.color: Theme.border

                            Text {
                                anchors.fill: parent
                                anchors.margins: 10
                                text: modelData
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: Theme.text
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                Repeater {
                    model: root.rows
                    delegate: Row {
                        id: rowDelegate
                        property var rowData: modelData
                        width: parent.width
                        height: root.rowHeight

                        Repeater {
                            model: root.columns
                            delegate: Rectangle {
                                width: Math.max(150, root.width / Math.max(1, root.columns.length))
                                height: root.rowHeight
                                color: index % 2 === 0 ? Theme.surface : Theme.surfaceMuted
                                border.color: Theme.border

                                Text {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    text: rowText(modelData, rowDelegate.rowData)
                                    font.pixelSize: 13
                                    color: Theme.text
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }

        EmptyView {
            anchors.fill: parent
            visible: root.rows.length === 0
            title: "暂无数据"
        }
    }

    function rowText(column, row) {
        if (row === undefined || row === null) return ""
        if (Array.isArray(row)) {
            var columnIndex = root.columns.indexOf(column)
            return columnIndex >= 0 && columnIndex < row.length ? row[columnIndex] : ""
        }
        return row[column] !== undefined ? row[column] : ""
    }
}

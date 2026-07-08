import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../components"
import "../styles"

Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        ModuleHeader {
            Layout.fillWidth: true
            title: "教务成绩"
            subtitle: "B 返回原始 JSON，D 负责成绩模型、统计、缓存和页面状态。"
        }

        DataTable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: ["课程", "成绩", "学分", "绩点", "类型"]
            rows: []
        }
    }
}

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
            title: "考表查询"
            subtitle: "B 提供结构化考表数据，D 负责排序、缓存和展示。"
        }

        DataTable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: ["课程", "日期", "时间", "地点", "座位"]
            rows: []
        }
    }
}

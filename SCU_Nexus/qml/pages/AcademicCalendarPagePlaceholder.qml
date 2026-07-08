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
            title: "校历查询"
            subtitle: "校历查询不要求登录，页面与抓取逻辑由 D 接入。"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.cardRadius
            color: Theme.surface
            border.color: Theme.border

            EmptyView {
                anchors.fill: parent
                title: "校历页面待接入"
                description: "这里不会显示登录提示；D 接入公开校历数据后替换此占位页。"
            }
        }
    }
}

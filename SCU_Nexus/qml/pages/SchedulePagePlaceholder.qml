import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import "../styles"

Item {
    id: root

    property int stateIndex: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        ModuleHeader {
            Layout.fillWidth: true
            title: "课表管理"
            subtitle: "课表本地入口可离线进入；在线导入由 C 的 ViewModel 在需要时请求登录。"
        }

        RowLayout {
            spacing: 8

            AppButton { text: "加载"; type: stateIndex === 0 ? "primary" : "secondary"; onClicked: stateIndex = 0 }
            AppButton { text: "空数据"; type: stateIndex === 1 ? "primary" : "secondary"; onClicked: stateIndex = 1 }
            AppButton { text: "错误"; type: stateIndex === 2 ? "primary" : "secondary"; onClicked: stateIndex = 2 }
            AppButton { text: "需登录"; type: stateIndex === 3 ? "primary" : "secondary"; onClicked: stateIndex = 3 }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.cardRadius
            color: Theme.surface
            border.color: Theme.border

            Loader {
                anchors.fill: parent
                sourceComponent: {
                    if (stateIndex === 0) return loadingView
                    if (stateIndex === 1) return emptyView
                    if (stateIndex === 2) return errorView
                    return loginRequiredView
                }
            }
        }
    }

    Component { id: loadingView; LoadingView { text: "加载课表中..." } }

    Component {
        id: emptyView
        EmptyView {
            title: "暂无课表"
            description: "当前还没有本地课表数据。"
            actionText: "导入课表"
            onActionTriggered: toastManager.show("导入课表入口等待 C 接入。")
        }
    }

    Component {
        id: errorView
        ErrorView {
            title: "课表加载失败"
            message: "错误文案将由 C 的 ViewModel 提供，A 只负责展示。"
            onRetry: toastManager.show("课表重试入口已预留。")
        }
    }

    Component {
        id: loginRequiredView
        LoginRequiredView {
            message: "在线导入课表需要登录教务系统"
            onLoginRequested: router.navigate("Login")
        }
    }
}

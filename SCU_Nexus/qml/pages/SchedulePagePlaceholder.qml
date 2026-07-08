import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    color: "transparent"

    property int stateIndex: 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20

        RowLayout {
            spacing: 8
            Button {
                text: "加载中"
                onClicked: stateIndex = 0
            }
            Button {
                text: "空数据"
                onClicked: stateIndex = 1
            }
            Button {
                text: "错误"
                onClicked: stateIndex = 2
            }
            Button {
                text: "未登录"
                onClicked: stateIndex = 3
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#fafafa"
            radius: 8

            Loader {
                anchors.fill: parent
                sourceComponent: {
                    switch(stateIndex) {
                        case 0: return loadingView
                        case 1: return emptyView
                        case 2: return errorView
                        case 3: return loginRequiredView
                        default: return loadingView
                    }
                }
            }
        }
    }

    Component {
        id: loadingView
        LoadingView { text: "加载课表中..." }
    }

    Component {
        id: emptyView
        EmptyView {
            title: "暂无课表"
            description: "当前学期还没有课程安排"
            actionText: "刷新"
            onActionTriggered: console.log("刷新")
        }
    }

    Component {
        id: errorView
        ErrorView {
            title: "加载失败"
            message: "网络连接异常，请检查网络设置"
            onRetry: console.log("重试")
        }
    }

    Component {
        id: loginRequiredView
        LoginRequiredView {
            message: "请先登录后查看课表"
            onLoginRequested: router.navigate("Login")
        }
    }
}
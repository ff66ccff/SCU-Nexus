import QtQuick 2.15
import ".."

Item {
    id: root

    property int queryState: 0
    property string errorMessage: ""
    property string emptyTitle: "暂无数据"
    property string emptyDescription: ""
    property string loginMessage: "请先登录后查看"
    signal retry()
    signal loginRequested()

    readonly property bool firstLoading: queryState === 1
    visible: firstLoading || queryState === 4 || queryState === 5 || queryState === 6

    LoadingView {
        anchors.fill: parent
        visible: root.firstLoading
    }

    EmptyView {
        anchors.fill: parent
        visible: root.queryState === 4
        title: root.emptyTitle
        description: root.emptyDescription
        actionText: "刷新"
        onActionTriggered: root.retry()
    }

    ErrorView {
        anchors.fill: parent
        visible: root.queryState === 5
        message: root.errorMessage
        onRetry: root.retry()
    }

    LoginRequiredView {
        anchors.fill: parent
        visible: root.queryState === 6
        message: root.loginMessage
        onLoginRequested: root.loginRequested()
    }
}

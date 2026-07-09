import QtQuick 2.15
import ".."

Item {
    id: root

    property int state: 0
    property bool loading: false
    property string errorMessage: ""
    property string emptyTitle: "暂无数据"
    property string emptyDescription: ""
    property string loginMessage: "请先登录后查看"
    signal retry()
    signal loginRequested()

    visible: loading || state === 3 || state === 4 || state === 5

    LoadingView {
        anchors.fill: parent
        visible: root.loading
    }

    EmptyView {
        anchors.fill: parent
        visible: !root.loading && root.state === 3
        title: root.emptyTitle
        description: root.emptyDescription
        actionText: "刷新"
        onActionTriggered: root.retry()
    }

    ErrorView {
        anchors.fill: parent
        visible: !root.loading && root.state === 4
        message: root.errorMessage
        onRetry: root.retry()
    }

    LoginRequiredView {
        anchors.fill: parent
        visible: !root.loading && root.state === 5
        message: root.loginMessage
        onLoginRequested: root.loginRequested()
    }
}

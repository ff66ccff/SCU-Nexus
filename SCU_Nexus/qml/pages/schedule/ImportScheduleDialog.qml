pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Dialog {
    id: root

    title: "从教务系统导入课表"
    width: 520
    height: 480
    modal: true

    property bool loading: false
    property string statusText: ""
    property string errorText: ""
    property var semesters: []
    property int selectedIndex: -1

    signal importClicked(string planCode, string label)
    signal refreshClicked()

    onSemestersChanged: root.selectedIndex = -1
    onOpened: root.selectedIndex = -1

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        visible: !scheduleImportViewModel.loginRequired

        Text {
            Layout.fillWidth: true
            text: root.loading ? "正在访问教务系统..." : root.statusText
            visible: text.length > 0
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            text: root.errorText
            visible: text.length > 0
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: root.loading
            visible: running
        }

        ListView {
            id: semesterList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.semesters
            spacing: 4

            delegate: ItemDelegate {
                required property int index
                required property var modelData
                width: semesterList.width
                highlighted: root.selectedIndex === index
                text: modelData.label + (modelData.isCurrent ? "  （当前）" : "")
                onClicked: root.selectedIndex = index
            }

            Label {
                anchors.centerIn: parent
                visible: !root.loading && root.semesters.length === 0
                text: "暂无学期数据，请刷新或检查登录状态。"
                color: Theme.mutedText
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "刷新列表"
                enabled: !root.loading
                onClicked: root.refreshClicked()
            }
            Item { Layout.fillWidth: true }
            Button { text: "取消"; onClicked: root.close() }
            Button {
                text: "导入所选学期"
                highlighted: true
                enabled: root.selectedIndex >= 0
                         && root.selectedIndex < root.semesters.length
                         && !root.loading
                onClicked: {
                    if (root.selectedIndex < 0 || root.selectedIndex >= root.semesters.length)
                        return
                    var semester = root.semesters[root.selectedIndex]
                    root.importClicked(semester.planCode, semester.label)
                }
            }
        }
    }

    LoginRequiredView {
        anchors.fill: parent
        visible: scheduleImportViewModel.loginRequired
        message: "导入教务课表需要先登录"
        onLoginRequested: {
            root.close()
            router.navigate("Login")
        }
    }
}

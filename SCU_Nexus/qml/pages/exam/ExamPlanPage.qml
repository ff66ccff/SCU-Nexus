import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../components/query"
import "../../styles"

Item {
    Component.onCompleted: examPlanViewModel.load()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        RowLayout {
            Layout.fillWidth: true

            ModuleHeader {
                Layout.fillWidth: true
                title: "考表"
                subtitle: examPlanViewModel.loading && examPlanViewModel.hasCache
                    ? "正在刷新，当前显示缓存考表"
                    : "按考试开始时间排序"
            }

            RefreshButton {
                loading: examPlanViewModel.loading
                onRefreshRequested: examPlanViewModel.refresh()
            }
        }

        LastUpdatedLabel {
            Layout.fillWidth: true
            value: examPlanViewModel.lastUpdated
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent
                visible: !statePane.visible
                clip: true

                ColumnLayout {
                    width: Math.max(parent.width - 16, 320)
                    spacing: 12

                    Repeater {
                        model: examPlanViewModel.exams
                        ExamCard { exam: modelData }
                    }
                }
            }

            QueryStatePane {
                id: statePane
                anchors.fill: parent
                queryState: examPlanViewModel.state
                errorMessage: examPlanViewModel.errorMessage
                emptyTitle: "暂无考试安排"
                loginMessage: "考表查询需要登录教务系统"
                onRetry: examPlanViewModel.refresh()
                onLoginRequested: router.navigate("Login")
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components/query"
import "../../styles"

Item {
    property var groups: []

    Component.onCompleted: refreshGroups()

    Connections {
        target: gradesViewModel
        function onSchemeChanged() { refreshGroups() }
        function onSearchQueryChanged() { refreshGroups() }
    }

    function refreshGroups() {
        groups = gradesViewModel.filteredSchemeGroups()
    }

    ScrollView {
        anchors.fill: parent
        visible: !statePane.visible
        clip: true

        ColumnLayout {
            width: Math.max(parent.width - 16, 320)
            spacing: 14

            GradeSummaryPanel {
                Layout.fillWidth: true
                summary: gradesViewModel.schemeSummary
                metrics: [
                    { label: "GPA", key: "gpa" },
                    { label: "必修 GPA", key: "requiredGpa" },
                    { label: "通过门数", key: "passedCount" },
                    { label: "未通过门数", key: "failedCount" },
                    { label: "平均分", key: "weightedAvgScore" },
                    { label: "必修平均分", key: "requiredWeightedAvgScore" },
                    { label: "已修学分", key: "earnedCredits" },
                    { label: "要求学分", key: "totalCredits" }
                ]
            }

            Repeater {
                model: groups

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        Layout.fillWidth: true
                        text: modelData.label
                        font.pixelSize: Theme.fontSection
                        font.weight: Theme.weightStrong
                        color: Theme.text
                    }

                    Repeater {
                        model: modelData.items
                        GradeCourseCard { course: modelData }
                    }
                }
            }
        }
    }

    QueryStatePane {
        id: statePane
        anchors.fill: parent
        state: gradesViewModel.schemeState
        loading: gradesViewModel.schemeState === 1
        errorMessage: gradesViewModel.schemeErrorMessage
        emptyTitle: "暂无方案成绩"
        loginMessage: "教务成绩需要登录教务系统"
        onRetry: gradesViewModel.refreshSchemeScores()
        onLoginRequested: router.navigate("Login")
    }
}

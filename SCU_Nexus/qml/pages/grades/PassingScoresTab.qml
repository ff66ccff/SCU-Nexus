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
        function onPassingChanged() { refreshGroups() }
        function onSearchQueryChanged() { refreshGroups() }
    }

    function refreshGroups() {
        groups = gradesViewModel.filteredPassingGroups()
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
                summary: gradesViewModel.passingSummary
                metrics: [
                    { label: "总 GPA", key: "gpa" },
                    { label: "累计学分", key: "totalCredits" },
                    { label: "总通过门数", key: "totalPassed" },
                    { label: "学期数", key: "termCount" },
                    { label: "平均分", key: "weightedAvgScore" },
                    { label: "必修平均分", key: "requiredWeightedAvgScore" }
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
        queryState: gradesViewModel.passingState
        errorMessage: gradesViewModel.passingErrorMessage
        emptyTitle: "暂无及格成绩"
        loginMessage: "教务成绩需要登录教务系统"
        onRetry: gradesViewModel.refreshPassingScores()
        onLoginRequested: router.navigate("Login")
    }
}

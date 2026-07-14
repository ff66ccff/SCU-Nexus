import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components/query"
import "../../styles"

// Context properties are intentionally injected by main.cpp.
// qmllint disable unqualified

Item {
    id: root
    objectName: "schemeScoresTab"

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

    function displayValue(value, fallback) {
        return value === undefined || value === null || value === "" ? fallback : value
    }

    function groupHeader(group) {
        group = group || ({})
        return displayValue(group.label, "") + " · 通过 "
                + displayValue(group.passedCount, 0) + " 门 · "
                + displayValue(group.credits, 0) + " 学分"
    }

    ScrollView {
        id: schemeScroll
        anchors.fill: parent
        visible: !statePane.visible
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: Math.max(schemeScroll.availableWidth, 320)
            spacing: Theme.spacing16

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
                    { label: "要求学分", key: "totalCredits" },
                    { label: "选修学分", key: "electiveCredits" },
                    { label: "任选学分", key: "optionalCredits" }
                ]
            }

            Repeater {
                model: groups

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        Layout.fillWidth: true
                        text: groupHeader(modelData)
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
        queryState: gradesViewModel.schemeState
        errorMessage: gradesViewModel.schemeErrorMessage
        emptyTitle: "暂无方案成绩"
        loginMessage: "教务成绩需要登录教务系统"
        onRetry: gradesViewModel.refreshSchemeScores()
        onLoginRequested: router.navigate("Login")
    }
}

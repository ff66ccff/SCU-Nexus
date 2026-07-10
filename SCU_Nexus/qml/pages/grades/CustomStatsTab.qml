import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../components/query"
import "../../styles"

Item {
    id: root

    property var selectedKeys: []
    property string attr: "全部"
    property var groups: []
    property var stats: gradesViewModel.customStatsForSelected(selectedKeys)

    Component.onCompleted: refreshGroups()
    onAttrChanged: refreshGroups()

    Connections {
        target: gradesViewModel
        function onSchemeChanged() { refreshGroups() }
        function onSearchQueryChanged() { refreshGroups() }
    }

    function refreshGroups() {
        groups = gradesViewModel.filteredSchemeGroupsByAttr(attr)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: ["全部", "必修", "选修", "任选"]
                AppButton {
                    text: modelData
                    type: root.attr === modelData ? "primary" : "secondary"
                    onClicked: root.attr = modelData
                }
            }

            Item { Layout.fillWidth: true }

            AppButton {
                text: "清空"
                type: "secondary"
                onClicked: root.selectedKeys = []
            }
        }

        GradeSummaryPanel {
            Layout.fillWidth: true
            summary: root.stats
            metrics: [
                { label: "已选课程", key: "selectedCount" },
                { label: "GPA", key: "gpa" },
                { label: "学分", key: "credits" },
                { label: "均分", key: "weightedAvgScore" },
                { label: "必修均分", key: "requiredWeightedAvgScore" },
                { label: "通过门数", key: "passedCount" }
            ]
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !statePane.visible
            clip: true

            ColumnLayout {
                width: Math.max(parent.width - 16, 320)
                spacing: 12

                Repeater {
                    model: root.groups

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                Layout.fillWidth: true
                                text: modelData.label
                                font.pixelSize: Theme.fontSection
                                font.weight: Theme.weightStrong
                                color: Theme.text
                            }
                        }

                        Repeater {
                            model: modelData.items
                            GradeCourseCard {
                                course: modelData
                                selectable: true
                                selected: root.selectedKeys.indexOf(modelData.key) >= 0
                                onToggled: function(key) {
                                    var next = root.selectedKeys.slice()
                                    var index = next.indexOf(key)
                                    if (index >= 0) next.splice(index, 1)
                                    else next.push(key)
                                    root.selectedKeys = next
                                }
                            }
                        }
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
        emptyTitle: "请先获取方案成绩"
        loginMessage: "教务成绩需要登录教务系统"
        onRetry: gradesViewModel.refreshSchemeScores()
        onLoginRequested: router.navigate("Login")
    }
}

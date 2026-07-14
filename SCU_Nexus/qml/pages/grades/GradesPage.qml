pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../components/query"
import "../../styles"

// Context properties are intentionally injected by main.cpp.
// qmllint disable unqualified

Item {
    id: root

    readonly property bool refreshing: gradesViewModel.schemeState === 3
        || gradesViewModel.passingState === 3
    readonly property bool queryBusy: root.refreshing
        || gradesViewModel.schemeState === 1
        || gradesViewModel.passingState === 1

    Component.onCompleted: gradesViewModel.load()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing12

            ModuleHeader {
                Layout.fillWidth: true
                title: "成绩"
                subtitle: root.refreshing
                    ? "正在刷新，当前显示缓存成绩"
                    : "方案成绩、及格成绩和自定义统计"
            }

            RefreshButton {
                loading: root.queryBusy
                onRefreshRequested: {
                    if (tabs.currentIndex === 1)
                        gradesViewModel.refreshPassingScores()
                    else
                        gradesViewModel.refreshSchemeScores()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing8

            SearchBar {
                Layout.fillWidth: true
                Layout.minimumWidth: 180
                text: gradesViewModel.searchQuery
                placeholderText: "搜索课程名称"
                onTextChanged: gradesViewModel.setSearchQuery(text)
                onSearchRequested: function(query) {
                    gradesViewModel.setSearchQuery(query)
                }
            }

            ComboBox {
                Layout.preferredWidth: 150
                model: gradesViewModel.availableCourseTypes
                currentIndex: Math.max(0, model.indexOf(gradesViewModel.selectedCourseType))
                enabled: count > 1
                onActivated: gradesViewModel.setSelectedCourseType(currentText)
            }

            ComboBox {
                Layout.preferredWidth: 190
                model: gradesViewModel.availableTerms
                currentIndex: Math.max(0, model.indexOf(gradesViewModel.selectedTerm))
                enabled: count > 1
                onActivated: gradesViewModel.setSelectedTerm(currentText)
            }
        }

        RowLayout {
            Layout.fillWidth: true

            TabBar {
                id: tabs
                Layout.fillWidth: true

                TabButton { text: "方案成绩" }
                TabButton { text: "及格成绩" }
                TabButton { text: "自定义统计" }
            }

            LastUpdatedLabel {
                Layout.preferredWidth: 220
                value: tabs.currentIndex === 1
                       ? gradesViewModel.passingLastUpdated
                       : gradesViewModel.schemeLastUpdated
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            SchemeScoresTab { }
            PassingScoresTab { }
            CustomStatsTab { }
        }
    }
}

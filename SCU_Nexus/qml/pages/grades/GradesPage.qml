import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../components/query"
import "../../styles"

Item {
    id: root

    Component.onCompleted: gradesViewModel.load()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        RowLayout {
            Layout.fillWidth: true

            ModuleHeader {
                Layout.fillWidth: true
                title: "教务成绩"
                subtitle: "方案成绩、及格成绩和自定义统计"
            }

            RefreshButton {
                loading: gradesViewModel.schemeState === 1 || gradesViewModel.passingState === 1
                onRefreshRequested: {
                    if (tabs.currentIndex === 0 || tabs.currentIndex === 2) gradesViewModel.refreshSchemeScores()
                    else gradesViewModel.refreshPassingScores()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            SearchBar {
                Layout.fillWidth: true
                text: gradesViewModel.searchQuery
                onTextChanged: gradesViewModel.setSearchQuery(text)
                onSearchRequested: gradesViewModel.setSearchQuery(query)
            }

            LastUpdatedLabel {
                Layout.preferredWidth: 220
                value: tabs.currentIndex === 1 ? gradesViewModel.passingLastUpdated : gradesViewModel.schemeLastUpdated
            }
        }

        TabBar {
            id: tabs
            Layout.fillWidth: true

            TabButton { text: "方案成绩" }
            TabButton { text: "及格成绩" }
            TabButton { text: "自定义统计" }
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

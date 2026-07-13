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

    function keysForGroups(sourceGroups) {
        var keys = []
        for (var i = 0; i < sourceGroups.length; ++i)
            for (var j = 0; j < sourceGroups[i].items.length; ++j)
                keys.push(sourceGroups[i].items[j].key)
        return keys
    }

    function setKeysSelected(keys, selected) {
        var next = root.selectedKeys.slice()
        for (var i = 0; i < keys.length; ++i) {
            var index = next.indexOf(keys[i])
            if (selected && index < 0) next.push(keys[i])
            if (!selected && index >= 0) next.splice(index, 1)
        }
        root.selectedKeys = next
    }

    function allKeysSelected(keys) {
        if (keys.length === 0)
            return false
        for (var i = 0; i < keys.length; ++i) {
            if (root.selectedKeys.indexOf(keys[i]) < 0)
                return false
        }
        return true
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AppButton {
                text: "全选当前筛选"
                type: "secondary"
                onClicked: root.setKeysSelected(root.keysForGroups(root.groups), true)
            }

            AppButton {
                text: "取消当前筛选"
                type: "secondary"
                onClicked: root.setKeysSelected(root.keysForGroups(root.groups), false)
            }

            Item { Layout.fillWidth: true }
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
                { label: "通过门数", key: "passedCount" },
                { label: "未通过门数", key: "failedCount" }
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
                        property var groupData: modelData
                        property var groupKeys: root.keysForGroups([groupData])
                        property bool groupSelected: root.allKeysSelected(groupKeys)

                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                Layout.fillWidth: true
                                text: groupData.label
                                font.pixelSize: Theme.fontSection
                                font.weight: Theme.weightStrong
                                color: Theme.text
                            }

                            AppButton {
                                text: groupSelected ? "取消本学期" : "全选本学期"
                                type: "secondary"
                                onClicked: root.setKeysSelected(groupKeys, !groupSelected)
                            }
                        }

                        Repeater {
                            model: groupData.items
                            GradeCourseCard {
                                course: modelData
                                selectable: true
                                selected: root.selectedKeys.indexOf(modelData.key) >= 0
                                onToggled: function(key) {
                                    var isSelected = root.selectedKeys.indexOf(key) >= 0
                                    root.setKeysSelected([key], !isSelected)
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

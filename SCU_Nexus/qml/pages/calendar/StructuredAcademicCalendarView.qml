pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Item {
    id: root

    property var calendarData: ({})
    property int selectedTermIndex: 0
    property date currentDate: new Date()
    readonly property var normalizedCalendar: root.safeMap(root.calendarData)
    readonly property var terms: root.safeMapList(root.normalizedCalendar.terms)
    readonly property var selectedTerm: root.selectedTermIndex >= 0
                                        && root.terms.length > root.selectedTermIndex
                                        ? root.safeMap(root.terms[root.selectedTermIndex]) : ({})
    readonly property bool compact: width < 620
    signal viewOriginalRequested(string imageUrl)

    onCalendarDataChanged: root.selectedTermIndex = 0

    function safeMap(value) {
        if (!root.isMap(value))
            return ({})
        return value
    }

    function isMap(value) {
        return value !== undefined && value !== null && typeof value === "object"
                && !Array.isArray(value) && !(value instanceof Date)
                && typeof value.slice !== "function"
    }

    function safeList(value) {
        if (Array.isArray(value))
            return value
        if (value !== null && typeof value === "object"
                && typeof value.length === "number"
                && typeof value.slice === "function")
            return value
        return []
    }

    function safeMapList(value) {
        const source = root.safeList(value)
        const result = []
        for (let index = 0; index < source.length; ++index) {
            if (root.isMap(source[index]))
                result.push(source[index])
        }
        return result
    }

    function stringValue(value) {
        return value === undefined || value === null ? "" : String(value)
    }

    function formatDate(value) {
        if (value === undefined || value === null || value === "")
            return ""

        if (typeof value === "string") {
            const match = value.match(/^(\d{4})-(\d{1,2})-(\d{1,2})/)
            if (match)
                return match[1] + "年" + Number(match[2]) + "月" + Number(match[3]) + "日"
        }

        try {
            const formatted = Qt.formatDate(value, "yyyy年M月d日")
            return formatted && formatted !== "Invalid Date" ? formatted : root.stringValue(value)
        } catch (error) {
            return root.stringValue(value)
        }
    }

    function dateRange(startDate, endDate) {
        const start = root.formatDate(startDate)
        const end = root.formatDate(endDate)
        if (!start)
            return end
        if (!end || start === end)
            return start
        return start + " – " + end
    }

    function phaseLabel(phase) {
        switch (root.stringValue(phase)) {
        case "teaching": return "教学周"
        case "exam": return "考试周"
        case "practice": return "实践周"
        case "winter-break":
        case "winter_break": return "寒假"
        case "summer-break":
        case "summer_break": return "暑假"
        default: return "其他安排"
        }
    }

    function phaseBackground(phase) {
        switch (root.stringValue(phase)) {
        case "teaching": return Theme.primarySoft
        case "exam": return Theme.dangerSoft
        case "practice": return Theme.surfaceMuted
        case "winter-break":
        case "winter_break": return Theme.controlPressed
        case "summer-break":
        case "summer_break": return Theme.subtleActive
        default: return Theme.control
        }
    }

    function phaseForeground(phase) {
        switch (root.stringValue(phase)) {
        case "teaching": return Theme.accent
        case "exam": return Theme.danger
        case "practice": return Theme.success
        case "summer-break":
        case "summer_break": return Theme.accent
        default: return Theme.mutedText
        }
    }

    function eventTypeLabel(type) {
        switch (root.stringValue(type)) {
        case "registration": return "报到注册"
        case "make-up-exam": return "补缓考"
        case "ceremony": return "典礼"
        case "instruction": return "教学安排"
        case "holiday": return "节假日"
        case "athletics": return "体育活动"
        case "final-exam": return "期末考试"
        case "vacation": return "假期"
        default: return "校历事项"
        }
    }

    function dateSortKey(value) {
        if (value instanceof Date && !isNaN(value.getTime()))
            return value.getTime()
        const text = root.stringValue(value)
        const match = text.match(/^(\d{4})-(\d{1,2})-(\d{1,2})/)
        if (!match)
            return Number.MAX_SAFE_INTEGER
        return Date.UTC(Number(match[1]), Number(match[2]) - 1, Number(match[3]))
    }

    function calendarDayKey(value) {
        if (value instanceof Date && !isNaN(value.getTime()))
            return Date.UTC(value.getFullYear(), value.getMonth(), value.getDate())
        const text = root.stringValue(value)
        const match = text.match(/^(\d{4})-(\d{1,2})-(\d{1,2})/)
        if (!match)
            return Number.NaN
        return Date.UTC(Number(match[1]), Number(match[2]) - 1, Number(match[3]))
    }

    function isDateInRange(value, startDate, endDate) {
        const valueKey = root.calendarDayKey(value)
        const startKey = root.calendarDayKey(startDate)
        const endKey = root.calendarDayKey(endDate)
        return Number.isFinite(valueKey) && Number.isFinite(startKey)
                && Number.isFinite(endKey) && valueKey >= startKey && valueKey <= endKey
    }

    function sortedEvents(events) {
        const source = root.safeList(events)
        const decorated = []
        for (let index = 0; index < source.length; ++index) {
            decorated.push({
                "item": root.safeMap(source[index]),
                "index": index
            })
        }
        decorated.sort(function(left, right) {
            const difference = root.dateSortKey(left.item ? left.item.startDate : undefined)
                             - root.dateSortKey(right.item ? right.item.startDate : undefined)
            return difference === 0 ? left.index - right.index : difference
        })
        const result = []
        for (let index = 0; index < decorated.length; ++index)
            result.push(decorated[index].item)
        return result
    }

    function sortedNotes(notes) {
        const source = root.safeList(notes)
        const decorated = []
        for (let index = 0; index < source.length; ++index) {
            decorated.push({
                "item": root.safeMap(source[index]),
                "index": index
            })
        }
        decorated.sort(function(left, right) {
            const leftOrder = left.item && Number.isFinite(Number(left.item.order))
                            ? Number(left.item.order) : Number.MAX_SAFE_INTEGER
            const rightOrder = right.item && Number.isFinite(Number(right.item.order))
                             ? Number(right.item.order) : Number.MAX_SAFE_INTEGER
            return leftOrder === rightOrder ? left.index - right.index : leftOrder - rightOrder
        })
        const result = []
        for (let index = 0; index < decorated.length; ++index)
            result.push(decorated[index].item)
        return result
    }

    function termSegments() {
        const result = []
        const source = root.safeList(root.terms)
        for (let index = 0; index < source.length; ++index) {
            const term = root.safeMap(source[index])
            result.push({
                "label": root.stringValue(term.name) || "第 " + (index + 1) + " 学期",
                "value": String(index)
            })
        }
        return result
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        contentHeight: contentColumn.implicitHeight + Theme.pagePadding * 2

        ColumnLayout {
            id: contentColumn
            x: Math.max(Theme.pagePadding,
                        (scrollView.availableWidth - width) / 2)
            y: Theme.pagePadding
            width: Math.max(0, Math.min(scrollView.availableWidth - Theme.pagePadding * 2,
                                        1120))
            spacing: Theme.sectionGap

            SegmentedControl {
                objectName: "structuredTermSelector"
                Layout.fillWidth: root.compact
                Layout.preferredWidth: Math.min(implicitWidth, contentColumn.width)
                Layout.maximumWidth: contentColumn.width
                visible: root.terms.length > 1
                model: root.termSegments()
                value: String(root.selectedTermIndex)
                onActivated: function(newValue) {
                    const index = Number(newValue)
                    if (Number.isInteger(index) && index >= 0 && index < root.terms.length)
                        root.selectedTermIndex = index
                }
            }

            Card {
                objectName: "structuredCalendarEmptyState"
                Layout.fillWidth: true
                visible: root.terms.length === 0
                implicitHeight: emptyLayout.implicitHeight + Theme.cardPadding * 2
                elevation: 0

                ColumnLayout {
                    id: emptyLayout
                    anchors.fill: parent
                    anchors.margins: Theme.cardPadding
                    spacing: Theme.spacing4

                    Text {
                        Layout.fillWidth: true
                        text: "暂无结构化校历"
                        font.pixelSize: Theme.fontSection
                        font.weight: Theme.weightStrong
                        color: Theme.text
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "当前校历没有可展示的学期数据"
                        font.pixelSize: Theme.fontBody
                        color: Theme.mutedText
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                implicitHeight: overviewLayout.implicitHeight + Theme.cardPadding * 2

                GridLayout {
                    id: overviewLayout
                    anchors.fill: parent
                    anchors.margins: Theme.cardPadding
                    columns: root.compact ? 1 : 2
                    columnSpacing: Theme.spacing12
                    rowSpacing: Theme.spacing12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing4

                        Text {
                            Layout.fillWidth: true
                            text: root.stringValue(root.normalizedCalendar.title)
                                  || root.stringValue(root.normalizedCalendar.academicYear)
                                  || "学年校历"
                            font.pixelSize: Theme.fontSection
                            font.weight: Theme.weightStrong
                            color: Theme.text
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (root.stringValue(root.selectedTerm.name) || "当前学期")
                                  + " · "
                                  + root.dateRange(root.selectedTerm.startDate,
                                                   root.selectedTerm.endDate)
                            font.pixelSize: Theme.fontLabel
                            color: Theme.mutedText
                            wrapMode: Text.WordWrap
                        }
                    }

                    AppButton {
                        objectName: "structuredCalendarOriginalButton"
                        Layout.alignment: root.compact ? Qt.AlignLeft
                                                      : Qt.AlignRight | Qt.AlignVCenter
                        text: "查看原版"
                        type: "secondary"
                        enabled: root.stringValue(root.selectedTerm.sourceImageUrl).length > 0
                        onClicked: root.viewOriginalRequested(
                            root.stringValue(root.selectedTerm.sourceImageUrl))
                    }
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                title: "教学周次"
                description: root.safeList(root.selectedTerm.weeks).length + " 周"
            }

            GridLayout {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.safeList(root.selectedTerm.weeks).length > 0
                columns: contentColumn.width >= 900 ? 4
                         : (contentColumn.width >= 520 ? 2 : 1)
                columnSpacing: Theme.spacing12
                rowSpacing: Theme.spacing12

                Repeater {
                    model: root.safeList(root.selectedTerm.weeks)

                    delegate: Card {
                        id: weekCard
                        required property var modelData
                        readonly property var weekData: root.safeMap(weekCard.modelData)
                        readonly property bool currentWeek: root.isDateInRange(
                            root.currentDate,
                            weekCard.weekData.startDate,
                            weekCard.weekData.endDate)

                        objectName: "structuredWeekCard-"
                                    + root.stringValue(weekCard.weekData.weekNo)
                        Accessible.role: Accessible.ListItem
                        Accessible.name: "第 " + root.stringValue(weekCard.weekData.weekNo)
                                         + " 周" + (weekCard.currentWeek ? "，当前周" : "")
                        Accessible.description: root.dateRange(
                            weekCard.weekData.startDate, weekCard.weekData.endDate)
                            + "，" + (root.stringValue(weekCard.weekData.label)
                                      || root.phaseLabel(weekCard.weekData.phase))

                        Layout.fillWidth: true
                        implicitHeight: weekLayout.implicitHeight + Theme.spacing12 * 2
                        color: root.phaseBackground(weekCard.weekData.phase)
                        elevation: 0

                        ColumnLayout {
                            id: weekLayout
                            anchors.fill: parent
                            anchors.margins: Theme.spacing12
                            spacing: Theme.spacing4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacing8

                                Text {
                                    Layout.fillWidth: true
                                    text: "第 " + root.stringValue(
                                        weekCard.weekData.weekNo) + " 周"
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Theme.weightStrong
                                    color: Theme.text
                                }
                                Text {
                                    text: root.stringValue(weekCard.weekData.label)
                                          || root.phaseLabel(weekCard.weekData.phase)
                                    font.pixelSize: Theme.fontCaption
                                    font.weight: Theme.weightMedium
                                    color: root.phaseForeground(weekCard.weekData.phase)
                                }
                                Text {
                                    objectName: "structuredWeekCurrentLabel-"
                                                + root.stringValue(weekCard.weekData.weekNo)
                                    visible: weekCard.currentWeek
                                    text: "当前周"
                                    font.pixelSize: Theme.fontCaption
                                    font.weight: Theme.weightStrong
                                    color: Theme.accent
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.dateRange(weekCard.weekData.startDate,
                                                     weekCard.weekData.endDate)
                                font.pixelSize: Theme.fontCaption
                                color: Theme.mutedText
                                wrapMode: Text.WordWrap
                            }
                        }

                        Rectangle {
                            objectName: "structuredWeekCurrentOutline-"
                                        + root.stringValue(weekCard.weekData.weekNo)
                            z: 1
                            anchors.fill: parent
                            radius: weekCard.radius
                            color: "transparent"
                            border.width: weekCard.currentWeek ? 2 : 0
                            border.color: Theme.accent
                            visible: weekCard.currentWeek
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.safeList(root.selectedTerm.weeks).length === 0
                text: "本学期暂无周次安排"
                font.pixelSize: Theme.fontBody
                color: Theme.mutedText
            }

            SectionHeader {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                title: "重要事项"
                description: root.sortedEvents(root.selectedTerm.events).length + " 项"
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.sortedEvents(root.selectedTerm.events).length > 0
                spacing: Theme.spacing8

                Repeater {
                    model: root.sortedEvents(root.selectedTerm.events)

                    delegate: Card {
                        id: eventCard
                        required property int index
                        required property var modelData
                        readonly property var eventData: root.safeMap(eventCard.modelData)

                        objectName: "structuredEventCard-"
                                    + (root.stringValue(eventCard.eventData.id)
                                       || String(eventCard.index))

                        Layout.fillWidth: true
                        implicitHeight: eventLayout.implicitHeight + Theme.spacing12 * 2
                        elevation: 0

                        RowLayout {
                            id: eventLayout
                            anchors.fill: parent
                            anchors.margins: Theme.spacing12
                            spacing: Theme.spacing12

                            Rectangle {
                                Layout.preferredWidth: 4
                                Layout.fillHeight: true
                                radius: 2
                                color: Theme.accent
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacing4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: Theme.spacing8

                                    Text {
                                        Layout.fillWidth: true
                                        text: root.stringValue(eventCard.eventData.title)
                                              || "未命名事项"
                                        font.pixelSize: Theme.fontBody
                                        font.weight: Theme.weightStrong
                                        color: Theme.text
                                        wrapMode: Text.WordWrap
                                    }
                                    Text {
                                        text: root.eventTypeLabel(eventCard.eventData.type)
                                        font.pixelSize: Theme.fontCaption
                                        font.weight: Theme.weightMedium
                                        color: Theme.accent
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: root.dateRange(eventCard.eventData.startDate,
                                                         eventCard.eventData.endDate)
                                    font.pixelSize: Theme.fontCaption
                                    color: Theme.mutedText
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.sortedEvents(root.selectedTerm.events).length === 0
                text: "本学期暂无重要事项"
                font.pixelSize: Theme.fontBody
                color: Theme.mutedText
            }

            SectionHeader {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                title: "校历说明"
                description: root.sortedNotes(root.selectedTerm.notes).length + " 条"
            }

            Card {
                id: notesCard

                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.sortedNotes(root.selectedTerm.notes).length > 0
                implicitHeight: notesLayout.implicitHeight + Theme.cardPadding * 2
                elevation: 0

                ColumnLayout {
                    id: notesLayout
                    anchors.fill: parent
                    anchors.margins: Theme.cardPadding
                    spacing: Theme.spacing12

                    AppButton {
                        id: notesToggle

                        objectName: "structuredNotesToggle"
                        Layout.fillWidth: true
                        type: "ghost"
                        checkable: true
                        text: notesToggle.checked ? "收起全部说明" : "展开全部说明"
                        iconText: notesToggle.checked ? "⌃" : "⌄"
                        Accessible.role: Accessible.Button
                        Accessible.name: notesToggle.text
                        Accessible.description: notesToggle.checked
                                                ? "校历说明已展开" : "校历说明已收起"
                        Accessible.checkable: true
                        Accessible.checked: notesToggle.checked
                        Keys.onReturnPressed: notesToggle.checked = !notesToggle.checked
                        Keys.onEnterPressed: notesToggle.checked = !notesToggle.checked
                    }

                    Repeater {
                        model: root.sortedNotes(root.selectedTerm.notes)

                        delegate: RowLayout {
                            id: noteRow
                            required property int index
                            required property var modelData
                            readonly property var noteData: root.safeMap(noteRow.modelData)

                            objectName: "structuredNote-"
                                        + (root.stringValue(noteRow.noteData.order) || "unordered")
                                        + "-" + noteRow.index
                            Layout.fillWidth: true
                            visible: notesToggle.checked
                            spacing: Theme.spacing8

                            Text {
                                Layout.alignment: Qt.AlignTop
                                text: root.stringValue(noteRow.noteData.order)
                                      || String(noteRow.index + 1)
                                font.pixelSize: Theme.fontCaption
                                font.weight: Theme.weightStrong
                                color: Theme.accent
                            }
                            Text {
                                objectName: "structuredNoteText"
                                Layout.fillWidth: true
                                text: root.stringValue(noteRow.noteData.text)
                                font.pixelSize: Theme.fontBody
                                color: Theme.text
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

            }

            Text {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.sortedNotes(root.selectedTerm.notes).length === 0
                text: "本学期暂无补充说明"
                font.pixelSize: Theme.fontBody
                color: Theme.mutedText
            }
        }
    }
}

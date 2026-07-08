pragma Singleton

import QtQuick 2.15

QtObject {
    readonly property int pagePadding: 20
    readonly property int sectionGap: 16
    readonly property int controlHeight: 36
    readonly property int cardRadius: 8
    readonly property int smallRadius: 4
    readonly property int navWidth: 180

    readonly property color background: themeManager.dark ? "#111827" : "#f4f6f8"
    readonly property color surface: themeManager.dark ? "#1f2937" : "#ffffff"
    readonly property color surfaceMuted: themeManager.dark ? "#18212f" : "#f8fafc"
    readonly property color control: themeManager.dark ? "#253244" : "#f9fafb"
    readonly property color controlPressed: themeManager.dark ? "#334155" : "#eef2f7"
    readonly property color border: themeManager.dark ? "#334155" : "#d9e1ea"
    readonly property color text: themeManager.dark ? "#f8fafc" : "#1f2937"
    readonly property color mutedText: themeManager.dark ? "#aab6c5" : "#667085"
    readonly property color placeholder: themeManager.dark ? "#778397" : "#98a2b3"
    readonly property color primary: "#1867c0"
    readonly property color primaryPressed: "#0f4c91"
    readonly property color danger: "#d92d20"
    readonly property color success: "#039855"
}

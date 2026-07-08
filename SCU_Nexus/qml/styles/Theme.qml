pragma Singleton

import QtQuick 2.15

QtObject {
    // ---- Spacing & metrics (doc §10) ----
    readonly property int pagePadding: 20
    readonly property int sectionGap: 16
    readonly property int cardPadding: 16
    readonly property int controlHeight: 36
    readonly property int cardRadius: 8
    readonly property int smallRadius: 4
    readonly property int navWidth: 180
    readonly property int topBarHeight: 58

    // ---- Typography scale (doc §10 text hierarchy) ----
    readonly property int fontTitle: 22      // 页面标题 (20-24)
    readonly property int fontSection: 16    // 区块标题 (16-18)
    readonly property int fontBody: 14       // 列表/表格正文 (13-15)
    readonly property int fontLabel: 13      // 次要说明/标签
    readonly property int fontCaption: 12    // 辅助说明 (12-13)
    readonly property int fontMicro: 11      // 版本号等极小信息

    readonly property int weightNormal: Font.Normal
    readonly property int weightMedium: Font.Medium
    readonly property int weightStrong: Font.DemiBold

    // ---- Surfaces & lines ----
    readonly property color background: themeManager.dark ? "#111827" : "#f4f6f8"
    readonly property color surface: themeManager.dark ? "#1f2937" : "#ffffff"
    readonly property color surfaceMuted: themeManager.dark ? "#18212f" : "#f8fafc"
    readonly property color control: themeManager.dark ? "#253244" : "#f9fafb"
    readonly property color controlPressed: themeManager.dark ? "#334155" : "#eef2f7"
    readonly property color border: themeManager.dark ? "#334155" : "#d9e1ea"

    // ---- Text ----
    readonly property color text: themeManager.dark ? "#f8fafc" : "#1f2937"
    readonly property color mutedText: themeManager.dark ? "#aab6c5" : "#667085"
    readonly property color placeholder: themeManager.dark ? "#778397" : "#98a2b3"

    // ---- Accents ----
    readonly property color primary: "#1867c0"
    readonly property color primaryPressed: "#0f4c91"
    readonly property color primarySoft: themeManager.dark ? "#123b63" : "#e8f1fb"
    readonly property color danger: "#d92d20"
    readonly property color dangerSoft: themeManager.dark ? "#3a1c1a" : "#fef3f2"
    readonly property color dangerBorder: themeManager.dark ? "#5c2b27" : "#fecdca"
    readonly property color success: "#039855"
}

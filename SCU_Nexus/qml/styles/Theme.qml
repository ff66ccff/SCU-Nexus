pragma Singleton

import QtQuick 2.15

// 设计令牌单例。
// 视觉基调对齐 FluentWinUI3（Windows 11 Fluent Design）：
//   - 中性表面采用 Mica / Card / Layer 三层灰阶；
//   - 强调色统一为四川大学品牌红（SCU crimson, #C8102E）；
//   - 浅色/深色由 ThemeManager 驱动（同时同步到 QStyleHints，让原生控件跟随）。
// 说明：为兼容既有页面，保留 primary* 命名，并把它们映射到品牌强调色。
QtObject {
    // ---- Spacing & metrics (doc §10) ----
    readonly property int pagePadding: 20
    readonly property int sectionGap: 16
    readonly property int cardPadding: 16
    readonly property int controlHeight: 34
    readonly property int cardRadius: 8       // Fluent 卡片/浮层圆角
    readonly property int smallRadius: 5       // Fluent 控件圆角
    readonly property int navWidth: 200
    readonly property int topBarHeight: 56

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

    // ---- Surfaces & lines (Fluent Mica / Card / Layer) ----
    readonly property color background: themeManager.dark ? "#202020" : "#f3f3f3"   // 窗口 Mica 底
    readonly property color surface: themeManager.dark ? "#2b2b2b" : "#ffffff"      // 卡片 Card
    readonly property color surfaceMuted: themeManager.dark ? "#303030" : "#f6f6f6" // 次级/表头
    readonly property color navBackground: themeManager.dark ? "#1c1c1c" : "#ebebeb" // 导航 Pane
    readonly property color control: themeManager.dark ? "#2d2d2d" : "#fbfbfb"      // 输入/占位填充
    readonly property color controlPressed: themeManager.dark ? "#373737" : "#ededed"
    readonly property color subtleHover: themeManager.dark ? "#2e2e2e" : "#e9e9e9"  // 列表项悬浮
    readonly property color subtleActive: themeManager.dark ? "#343434" : "#e2e2e2" // 列表项选中
    readonly property color border: themeManager.dark ? "#383838" : "#e5e5e5"       // 描边 Stroke
    readonly property color shadow: themeManager.dark ? "#66000000" : "#26000000"   // 卡片投影 (Fluent elevation)

    // ---- Text ----
    readonly property color text: themeManager.dark ? "#ffffff" : "#1a1a1a"
    readonly property color mutedText: themeManager.dark ? "#c8c8c8" : "#5e5e5e"
    readonly property color placeholder: themeManager.dark ? "#9a9a9a" : "#8a8a8a"

    // ---- Accent（SCU crimson，映射到 primary* 以兼容既有页面）----
    readonly property color accent: "#c8102e"
    readonly property color accentText: "#ffffff"     // 强调填充上的文字
    readonly property color primary: accent
    readonly property color primaryPressed: "#a50d26"
    readonly property color primarySoft: themeManager.dark ? "#3a171c" : "#fbe7ea"

    // ---- Status ----
    readonly property color danger: themeManager.dark ? "#e06c5b" : "#c42b1c"
    readonly property color dangerSoft: themeManager.dark ? "#3a1a16" : "#fdf1f0"
    readonly property color dangerBorder: themeManager.dark ? "#5c2b22" : "#f1c9c4"
    readonly property color success: themeManager.dark ? "#4cc26a" : "#0f7b0f"
}

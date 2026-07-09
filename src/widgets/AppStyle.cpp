#include "widgets/AppStyle.h"

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QPalette>

// 应用样式或脱敏规则到目标内容。
void AppStyle::apply(QApplication *app)
{
    if (!app) {
        return;
    }

    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPointSize(10);
    app->setFont(font);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#eaf2f8")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#102033")));
    palette.setColor(QPalette::Base, QColor(255, 255, 255, 220));
    palette.setColor(QPalette::AlternateBase, QColor(244, 248, 252, 180));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#102033")));
    palette.setColor(QPalette::Button, QColor(255, 255, 255, 205));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#102033")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#2f6fdd")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
    app->setPalette(palette);

    app->setStyleSheet(QStringLiteral(R"(
        QMainWindow {
            background: #eaf2f8;
        }

        QWidget#PageSurface {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #edf7fb,
                                        stop:0.46 #e9f1f8,
                                        stop:1 #f7fbff);
        }

        QWidget#Toolbar,
        QFrame#Card,
        QLabel#SummaryCard,
        QWidget#HeaderGlass,
        QWidget#StatePanel {
            background: rgba(255, 255, 255, 172);
            border: 1px solid rgba(255, 255, 255, 218);
            border-top-color: rgba(255, 255, 255, 245);
            border-left-color: rgba(255, 255, 255, 235);
            border-radius: 14px;
        }

        QWidget#HeaderGlass {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 rgba(255,255,255,205),
                                        stop:0.58 rgba(240,248,255,168),
                                        stop:1 rgba(229,242,255,150));
        }

        QLabel#SummaryCard {
            line-height: 150%;
            color: #102033;
        }

        QLabel#PageTitle {
            color: #102033;
            font-size: 22px;
            font-weight: 700;
            padding: 0;
        }

        QLabel#PageSubtitle {
            color: #607083;
            font-size: 12px;
            padding: 1px 0 0 1px;
        }

        QLabel#SignalStrip {
            background: rgba(47, 111, 221, 210);
            color: #ffffff;
            border-radius: 10px;
            padding: 6px 12px;
            font-weight: 700;
            letter-spacing: 0px;
        }

        QLabel#SectionTitle {
            color: #102033;
            font-size: 15px;
            font-weight: 700;
        }

        QLabel#MutedText,
        QLabel#LastUpdated,
        QLabel#CountBadge {
            color: #607083;
        }

        QLabel#CountBadge {
            background: rgba(47, 111, 221, 28);
            color: #2257b8;
            border: 1px solid rgba(47, 111, 221, 55);
            border-radius: 10px;
            padding: 6px 12px;
            font-weight: 600;
        }

        QPushButton {
            background: rgba(255, 255, 255, 176);
            color: #183047;
            border: 1px solid rgba(148, 163, 184, 120);
            border-top-color: rgba(255, 255, 255, 230);
            border-radius: 10px;
            padding: 8px 16px;
            min-height: 22px;
        }

        QPushButton:hover {
            background: rgba(255, 255, 255, 228);
            border-color: rgba(47, 111, 221, 120);
        }

        QPushButton:pressed {
            background: rgba(219, 234, 254, 215);
        }

        QPushButton:disabled {
            background: rgba(241, 245, 249, 130);
            color: #9aa8b6;
            border-color: rgba(203, 213, 225, 100);
        }

        QPushButton#PrimaryButton {
            background: rgba(47, 111, 221, 225);
            color: #ffffff;
            border-color: rgba(47, 111, 221, 235);
            border-top-color: rgba(147, 197, 253, 210);
            font-weight: 600;
        }

        QPushButton#PrimaryButton:hover {
            background: rgba(37, 99, 235, 238);
            border-color: rgba(37, 99, 235, 245);
        }

        QLineEdit,
        QComboBox {
            background: rgba(255, 255, 255, 190);
            border: 1px solid rgba(148, 163, 184, 120);
            border-top-color: rgba(255, 255, 255, 230);
            border-radius: 10px;
            padding: 8px 12px;
            min-height: 24px;
            selection-background-color: #2f6fdd;
        }

        QLineEdit:focus,
        QComboBox:focus {
            border-color: rgba(47, 111, 221, 180);
            background: rgba(255, 255, 255, 235);
        }

        QTabWidget::pane {
            border: 0;
            top: -1px;
        }

        QTabBar::tab {
            background: transparent;
            color: #64748b;
            padding: 10px 20px;
            margin-right: 6px;
            border-bottom: 3px solid transparent;
            font-weight: 600;
        }

        QTabBar::tab:selected {
            color: #2257b8;
            border-bottom-color: #2f6fdd;
        }

        QTabBar::tab:hover {
            color: #2f6fdd;
        }

        QListWidget,
        QTreeWidget,
        QScrollArea {
            background: transparent;
            border: 0;
        }

        QListWidget::item {
            border: 0;
            padding: 0;
            margin: 0;
        }

        QHeaderView::section {
            background: rgba(255, 255, 255, 165);
            color: #536579;
            border: 0;
            border-bottom: 1px solid rgba(203, 213, 225, 120);
            padding: 9px;
            font-weight: 700;
        }

        QTreeWidget {
            background: rgba(255, 255, 255, 162);
            border: 1px solid rgba(255, 255, 255, 205);
            border-top-color: rgba(255, 255, 255, 235);
            border-radius: 12px;
            alternate-background-color: rgba(244, 248, 252, 120);
            outline: 0;
        }

        QTreeWidget::item {
            padding: 7px;
            border-bottom: 1px solid rgba(226, 232, 240, 110);
        }

        QTreeWidget::item:selected {
            background: rgba(191, 219, 254, 165);
            color: #102033;
        }

        QProgressBar {
            border: 0;
            border-radius: 5px;
            background: rgba(203, 213, 225, 120);
            height: 8px;
        }

        QProgressBar::chunk {
            background: #2f6fdd;
            border-radius: 5px;
        }
    )"));
}

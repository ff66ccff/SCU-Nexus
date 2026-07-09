#include "ThemeManager.h"

#include <QGuiApplication>
#include <QSettings>
#include <QStyleHints>

// 构造对象并初始化依赖关系。
ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    // Restore the persisted appearance choice (defaults to "system").
    QSettings settings;
    const QString stored = settings.value(QStringLiteral("appearance/mode"),
                                           QStringLiteral("system")).toString();
    m_mode = (stored == "light" || stored == "dark") ? stored : QStringLiteral("system");

    // Track the OS colour scheme so "跟随系统" reacts to system changes at runtime.
    if (auto* hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged,
                this, [this](Qt::ColorScheme) { updateDark(); });
    }

    // Push the restored choice into the FluentWinUI3 style before the first frame.
    applyColorScheme();
    m_dark = resolveDark();
}

// 返回当前主题模式设置。
QString ThemeManager::mode() const
{
    return m_mode;
}

// 返回当前是否处于深色主题。
bool ThemeManager::dark() const
{
    return m_dark;
}

// 根据主题模式和系统偏好计算深色状态。
bool ThemeManager::resolveDark() const
{
    if (m_mode == "dark") {
        return true;
    }
    if (m_mode == "light") {
        return false;
    }
    // "system": read the OS colour scheme (Qt 6.5+).
    if (auto* hints = QGuiApplication::styleHints()) {
        return hints->colorScheme() == Qt::ColorScheme::Dark;
    }
    return false;
}

// 重新计算深色状态并在变化时通知界面。
void ThemeManager::updateDark()
{
    const bool next = resolveDark();
    if (next == m_dark) {
        return;
    }
    m_dark = next;
    emit darkChanged();
}

// 把当前模式同步给 Qt 的全局色彩方案，使 FluentWinUI3 控件跟随浅色/深色切换。
// system → Unknown（交回操作系统决定）；light/dark → 强制对应方案。
void ThemeManager::applyColorScheme()
{
    auto* hints = QGuiApplication::styleHints();
    if (!hints) {
        return;
    }
    if (m_mode == "dark") {
        hints->setColorScheme(Qt::ColorScheme::Dark);
    } else if (m_mode == "light") {
        hints->setColorScheme(Qt::ColorScheme::Light);
    } else {
        hints->setColorScheme(Qt::ColorScheme::Unknown);
    }
}

// 设置属性值并在变化时发出通知。
void ThemeManager::setMode(const QString& mode)
{
    const QString normalized = (mode == "light" || mode == "dark") ? mode : QStringLiteral("system");
    if (m_mode == normalized) {
        return;
    }
    m_mode = normalized;

    QSettings settings;
    settings.setValue(QStringLiteral("appearance/mode"), m_mode);

    applyColorScheme();
    emit modeChanged();
    updateDark();
}

#include "ThemeManager.h"

#include <QGuiApplication>
#include <QSettings>
#include <QStyleHints>

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

    m_dark = resolveDark();
}

QString ThemeManager::mode() const
{
    return m_mode;
}

bool ThemeManager::dark() const
{
    return m_dark;
}

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

void ThemeManager::updateDark()
{
    const bool next = resolveDark();
    if (next == m_dark) {
        return;
    }
    m_dark = next;
    emit darkChanged();
}

void ThemeManager::setMode(const QString& mode)
{
    const QString normalized = (mode == "light" || mode == "dark") ? mode : QStringLiteral("system");
    if (m_mode == normalized) {
        return;
    }
    m_mode = normalized;

    QSettings settings;
    settings.setValue(QStringLiteral("appearance/mode"), m_mode);

    emit modeChanged();
    updateDark();
}

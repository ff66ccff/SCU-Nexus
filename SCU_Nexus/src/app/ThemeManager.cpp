#include "ThemeManager.h"

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{

}

QString ThemeManager::mode() const
{
    return m_mode;
}

bool ThemeManager::dark() const
{
    return m_mode == "dark";
}

void ThemeManager::setMode(const QString& mode)
{
    const QString normalized = (mode == "light" || mode == "dark") ? mode : QStringLiteral("system");
    if (m_mode == normalized) {
        return;
    }
    m_mode = normalized;
    emit modeChanged();
}

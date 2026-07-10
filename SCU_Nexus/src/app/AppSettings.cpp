#include "AppSettings.h"

#include <QGuiApplication>
#include <QScreen>
#include <QSettings>

namespace {
constexpr int DefaultWindowWidth = 1100;
constexpr int DefaultWindowHeight = 760;
constexpr int MinimumWindowWidth = 900;
constexpr int MinimumWindowHeight = 620;
}

// 构造对象并初始化依赖关系。
AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{

}

QVariantMap AppSettings::restoreWindowGeometry() const
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    const QRect available = screen
        ? screen->availableGeometry()
        : QRect(0, 0, DefaultWindowWidth, DefaultWindowHeight);

    const QSettings settings;
    const QRect geometry = sanitizedGeometry(
        settings.value(QStringLiteral("window/geometry")).toRect(), available);

    return {
        {QStringLiteral("x"), geometry.x()},
        {QStringLiteral("y"), geometry.y()},
        {QStringLiteral("width"), geometry.width()},
        {QStringLiteral("height"), geometry.height()},
    };
}

void AppSettings::saveWindowGeometry(int x, int y, int width, int height)
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), QRect(x, y, width, height));
}

QRect AppSettings::sanitizedGeometry(const QRect &saved, const QRect &available)
{
    const QRect safeAvailable = available.isValid()
        ? available
        : QRect(0, 0, DefaultWindowWidth, DefaultWindowHeight);

    if (!saved.isValid()) {
        const QSize defaultSize(qMin(DefaultWindowWidth, safeAvailable.width()),
                                qMin(DefaultWindowHeight, safeAvailable.height()));
        QRect geometry(QPoint(), defaultSize);
        geometry.moveCenter(safeAvailable.center());
        return geometry;
    }

    const QSize savedSize(qMin(qMax(saved.width(), MinimumWindowWidth),
                               safeAvailable.width()),
                          qMin(qMax(saved.height(), MinimumWindowHeight),
                               safeAvailable.height()));
    QRect geometry(saved.topLeft(), savedSize);

    if (geometry.left() < safeAvailable.left())
        geometry.moveLeft(safeAvailable.left());
    if (geometry.right() > safeAvailable.right())
        geometry.moveRight(safeAvailable.right());
    if (geometry.top() < safeAvailable.top())
        geometry.moveTop(safeAvailable.top());
    if (geometry.bottom() > safeAvailable.bottom())
        geometry.moveBottom(safeAvailable.bottom());

    return geometry;
}

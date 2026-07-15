#include "AppSettings.h"

#include <QGuiApplication>
#include <QScreen>
#include <QSettings>

namespace {
constexpr int DefaultWindowWidth = 1100;
constexpr int DefaultWindowHeight = 760;
constexpr int MinimumWindowWidth = 900;
constexpr int MinimumWindowHeight = 620;
const QString QwenApiKeySetting = QStringLiteral("ai/qwen_api_key");
}

// 构造对象并初始化依赖关系。
AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_qwenApiKey(QSettings().value(QwenApiKeySetting).toString().trimmed())
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

QString AppSettings::qwenApiKey() const
{
    return m_qwenApiKey;
}

bool AppSettings::hasQwenApiKey() const
{
    return !m_qwenApiKey.isEmpty();
}

bool AppSettings::saveQwenApiKey(const QString &apiKey)
{
    const QString normalizedApiKey = apiKey.trimmed();
    if (normalizedApiKey == m_qwenApiKey)
        return true;

    QSettings settings;
    if (normalizedApiKey.isEmpty())
        settings.remove(QwenApiKeySetting);
    else
        settings.setValue(QwenApiKeySetting, normalizedApiKey);
    settings.sync();

    if (settings.status() != QSettings::NoError)
        return false;

    m_qwenApiKey = normalizedApiKey;
    emit qwenApiKeyChanged();
    return true;
}

bool AppSettings::clearQwenApiKey()
{
    return saveQwenApiKey({});
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

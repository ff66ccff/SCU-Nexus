#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QRect>
#include <QVariantMap>
#include <functional>

class AppFoundationTests;

class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString qwenApiKey READ qwenApiKey NOTIFY qwenApiKeyChanged)
    Q_PROPERTY(bool hasQwenApiKey READ hasQwenApiKey NOTIFY qwenApiKeyChanged)
public:
    explicit AppSettings(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap restoreWindowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height);

    QString qwenApiKey() const;
    bool hasQwenApiKey() const;
    Q_INVOKABLE bool saveQwenApiKey(const QString &apiKey);
    Q_INVOKABLE bool clearQwenApiKey();

    static QRect sanitizedGeometry(const QRect &saved, const QRect &available);

signals:
    void qwenApiKeyChanged();

private:
    AppSettings(QObject *parent, std::function<void()> syncFailureHook);

    friend class AppFoundationTests;

    QString m_qwenApiKey;
    std::function<void()> m_syncFailureHook;
};

#endif // APPSETTINGS_H

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QRect>
#include <QVariantMap>

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
    QString m_qwenApiKey;
};

#endif // APPSETTINGS_H

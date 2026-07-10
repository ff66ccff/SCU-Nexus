#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QRect>
#include <QVariantMap>

class AppSettings : public QObject
{
    Q_OBJECT
public:
    explicit AppSettings(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap restoreWindowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height);

    static QRect sanitizedGeometry(const QRect &saved, const QRect &available);
};

#endif // APPSETTINGS_H

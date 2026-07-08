#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY modeChanged)
public:
    explicit ThemeManager(QObject *parent = nullptr);

    QString mode() const;
    bool dark() const;

    Q_INVOKABLE void setMode(const QString& mode);

signals:
    void modeChanged();

private:
    QString m_mode = "system";
};

#endif // THEMEMANAGER_H

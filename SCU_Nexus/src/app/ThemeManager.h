#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY darkChanged)
public:
    explicit ThemeManager(QObject *parent = nullptr);

    QString mode() const;
    bool dark() const;

    Q_INVOKABLE void setMode(const QString& mode);

signals:
    void modeChanged();
    void darkChanged();

private:
    bool resolveDark() const;
    void updateDark();
    void applyColorScheme();

    QString m_mode = "system";
    bool m_dark = false;
};

#endif // THEMEMANAGER_H

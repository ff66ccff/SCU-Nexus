#ifndef TOASTMANAGER_H
#define TOASTMANAGER_H

#include <QObject>

class ToastManager : public QObject
{
    Q_OBJECT
public:
    explicit ToastManager(QObject *parent = nullptr);
    Q_INVOKABLE void show(const QString& message, const QString& level = QStringLiteral("info"));

signals:
    void toastRequested(QString message, QString level);
};

#endif // TOASTMANAGER_H

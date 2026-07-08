#ifndef TOASTMANAGER_H
#define TOASTMANAGER_H

#include <QObject>

class ToastManager : public QObject
{
    Q_OBJECT
public:
    explicit ToastManager(QObject *parent = nullptr);
    Q_INVOKABLE void showMsg(QString str);
};

#endif // TOASTMANAGER_H

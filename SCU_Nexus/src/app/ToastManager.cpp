#include "ToastManager.h"
#include <QDebug>

ToastManager::ToastManager(QObject *parent)
    : QObject(parent)
{

}

void ToastManager::showMsg(QString str)
{
    show(str);
}

void ToastManager::show(const QString& message, const QString& level)
{
    qDebug() << "toast" << level << message;
    emit toastRequested(message, level);
}

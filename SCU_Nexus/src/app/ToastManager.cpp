#include "ToastManager.h"
#include <QDebug>

ToastManager::ToastManager(QObject *parent)
    : QObject(parent)
{

}

void ToastManager::showMsg(QString str)
{
    qDebug() << str;
}

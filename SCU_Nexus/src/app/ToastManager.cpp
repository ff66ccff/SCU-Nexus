#include "ToastManager.h"
#include <QDebug>

// 构造对象并初始化依赖关系。
ToastManager::ToastManager(QObject *parent)
    : QObject(parent)
{

}

// 发送提示消息供界面层展示。
void ToastManager::showMsg(QString str)
{
    show(str);
}

// 发送提示消息供界面层展示。
void ToastManager::show(const QString& message, const QString& level)
{
    qDebug() << "toast" << level << message;
    emit toastRequested(message, level);
}

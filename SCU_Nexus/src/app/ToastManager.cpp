#include "ToastManager.h"

// 构造对象并初始化依赖关系。
ToastManager::ToastManager(QObject *parent)
    : QObject(parent)
{

}

// 发送提示消息供界面层展示。
void ToastManager::show(const QString& message, const QString& level)
{
    emit toastRequested(message, level);
}

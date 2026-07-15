#ifndef AUTHERRORS_H
#define AUTHERRORS_H

// 认证层与网络/API 层共享同一套 ApiError，避免把 C++ 异常对象暴露给 QML。
#include "src/core/network/NetworkError.h"

#endif // AUTHERRORS_H

#ifndef NETWORKERROR_H
#define NETWORKERROR_H

#include <QString>

// 跨网络、认证、教务 API 和 ViewModel 传递的稳定错误分类。
// QML 只应展示 ApiError::message；debugBody 可能含服务端内容，只能进入脱敏日志。
enum class ApiErrorType {
    Network,
    Timeout,
    ParseFailed,
    Unauthenticated,
    SessionExpired,
    ServiceUnavailable,
    RateLimited,
    InvalidCaptcha,
    InvalidCredential,
    ParameterInvalid,
    ConflictDetected,
    DataWriteFailed,
    EmptyResult,
    // 当前回调约定使用默认构造的 Unknown 表示“没有错误”。
    Unknown
};

struct ApiError
{
    ApiErrorType type = ApiErrorType::Unknown;
    QString message;
    int statusCode = 0;
    QString debugBody; // 诊断信息，不得直接暴露给 QML。
};

#endif // NETWORKERROR_H

#ifndef NETWORKERROR_H
#define NETWORKERROR_H

#include <QString>

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
    Unknown
};

struct ApiError
{
    ApiErrorType type = ApiErrorType::Unknown;
    QString message;
    int statusCode = 0;
    QString debugBody;
};

#endif // NETWORKERROR_H

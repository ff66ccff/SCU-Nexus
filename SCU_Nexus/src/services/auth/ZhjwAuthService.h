#ifndef ZHJWAUTHSERVICE_H
#define ZHJWAUTHSERVICE_H

#include "src/core/network/NetworkError.h"

#include <QObject>
#include <QString>
#include <functional>
#include <vector>

class CookieHttpClient;
class ScuAuthService;

class ZhjwAuthService : public QObject
{
    Q_OBJECT
public:
    using ClientCallback = std::function<void(CookieHttpClient* client, const ApiError& error)>;

    explicit ZhjwAuthService(QObject* parent = nullptr, ScuAuthService* scuAuthService = nullptr);
    ~ZhjwAuthService() override = default;

    virtual void getClient(ClientCallback callback);

public slots:
    virtual void invalidate();

private:
    void finishLogin(CookieHttpClient* client, const ApiError& error);

    ScuAuthService* m_scuAuthService = nullptr;
    bool m_ownsScuAuthService = false;
    CookieHttpClient* m_cachedClient = nullptr;
    QString m_boundAccessToken;
    bool m_loginInProgress = false;
    std::vector<ClientCallback> m_pendingCallbacks;
};

#endif // ZHJWAUTHSERVICE_H

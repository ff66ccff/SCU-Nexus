#ifndef AUTHVIEWMODEL_H
#define AUTHVIEWMODEL_H

#include <QObject>
#include <QUrl>

class ScuAuthService;

// 登录页与 ScuAuthService 之间的 QML 边界。
// ViewModel 只维护可展示状态，并把验证码字节落到缓存文件；QML 收集账号密码后
// 立即转交服务层，不保存 token/Cookie，也不参与 SM2 加密和会话管理。
class AuthViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool captchaLoading READ captchaLoading NOTIFY captchaLoadingChanged)
    Q_PROPERTY(QUrl captchaImageUrl READ captchaImageUrl NOTIFY captchaChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    explicit AuthViewModel(QObject* parent = nullptr);
    explicit AuthViewModel(ScuAuthService* authService, QObject* parent = nullptr);

    bool loggedIn() const;
    bool loading() const;
    bool captchaLoading() const;
    QUrl captchaImageUrl() const;
    QString errorMessage() const;

    Q_INVOKABLE void fetchCaptcha();
    Q_INVOKABLE void login(const QString& username, const QString& password, const QString& captchaText);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void clearError();

signals:
    void loggedInChanged();
    void loadingChanged();
    void captchaLoadingChanged();
    void captchaChanged();
    void errorChanged();
    void loginSucceeded();
    void loginFailed(const QString& message);
    void sessionExpired(const QString& message);

private:
    void initializeAuthService();
    void setLoggedIn(bool loggedIn);
    void setLoading(bool loading);
    void setCaptchaLoading(bool captchaLoading);
    void setCaptchaImageUrl(const QUrl& url);
    void setErrorMessage(const QString& message);
    QUrl writeCaptchaImage(const QByteArray& imageBytes, const QString& mimeType);

    ScuAuthService* m_authService = nullptr;
    bool m_loggedIn = false;
    bool m_loading = false;
    bool m_captchaLoading = false;
    QUrl m_captchaImageUrl;
    // 后端返回的 captcha code 必须和当前图片、用户输入一起提交，刷新图片时立即清空。
    QString m_captchaCode;
    QString m_errorMessage;
    int m_captchaImageSequence = 0;
};

#endif // AUTHVIEWMODEL_H

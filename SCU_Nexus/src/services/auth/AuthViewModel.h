#ifndef AUTHVIEWMODEL_H
#define AUTHVIEWMODEL_H

#include <QObject>
#include <QUrl>

class ScuAuthService;

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
    QString m_captchaCode;
    QString m_errorMessage;
    int m_captchaImageSequence = 0;
};

#endif // AUTHVIEWMODEL_H

#pragma once

#include <QJsonObject>
#include <QObject>

class MockZhjwApiService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn WRITE setLoggedIn NOTIFY loggedInChanged)

public:
    explicit MockZhjwApiService(QObject *parent = nullptr);

    bool loggedIn() const;
    void setLoggedIn(bool loggedIn);

    QList<QJsonObject> fetchExamPlan();
    QJsonObject fetchSchemeScores();
    QJsonObject fetchPassingScores();

signals:
    void loggedInChanged();

private:
    bool m_loggedIn = false;
};

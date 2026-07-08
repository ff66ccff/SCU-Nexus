#pragma once

#include "common/QueryState.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QStackedLayout;

class QueryStateWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QueryStateWidget(QWidget *parent = nullptr);

    void setState(QueryState state, const QString &errorMessage = QString());

signals:
    void retryRequested();

private:
    QStackedLayout *m_stack = nullptr;
    QLabel *m_message = nullptr;
    QPushButton *m_retryButton = nullptr;
};

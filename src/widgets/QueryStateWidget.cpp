#include "widgets/QueryStateWidget.h"

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>

QueryStateWidget::QueryStateWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("StatePanel"));
    setMinimumHeight(140);
    m_stack = new QStackedLayout(this);

    auto *empty = new QWidget(this);
    m_stack->addWidget(empty);

    auto *loadingPage = new QWidget(this);
    auto *loadingLayout = new QVBoxLayout(loadingPage);
    auto *loadingText = new QLabel(tr("正在加载"), loadingPage);
    loadingText->setObjectName(QStringLiteral("SectionTitle"));
    auto *loadingBar = new QProgressBar(loadingPage);
    loadingBar->setRange(0, 0);
    loadingLayout->addStretch();
    loadingLayout->addWidget(loadingText, 0, Qt::AlignHCenter);
    loadingLayout->addWidget(loadingBar);
    loadingLayout->addStretch();
    m_stack->addWidget(loadingPage);

    auto *messagePage = new QWidget(this);
    auto *messageLayout = new QVBoxLayout(messagePage);
    m_message = new QLabel(messagePage);
    m_message->setObjectName(QStringLiteral("SectionTitle"));
    m_message->setAlignment(Qt::AlignCenter);
    m_message->setWordWrap(true);
    m_retryButton = new QPushButton(tr("重试"), messagePage);
    m_retryButton->setObjectName(QStringLiteral("PrimaryButton"));
    connect(m_retryButton, &QPushButton::clicked, this, &QueryStateWidget::retryRequested);
    messageLayout->addStretch();
    messageLayout->addWidget(m_message);
    messageLayout->addWidget(m_retryButton, 0, Qt::AlignHCenter);
    messageLayout->addStretch();
    m_stack->addWidget(messagePage);
}

void QueryStateWidget::setState(QueryState state, const QString &errorMessage)
{
    if (state == QueryState::Idle) {
        m_message->setText(tr("点击刷新获取数据"));
        m_retryButton->setText(tr("刷新"));
        m_retryButton->show();
        m_stack->setCurrentIndex(2);
        return;
    }

    if (state == QueryState::Loading) {
        m_stack->setCurrentIndex(1);
        return;
    }

    if (state == QueryState::Empty || state == QueryState::Error || state == QueryState::LoginRequired) {
        if (state == QueryState::Empty) {
            m_message->setText(tr("暂无数据"));
            m_retryButton->hide();
        } else if (state == QueryState::LoginRequired) {
            m_message->setText(tr("请先登录后再查询"));
            m_retryButton->setText(tr("重试"));
            m_retryButton->show();
        } else {
            m_message->setText(errorMessage.isEmpty() ? tr("加载失败") : errorMessage);
            m_retryButton->setText(tr("重试"));
            m_retryButton->show();
        }
        m_stack->setCurrentIndex(2);
        return;
    }

    m_stack->setCurrentIndex(0);
}

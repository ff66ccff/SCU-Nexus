#include "widgets/ExamPlanWidget.h"

#include "viewmodels/ExamPlanViewModel.h"
#include "widgets/QueryStateWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>

// 读取指定资源并返回结果。
ExamPlanWidget::ExamPlanWidget(ExamPlanViewModel *viewModel, QWidget *parent)
    : QWidget(parent),
      m_viewModel(viewModel)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 8, 0, 0);
    root->setSpacing(12);
    auto *toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(16, 12, 16, 12);
    toolbar->setSpacing(12);
    auto *toolbarWidget = new QWidget(this);
    toolbarWidget->setObjectName(QStringLiteral("Toolbar"));
    toolbarWidget->setLayout(toolbar);
    m_refreshButton = new QPushButton(tr("刷新"), this);
    m_refreshButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_lastUpdatedLabel = new QLabel(tr("尚未更新"), this);
    m_lastUpdatedLabel->setObjectName(QStringLiteral("LastUpdated"));
    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName(QStringLiteral("CountBadge"));
    auto *section = new QLabel(tr("考试安排"), toolbarWidget);
    section->setObjectName(QStringLiteral("SectionTitle"));
    toolbar->addWidget(section);
    toolbar->addWidget(m_refreshButton);
    toolbar->addWidget(m_lastUpdatedLabel);
    toolbar->addStretch();
    toolbar->addWidget(m_countLabel);
    root->addWidget(toolbarWidget);

    m_stateWidget = new QueryStateWidget(this);
    root->addWidget(m_stateWidget);

    m_list = new QListWidget(this);
    m_list->setSpacing(10);
    root->addWidget(m_list, 1);

    connect(m_refreshButton, &QPushButton::clicked, m_viewModel, &ExamPlanViewModel::refresh);
    connect(m_stateWidget, &QueryStateWidget::retryRequested, m_viewModel, &ExamPlanViewModel::load);
    connect(m_viewModel, &ExamPlanViewModel::stateChanged, this, &ExamPlanWidget::syncFromViewModel);
    connect(m_viewModel, &ExamPlanViewModel::examsChanged, this, &ExamPlanWidget::syncFromViewModel);
    connect(m_viewModel, &ExamPlanViewModel::lastUpdatedChanged, this, &ExamPlanWidget::syncFromViewModel);
    syncFromViewModel();
}

// 根据视图模型同步界面展示。
void ExamPlanWidget::syncFromViewModel()
{
    m_refreshButton->setEnabled(!m_viewModel->loading());
    m_lastUpdatedLabel->setText(m_viewModel->lastUpdated().isValid()
                                    ? tr("更新于 %1").arg(m_viewModel->lastUpdated().toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")))
                                    : tr("尚未更新"));
    m_countLabel->setText(tr("共 %1 场").arg(m_viewModel->count()));
    m_stateWidget->setState(m_viewModel->state(), m_viewModel->errorMessage());
    m_list->setVisible(m_viewModel->state() == QueryState::Loaded);

    m_list->clear();
    for (const QVariant &value : m_viewModel->exams()) {
        const QVariantMap exam = value.toMap();
        auto *item = new QListWidgetItem();
        auto *card = new QFrame(m_list);
        card->setObjectName(QStringLiteral("Card"));
        card->setFrameShape(QFrame::NoFrame);
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(6);
        auto *title = new QLabel(exam.value(QStringLiteral("courseName")).toString(), card);
        title->setObjectName(QStringLiteral("SectionTitle"));
        QFont titleFont = title->font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 1);
        title->setFont(titleFont);
        layout->addWidget(title);
        auto *time = new QLabel(tr("%1  %2  %3  %4")
                                    .arg(exam.value(QStringLiteral("week")).toString(),
                                         exam.value(QStringLiteral("date")).toString(),
                                         exam.value(QStringLiteral("weekday")).toString(),
                                         exam.value(QStringLiteral("timeRange")).toString()), card);
        time->setObjectName(QStringLiteral("MutedText"));
        layout->addWidget(time);
        layout->addWidget(new QLabel(tr("地点：%1").arg(exam.value(QStringLiteral("location")).toString()), card));
        layout->addWidget(new QLabel(tr("座位号：%1").arg(exam.value(QStringLiteral("seatNumber")).toString()), card));
        const QString ticket = exam.value(QStringLiteral("ticketNumber")).toString();
        if (!ticket.isEmpty()) {
            layout->addWidget(new QLabel(tr("准考证号：%1").arg(ticket), card));
        }
        const QString tip = exam.value(QStringLiteral("tip")).toString();
        if (!tip.isEmpty() && tip != QStringLiteral("无")) {
            layout->addWidget(new QLabel(tr("提示：%1").arg(tip), card));
        }
        if (exam.value(QStringLiteral("isPast")).toBool()) {
            card->setStyleSheet(QStringLiteral("QFrame#Card { background: rgba(241, 245, 249, 150); border: 1px solid rgba(255, 255, 255, 190); border-radius: 12px; color: #8a9aac; }"));
        }
        item->setSizeHint(card->sizeHint());
        m_list->addItem(item);
        m_list->setItemWidget(item, card);
    }
}

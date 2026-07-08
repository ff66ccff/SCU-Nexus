#pragma once

#include <QWidget>

class ExamPlanViewModel;
class QLabel;
class QListWidget;
class QPushButton;
class QueryStateWidget;

class ExamPlanWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExamPlanWidget(ExamPlanViewModel *viewModel, QWidget *parent = nullptr);

private slots:
    void syncFromViewModel();

private:
    ExamPlanViewModel *m_viewModel = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QLabel *m_lastUpdatedLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QueryStateWidget *m_stateWidget = nullptr;
    QListWidget *m_list = nullptr;
};

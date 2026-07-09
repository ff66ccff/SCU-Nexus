#include "widgets/MainWindow.h"

#include "viewmodels/AcademicCalendarViewModel.h"
#include "viewmodels/ExamPlanViewModel.h"
#include "viewmodels/GradesViewModel.h"
#include "widgets/AcademicCalendarWidget.h"
#include "widgets/CampusBackdrop.h"
#include "widgets/ExamPlanWidget.h"
#include "widgets/GradesWidget.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QWidget>

MainWindow::MainWindow(AcademicCalendarViewModel *calendarViewModel,
                       ExamPlanViewModel *examPlanViewModel,
                       GradesViewModel *gradesViewModel,
                       QWidget *parent)
    : QMainWindow(parent),
      m_calendarViewModel(calendarViewModel),
      m_examPlanViewModel(examPlanViewModel),
      m_gradesViewModel(gradesViewModel)
{
    auto *central = new CampusBackdrop(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(26, 20, 26, 24);
    layout->setSpacing(14);

    auto *header = new QWidget(central);
    header->setObjectName(QStringLiteral("HeaderGlass"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 12, 18, 12);
    headerLayout->setSpacing(16);
    header->setMaximumHeight(82);

    auto *copyLayout = new QVBoxLayout();
    copyLayout->setContentsMargins(0, 0, 0, 0);
    copyLayout->setSpacing(3);

    auto *title = new QLabel(tr("SCU Nexus 查询中心"), header);
    title->setObjectName(QStringLiteral("PageTitle"));
    auto *subtitle = new QLabel(tr("校历、考表、成绩集中查询，保留缓存，随时刷新"), header);
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    copyLayout->addWidget(title);
    copyLayout->addWidget(subtitle);
    headerLayout->addLayout(copyLayout, 1);

    auto *signal = new QLabel(tr("教务数据台"), header);
    signal->setObjectName(QStringLiteral("SignalStrip"));
    headerLayout->addWidget(signal, 0, Qt::AlignVCenter);
    layout->addWidget(header);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(new AcademicCalendarWidget(m_calendarViewModel, tabs), tr("校历"));
    tabs->addTab(new ExamPlanWidget(m_examPlanViewModel, tabs), tr("考表"));
    tabs->addTab(new GradesWidget(m_gradesViewModel, tabs), tr("成绩"));
    layout->addWidget(tabs, 1);
    setCentralWidget(central);
    setWindowTitle(tr("SCU Nexus - 人员 D 查询模块"));
    resize(1320, 820);
    setMinimumSize(1040, 680);
}

// 加载当前模块数据并同步界面状态。
void MainWindow::loadAll()
{
    m_calendarViewModel->load();
    m_examPlanViewModel->load();
    m_gradesViewModel->load();
}

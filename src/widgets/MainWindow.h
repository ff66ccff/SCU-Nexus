#pragma once

#include <QMainWindow>

class AcademicCalendarViewModel;
class ExamPlanViewModel;
class GradesViewModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(AcademicCalendarViewModel *calendarViewModel,
               ExamPlanViewModel *examPlanViewModel,
               GradesViewModel *gradesViewModel,
               QWidget *parent = nullptr);

    void loadAll();

private:
    AcademicCalendarViewModel *m_calendarViewModel = nullptr;
    ExamPlanViewModel *m_examPlanViewModel = nullptr;
    GradesViewModel *m_gradesViewModel = nullptr;
};

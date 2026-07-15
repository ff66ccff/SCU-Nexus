#pragma once

#include "src/services/api/ApiDtos.h"

#include <QString>
#include <QTime>
#include <QVector>

enum class ClassroomPeriodStatus {
    Free,
    InClass,
    Exam,
    Experiment,
    Borrowed
};

ClassroomPeriodStatus classroomStatusForModule(const QString& moduleId);
QString classroomStatusKey(ClassroomPeriodStatus status);
QString classroomStatusText(ClassroomPeriodStatus status);
QVector<ClassroomPeriodStatus> classroomPeriodStatuses(
    const ClassroomQueryResultDto& result,
    const QString& classroomNumber);
int currentClassroomPeriod(const QString& campusName, const QTime& now);

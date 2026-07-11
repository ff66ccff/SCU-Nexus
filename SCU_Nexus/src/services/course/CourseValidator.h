#ifndef COURSEVALIDATOR_H
#define COURSEVALIDATOR_H

#include <QList>
#include <QString>

#include "../../models/Course.h"
#include "../../models/ScheduleConfig.h"

namespace SCUNexus {

struct CourseValidationResult {
    bool valid = false;
    QString message;
    QString conflictCourseId;
};

class CourseValidator final
{
public:
    static CourseValidationResult validate(const Course &course,
                                           const ScheduleConfig &config,
                                           const QList<Course> &existing = {},
                                           const QString &excludeId = {});
};

} // namespace SCUNexus

#endif // COURSEVALIDATOR_H

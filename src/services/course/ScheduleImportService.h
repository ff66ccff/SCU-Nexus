#ifndef SCHEDULEIMPORTSERVICE_H
#define SCHEDULEIMPORTSERVICE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QDate>
#include "../../models/Course.h"
#include "../../models/ScheduleConfig.h"

namespace SCUNexus {

// Handles the logic for importing schedules from the teaching affairs system.
// This service orchestrates parsing, validation, naming conflict resolution,
// and current-week synchronization.
class ScheduleImportService : public QObject {
    Q_OBJECT
public:
    explicit ScheduleImportService(QObject* parent = nullptr);

    // Import result types
    enum class ConflictStrategy {
        Cancel,
        AddSuffix,
        UpdateExisting
    };

    struct ImportValidationResult {
        bool valid = false;
        QStringList errors;
        QList<Course> validatedCourses;
    };

    // Validate imported courses against a config
    static ImportValidationResult validateCourses(
        const QList<Course>& courses,
        const ScheduleConfig& config);

    // Check for naming conflicts between new courses' schedules and existing schedules
    struct NameConflict {
        QString newSemesterName;
        QString existingScheduleId;
        QString existingScheduleName;
    };
    static QList<NameConflict> findNameConflicts(
        const QString& newSemesterName,
        const QList<ScheduleConfig>& existingSchedules);

    // Create a ScheduleConfig for an imported semester
    static ScheduleConfig createImportConfig(const QString& semesterName, QDate semesterStartDate);

    // Calculate a corrected semester start date from the current teaching week
    static QDate calculateStartDateFromCurrentWeek(int currentTeachingWeek, QDate today = QDate::currentDate());

private:
    // Convert a date to the Sunday of its week (matching Flutter toSunday())
    static QDate toSunday(QDate date);
};

} // namespace SCUNexus

#endif // SCHEDULEIMPORTSERVICE_H

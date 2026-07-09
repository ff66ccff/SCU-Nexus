#ifndef SCHEDULEREPOSITORY_H
#define SCHEDULEREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QString>
#include "../models/Course.h"
#include "../models/ScheduleConfig.h"

namespace SCUNexus {

class ScheduleRepository : public QObject {
    Q_OBJECT
public:
    explicit ScheduleRepository(QObject* parent = nullptr);
    ~ScheduleRepository();

    // Allow injecting a custom database path (for testing)
    void setDatabasePath(const QString& path);
    QString databasePath() const;

    // Initialize database: create tables, load metadata
    bool init();

    // Schedule-level operations
    QString currentScheduleId() const;
    QList<ScheduleConfig> allSchedules() const;
    ScheduleConfig currentScheduleConfig() const;

    bool switchSchedule(const QString& scheduleId);
    bool addSchedule(const ScheduleConfig& config);
    bool deleteSchedule(const QString& scheduleId);
    bool saveScheduleConfig(const ScheduleConfig& config);

    // Course-level operations (operate on current schedule)
    QList<Course> currentCourses() const;
    QList<Course> coursesForSchedule(const QString& scheduleId) const;
    bool addCourse(const Course& course);
    bool updateCourse(const Course& course);
    bool deleteCourse(const QString& courseId);
    bool replaceScheduleCourses(const QString& scheduleId, const QList<Course>& courses);
    bool hasConflict(const Course& course, const QString& excludeId = {}) const;

    // Debug / admin
    bool clearAllCourseData();

signals:
    void dataChanged();

private:
    bool createTables();
    bool loadMetadata();
    bool saveMetadata(const QString& key, const QString& value);

    bool loadSchedulesCache();
    bool loadCoursesCache();

    // Internal helpers
    Course courseFromQuery(const QSqlQuery& query) const;

    QString m_dbPath;
    QString m_currentScheduleId;
    QList<ScheduleConfig> m_schedulesCache;
    QList<Course> m_coursesCache;

    bool m_initialized = false;
};

} // namespace SCUNexus

#endif // SCHEDULEREPOSITORY_H

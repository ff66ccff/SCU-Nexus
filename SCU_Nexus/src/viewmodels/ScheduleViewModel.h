#ifndef SCHEDULEVIEWMODEL_H
#define SCHEDULEVIEWMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QVariantMap>
#include <QVariantList>
#include <QList>
#include "../models/Course.h"
#include "../models/ScheduleConfig.h"
#include "../repositories/ScheduleRepository.h"
#include "../services/course/CourseLayoutService.h"

namespace SCUNexus {

// QAbstractListModel for exposing courses to QML
class CourseListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum CourseRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TeacherRole,
        LocationRole,
        StartWeekRole,
        EndWeekRole,
        DayOfWeekRole,
        StartSectionRole,
        EndSectionRole,
        ColorValueRole,
        WeekTypeRole,
        ActiveRole,
        ConflictRole,
        TrackRole,
        TotalTracksRole,
        CourseIdAliasRole,
        CourseNameAliasRole
    };

    explicit CourseListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setCourses(const QList<CourseLayoutService::LayoutCourse>& courses);
    void clear();

private:
    QList<CourseLayoutService::LayoutCourse> m_courses;
};

// Main ViewModel for the schedule page
class ScheduleViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasSchedule READ hasSchedule NOTIFY schedulesChanged)
    Q_PROPERTY(int currentWeek READ currentWeek NOTIFY currentWeekChanged)
    Q_PROPERTY(int totalWeeks READ totalWeeks NOTIFY configChanged)
    Q_PROPERTY(int sectionsPerDay READ sectionsPerDay NOTIFY configChanged)
    Q_PROPERTY(QString currentScheduleName READ currentScheduleName NOTIFY configChanged)
    Q_PROPERTY(QVariantList timeSlots READ timeSlots NOTIFY configChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QObject* courseListModel READ courseListModel NOTIFY coursesChanged)
    Q_PROPERTY(bool showNonCurrentWeekCourses READ showNonCurrentWeekCourses WRITE setShowNonCurrentWeekCourses NOTIFY displayOptionsChanged)

public:
    explicit ScheduleViewModel(QObject* parent = nullptr);

    void setRepository(ScheduleRepository* repo);

    // Initialization
    Q_INVOKABLE void load();

    // Schedule switching
    Q_INVOKABLE void switchSchedule(const QString& scheduleId);
    Q_INVOKABLE QVariantList schedules() const;
    Q_INVOKABLE bool createSchedule(const QString& name);
    Q_INVOKABLE bool renameSchedule(const QString& scheduleId, const QString& name);
    Q_INVOKABLE bool deleteSchedule(const QString& scheduleId);
    Q_INVOKABLE QVariantMap currentConfig() const;
    Q_INVOKABLE bool saveCurrentConfig(const QVariantMap& data);

    // Week navigation
    Q_INVOKABLE void updateCurrentWeek(int week);
    Q_INVOKABLE void goPreviousWeek();
    Q_INVOKABLE void goNextWeek();
    Q_INVOKABLE void goToCurrentWeek();

    // Course CRUD via QML
    Q_INVOKABLE void addCourse(const QVariantMap& data);
    Q_INVOKABLE void updateCourse(const QString& id, const QVariantMap& data);
    Q_INVOKABLE void deleteCourse(const QString& id);
    Q_INVOKABLE bool clearAllCourseData();
    Q_INVOKABLE QVariantMap courseById(const QString& id) const;

    // Property accessors
    bool loading() const;
    bool hasSchedule() const;
    int currentWeek() const;
    int totalWeeks() const;
    int sectionsPerDay() const;
    QString currentScheduleName() const;
    QVariantList timeSlots() const;
    QString errorMessage() const;
    QObject* courseListModel();
    bool showNonCurrentWeekCourses() const;
    void setShowNonCurrentWeekCourses(bool v);

signals:
    void loadingChanged();
    void schedulesChanged();
    void currentWeekChanged();
    void configChanged();
    void errorChanged();
    void coursesChanged();
    void displayOptionsChanged();
    void dataReset();

private:
    void setError(const QString& message);
    void refreshCourses();
    void loadCurrentWeek();

    ScheduleRepository* m_repo = nullptr;
    CourseListModel* m_courseListModel = nullptr;
    bool m_loading = false;
    int m_displayWeek = 1;
    bool m_showNonCurrentWeekCourses = false;
    QString m_errorMessage;
};

} // namespace SCUNexus

#endif // SCHEDULEVIEWMODEL_H

#ifndef SCHEDULEIMPORTVIEWMODEL_H
#define SCHEDULEIMPORTVIEWMODEL_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <functional>
#include "../models/Course.h"
#include "../models/ScheduleConfig.h"
#include "../repositories/ScheduleRepository.h"
#include "../services/course/JwxtScheduleParser.h"
#include "../services/course/ScheduleImportService.h"

namespace SCUNexus {

class ScheduleImportViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool hasConflict READ hasConflict NOTIFY conflictChanged)
    Q_PROPERTY(QString conflictMessage READ conflictMessage NOTIFY conflictChanged)
    Q_PROPERTY(bool importComplete READ importComplete NOTIFY importCompleteChanged)
    Q_PROPERTY(QVariantList semesters READ availableSemesters NOTIFY semestersChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loginStateChanged)
    Q_PROPERTY(bool loginRequired READ loginRequired NOTIFY loginStateChanged)

public:
    using SemestersResult = std::function<void(QVariantList semesters, QString error)>;
    using ScheduleResult = std::function<void(QJsonObject schedule, QString error)>;
    using WeekResult = std::function<void(int week, QString error)>;
    using FetchSemesters = std::function<void(SemestersResult done)>;
    using FetchSchedule = std::function<void(const QString& planCode, ScheduleResult done)>;
    using FetchWeek = std::function<void(WeekResult done)>;

    explicit ScheduleImportViewModel(QObject* parent = nullptr);

    void setRepository(ScheduleRepository* repo);
    void setRemoteApi(FetchSemesters fetchSemesters,
                      FetchSchedule fetchSchedule,
                      FetchWeek fetchWeek);
    void setLoggedIn(bool loggedIn);

    Q_INVOKABLE bool loadSemesters();
    Q_INVOKABLE QVariantList availableSemesters() const;

    // Import schedule for a specific semester plan code
    Q_INVOKABLE bool importSchedule(const QString& planCode, const QString& semesterLabel);

    // Import directly from raw JSON (the actual import method)
    Q_INVOKABLE bool importFromJson(const QString& planCode, const QString& semesterLabel,
                                     const QJsonObject& rawJson);

    // Resolve name conflict
    Q_INVOKABLE void resolveConflict(const QString& strategy); // "cancel", "addSuffix", "updateExisting"

    // Sync current teaching week through B's injected API.
    Q_INVOKABLE bool syncCurrentWeek();

    // Property accessors
    bool loading() const;
    QString errorMessage() const;
    QString statusMessage() const;
    bool hasConflict() const;
    QString conflictMessage() const;
    bool importComplete() const;
    bool loggedIn() const;
    bool loginRequired() const;

signals:
    void loadingChanged();
    void errorChanged();
    void statusChanged();
    void conflictChanged();
    void importCompleteChanged();
    void semestersChanged();
    void loginStateChanged();
    void importFinished(const QString& scheduleId);
    void currentWeekSynced(int week);

private:
    bool ensureLoggedIn();
    quint64 beginAction();
    void clearActionPresentationState();
    void clearLoggedOutState();
    void setLoading(bool loading);
    bool applyCurrentWeek(int currentWeek);
    void setError(const QString& message);

    ScheduleRepository* m_repo = nullptr;
    FetchSemesters m_fetchSemesters;
    FetchSchedule m_fetchSchedule;
    FetchWeek m_fetchWeek;
    QVariantList m_semesters;
    bool m_loading = false;
    QString m_errorMessage;
    QString m_statusMessage;
    bool m_hasConflict = false;
    QString m_conflictMessage;
    bool m_importComplete = false;
    bool m_loggedIn = false;
    quint64 m_loginGeneration = 0;
    quint64 m_actionGeneration = 0;

    // Pending import data for conflict resolution
    QList<Course> m_pendingCourses;
    QString m_pendingSemesterName;
};

} // namespace SCUNexus

#endif // SCHEDULEIMPORTVIEWMODEL_H

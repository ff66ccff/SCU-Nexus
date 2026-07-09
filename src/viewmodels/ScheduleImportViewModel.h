#ifndef SCHEDULEIMPORTVIEWMODEL_H
#define SCHEDULEIMPORTVIEWMODEL_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QList>
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

public:
    explicit ScheduleImportViewModel(QObject* parent = nullptr);

    void setRepository(ScheduleRepository* repo);

    // Load available semesters (from B's API - stub for now)
    Q_INVOKABLE QVariantList availableSemesters() const;

    // Import schedule for a specific semester plan code
    Q_INVOKABLE bool importSchedule(const QString& planCode, const QString& semesterLabel);

    // Import directly from raw JSON (the actual import method)
    Q_INVOKABLE bool importFromJson(const QString& planCode, const QString& semesterLabel,
                                     const QJsonObject& rawJson);

    // Resolve name conflict
    Q_INVOKABLE void resolveConflict(const QString& strategy); // "cancel", "addSuffix", "updateExisting"

    // Sync current teaching week (calls B's fetchCurrentWeek)
    Q_INVOKABLE bool syncCurrentWeek(int currentWeek);

    // Property accessors
    bool loading() const;
    QString errorMessage() const;
    QString statusMessage() const;
    bool hasConflict() const;
    QString conflictMessage() const;
    bool importComplete() const;

signals:
    void loadingChanged();
    void errorChanged();
    void statusChanged();
    void conflictChanged();
    void importCompleteChanged();
    void importFinished(const QString& scheduleId);

private:
    ScheduleRepository* m_repo = nullptr;
    bool m_loading = false;
    QString m_errorMessage;
    QString m_statusMessage;
    bool m_hasConflict = false;
    QString m_conflictMessage;
    bool m_importComplete = false;

    // Pending import data for conflict resolution
    QList<Course> m_pendingCourses;
    QString m_pendingSemesterName;
};

} // namespace SCUNexus

#endif // SCHEDULEIMPORTVIEWMODEL_H

#include "ScheduleImportViewModel.h"
#include <QUuid>
#include <QDebug>

namespace SCUNexus {

ScheduleImportViewModel::ScheduleImportViewModel(QObject* parent)
    : QObject(parent)
{
}

void ScheduleImportViewModel::setRepository(ScheduleRepository* repo) {
    m_repo = repo;
}

QVariantList ScheduleImportViewModel::availableSemesters() const {
    // Stub: in production, this would call B's fetchSemesters()
    // Returns empty list - B's API will be integrated later
    return {};
}

bool ScheduleImportViewModel::importSchedule(const QString& planCode, const QString& semesterLabel) {
    if (!m_repo) {
        m_errorMessage = QStringLiteral("数据仓库未初始化");
        emit errorChanged();
        return false;
    }

    m_loading = true;
    m_importComplete = false;
    m_errorMessage.clear();
    emit loadingChanged();
    emit errorChanged();

    // Stub: in production, this calls B's fetchJwxtSchedule(planCode)
    // For now, the actual API call is external; this method expects
    // the raw JSON to be passed in via a separate setter or the caller
    // handles the API. Here we provide the conflict resolution framework.

    // The real implementation path:
    // 1. Call fetchJwxtSchedule(planCode) -> raw JSON
    // 2. JwxtScheduleParser::parse(raw) -> courses
    // 3. ScheduleImportService::validateCourses(courses, config)
    // 4. Check name conflicts
    // 5. If conflicts, store pending and signal; else write to repo

    m_loading = false;
    emit loadingChanged();
    return true;
}

// This is the method called when the caller has the raw JSON
bool ScheduleImportViewModel::importFromJson(const QString& planCode, const QString& semesterLabel,
                                              const QJsonObject& rawJson) {
    if (!m_repo) {
        m_errorMessage = QStringLiteral("数据仓库未初始化");
        emit errorChanged();
        return false;
    }

    m_loading = true;
    m_importComplete = false;
    m_errorMessage.clear();
    emit loadingChanged();

    // Parse
    auto parseResult = JwxtScheduleParser::parse(rawJson);
    if (!parseResult.success) {
        m_errorMessage = parseResult.errorMessage;
        m_loading = false;
        emit errorChanged();
        emit loadingChanged();
        return false;
    }

    // Clean label
    QString cleanedLabel = JwxtScheduleParser::cleanSemesterLabel(semesterLabel);

    // Create config
    QDate semesterStart = QDate::currentDate();
    // Find the Monday of current week
    int dow = semesterStart.dayOfWeek();
    semesterStart = semesterStart.addDays(-(dow - 1));

    ScheduleConfig config = ScheduleImportService::createImportConfig(cleanedLabel, semesterStart);

    // Validate courses
    auto validation = ScheduleImportService::validateCourses(parseResult.courses, config);
    if (!validation.valid) {
        m_errorMessage = validation.errors.join(QStringLiteral("\n"));
        m_loading = false;
        emit errorChanged();
        emit loadingChanged();
        return false;
    }

    // Check name conflicts
    auto conflicts = ScheduleImportService::findNameConflicts(cleanedLabel, m_repo->allSchedules());
    if (!conflicts.isEmpty()) {
        m_pendingCourses = validation.validatedCourses;
        m_pendingSemesterName = cleanedLabel;
        m_hasConflict = true;
        m_conflictMessage = QStringLiteral("已存在同名课表 \"%1\"，请选择处理方式：").arg(cleanedLabel);
        m_loading = false;
        emit conflictChanged();
        emit loadingChanged();
        return true; // Not an error - waiting for user decision
    }

    // No conflicts: add schedule and courses
    if (!m_repo->addSchedule(config)) {
        m_errorMessage = QStringLiteral("创建课表失败");
        m_loading = false;
        emit errorChanged();
        emit loadingChanged();
        return false;
    }

    if (!m_repo->replaceScheduleCourses(config.id, validation.validatedCourses)) {
        m_errorMessage = QStringLiteral("导入课程失败");
        m_loading = false;
        emit errorChanged();
        emit loadingChanged();
        return false;
    }

    m_repo->switchSchedule(config.id);

    m_loading = false;
    m_importComplete = true;
    m_statusMessage = QStringLiteral("成功导入 %1 门课程").arg(validation.validatedCourses.size());
    emit loadingChanged();
    emit statusChanged();
    emit importCompleteChanged();
    emit importFinished(config.id);
    return true;
}

void ScheduleImportViewModel::resolveConflict(const QString& strategy) {
    if (!m_hasConflict || !m_repo) return;

    if (strategy == QStringLiteral("cancel")) {
        m_hasConflict = false;
        m_pendingCourses.clear();
        m_pendingSemesterName.clear();
        emit conflictChanged();
        return;
    }

    if (strategy == QStringLiteral("updateExisting")) {
        // Find the existing schedule and update its courses
        auto conflicts = ScheduleImportService::findNameConflicts(
            m_pendingSemesterName, m_repo->allSchedules());
        if (conflicts.isEmpty()) return;

        QString existingId = conflicts.first().existingScheduleId;
        // Generate new IDs for courses
        QList<Course> newCourses;
        for (auto course : m_pendingCourses) {
            course.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            newCourses.append(course);
        }

        if (m_repo->replaceScheduleCourses(existingId, newCourses)) {
            m_repo->switchSchedule(existingId);
            m_importComplete = true;
            m_statusMessage = QStringLiteral("已更新课表，共 %1 门课程").arg(newCourses.size());
            emit importCompleteChanged();
            emit statusChanged();
            emit importFinished(existingId);
        } else {
            m_errorMessage = QStringLiteral("更新课表失败");
            emit errorChanged();
        }
    } else if (strategy == QStringLiteral("addSuffix")) {
        // Add "_导入" suffix
        QString newName = m_pendingSemesterName + QStringLiteral("_导入");
        QDate startDate = QDate::currentDate();
        int dow = startDate.dayOfWeek();
        startDate = startDate.addDays(-(dow - 1));

        ScheduleConfig config = ScheduleImportService::createImportConfig(newName, startDate);

        if (m_repo->addSchedule(config)) {
            if (m_repo->replaceScheduleCourses(config.id, m_pendingCourses)) {
                m_repo->switchSchedule(config.id);
                m_importComplete = true;
                m_statusMessage = QStringLiteral("已导入为新课程表 \"%1\"").arg(newName);
                emit importCompleteChanged();
                emit statusChanged();
                emit importFinished(config.id);
            } else {
                m_errorMessage = QStringLiteral("导入课程失败");
                emit errorChanged();
            }
        } else {
            m_errorMessage = QStringLiteral("创建课表失败");
            emit errorChanged();
        }
    }

    m_hasConflict = false;
    m_pendingCourses.clear();
    m_pendingSemesterName.clear();
    emit conflictChanged();
}

bool ScheduleImportViewModel::syncCurrentWeek(int currentWeek) {
    if (!m_repo) return false;

    ScheduleConfig config = m_repo->currentScheduleConfig();
    QDate newStart = ScheduleImportService::calculateStartDateFromCurrentWeek(currentWeek);
    config.semesterStartDate = newStart;

    return m_repo->saveScheduleConfig(config);
}

// Property accessors
bool ScheduleImportViewModel::loading() const { return m_loading; }
QString ScheduleImportViewModel::errorMessage() const { return m_errorMessage; }
QString ScheduleImportViewModel::statusMessage() const { return m_statusMessage; }
bool ScheduleImportViewModel::hasConflict() const { return m_hasConflict; }
QString ScheduleImportViewModel::conflictMessage() const { return m_conflictMessage; }
bool ScheduleImportViewModel::importComplete() const { return m_importComplete; }

} // namespace SCUNexus

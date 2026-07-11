#include "ScheduleImportViewModel.h"
#include <QDebug>
#include <QPointer>
#include <algorithm>

namespace SCUNexus {

ScheduleImportViewModel::ScheduleImportViewModel(QObject* parent)
    : QObject(parent)
{
}

void ScheduleImportViewModel::setRepository(ScheduleRepository* repo) {
    m_repo = repo;
}

void ScheduleImportViewModel::setRemoteApi(FetchSemesters fetchSemesters,
                                           FetchSchedule fetchSchedule,
                                           FetchWeek fetchWeek) {
    m_fetchSemesters = std::move(fetchSemesters);
    m_fetchSchedule = std::move(fetchSchedule);
    m_fetchWeek = std::move(fetchWeek);
}

void ScheduleImportViewModel::setLoggedIn(bool loggedIn) {
    const bool changed = m_loggedIn != loggedIn;
    if (changed) {
        m_loggedIn = loggedIn;
        ++m_loginGeneration;
    }

    if (!loggedIn) {
        clearLoggedOutState();
    }

    if (changed) {
        emit loginStateChanged();
    }
}

bool ScheduleImportViewModel::loadSemesters() {
    if (!ensureLoggedIn()) {
        return false;
    }

    const quint64 actionGeneration = beginAction();
    if (!m_fetchSemesters) {
        setLoading(false);
        setError(QStringLiteral("教务系统接口未初始化"));
        return false;
    }

    setLoading(true);

    QPointer<ScheduleImportViewModel> self(this);
    const quint64 loginGeneration = m_loginGeneration;
    m_fetchSemesters([self, loginGeneration, actionGeneration](QVariantList semesters, QString error) {
        if (!self || !self->m_loggedIn
            || self->m_loginGeneration != loginGeneration
            || self->m_actionGeneration != actionGeneration) {
            return;
        }
        self->setLoading(false);
        if (!error.isEmpty()) {
            self->setError(error);
        } else {
            self->m_semesters = std::move(semesters);
            self->setError({});
            emit self->semestersChanged();
        }
    });
    return true;
}

QVariantList ScheduleImportViewModel::availableSemesters() const {
    return m_semesters;
}

bool ScheduleImportViewModel::importSchedule(const QString& planCode, const QString& semesterLabel) {
    if (!ensureLoggedIn()) {
        return false;
    }

    const quint64 actionGeneration = beginAction();
    if (!m_repo) {
        setLoading(false);
        setError(QStringLiteral("数据仓库未初始化"));
        return false;
    }

    if (!m_fetchSchedule) {
        setLoading(false);
        setError(QStringLiteral("教务系统接口未初始化"));
        return false;
    }

    setLoading(true);
    QPointer<ScheduleImportViewModel> self(this);
    const quint64 loginGeneration = m_loginGeneration;
    m_fetchSchedule(planCode, [self, loginGeneration, actionGeneration, planCode, semesterLabel](QJsonObject rawJson, QString error) {
        if (!self || !self->m_loggedIn
            || self->m_loginGeneration != loginGeneration
            || self->m_actionGeneration != actionGeneration) {
            return;
        }
        if (!error.isEmpty()) {
            self->setLoading(false);
            self->setError(error);
            return;
        }
        self->importFromJson(planCode, semesterLabel, rawJson);
    });
    return true;
}

// This is the method called when the caller has the raw JSON
bool ScheduleImportViewModel::importFromJson(const QString& planCode, const QString& semesterLabel,
                                              const QJsonObject& rawJson) {
    Q_UNUSED(planCode)
    beginAction();
    if (!m_repo) {
        setLoading(false);
        setError(QStringLiteral("数据仓库未初始化"));
        return false;
    }

    setLoading(true);

    // Parse
    auto parseResult = JwxtScheduleParser::parse(rawJson);
    if (!parseResult.success) {
        setError(parseResult.errorMessage);
        setLoading(false);
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
        setError(QStringLiteral("导入数据格式异常：\n")
                 + validation.errors.join(QStringLiteral("\n")));
        setLoading(false);
        return false;
    }

    // Check name conflicts
    auto conflicts = ScheduleImportService::findNameConflicts(cleanedLabel, m_repo->allSchedules());
    if (!conflicts.isEmpty()) {
        m_pendingCourses = validation.validatedCourses;
        m_pendingSemesterName = cleanedLabel;
        m_hasConflict = true;
        m_conflictMessage = QStringLiteral("已存在同名课表 \"%1\"，请选择处理方式：").arg(cleanedLabel);
        setLoading(false);
        emit conflictChanged();
        return true; // Not an error - waiting for user decision
    }

    // Persist the schedule, courses, and current-schedule switch as one unit.
    if (!m_repo->addScheduleWithCoursesAndSwitch(config, validation.validatedCourses)) {
        setError(QStringLiteral("导入课程失败"));
        setLoading(false);
        return false;
    }

    setLoading(false);
    m_importComplete = true;
    m_statusMessage = QStringLiteral("成功导入 %1 门课程").arg(validation.validatedCourses.size());
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
        if (m_repo->replaceScheduleCoursesAndSwitch(existingId, m_pendingCourses)) {
            m_importComplete = true;
            m_statusMessage = QStringLiteral("已更新课表，共 %1 门课程").arg(m_pendingCourses.size());
            emit importCompleteChanged();
            emit statusChanged();
            emit importFinished(existingId);
        } else {
            setError(QStringLiteral("更新课表失败"));
        }
    } else if (strategy == QStringLiteral("addSuffix")) {
        const QString baseName = m_pendingSemesterName + QStringLiteral("_导入");
        QString newName = baseName;
        int suffix = 2;
        const QList<ScheduleConfig> schedules = m_repo->allSchedules();
        auto nameExists = [&](const QString& candidate) {
            return std::any_of(schedules.cbegin(), schedules.cend(),
                               [&](const ScheduleConfig& config) {
                                   return config.semesterName == candidate;
                               });
        };
        while (nameExists(newName)) {
            newName = QStringLiteral("%1 (%2)").arg(baseName).arg(suffix++);
        }
        QDate startDate = QDate::currentDate();
        int dow = startDate.dayOfWeek();
        startDate = startDate.addDays(-(dow - 1));

        ScheduleConfig config = ScheduleImportService::createImportConfig(newName, startDate);

        if (m_repo->addScheduleWithCoursesAndSwitch(config, m_pendingCourses)) {
            m_importComplete = true;
            m_statusMessage = QStringLiteral("已导入为新课程表 \"%1\"").arg(newName);
            emit importCompleteChanged();
            emit statusChanged();
            emit importFinished(config.id);
        } else {
            setError(QStringLiteral("导入课程失败"));
        }
    }

    m_hasConflict = false;
    m_pendingCourses.clear();
    m_pendingSemesterName.clear();
    emit conflictChanged();
}

bool ScheduleImportViewModel::syncCurrentWeek() {
    if (!ensureLoggedIn()) {
        return false;
    }

    const quint64 actionGeneration = beginAction();
    if (!m_repo || m_repo->currentScheduleId().isEmpty()) {
        setLoading(false);
        setError(QStringLiteral("当前没有可同步的课表"));
        return false;
    }
    if (!m_fetchWeek) {
        setLoading(false);
        setError(QStringLiteral("教务系统接口未初始化"));
        return false;
    }

    setLoading(true);

    QPointer<ScheduleImportViewModel> self(this);
    const quint64 loginGeneration = m_loginGeneration;
    m_fetchWeek([self, loginGeneration, actionGeneration](int week, QString error) {
        if (!self || !self->m_loggedIn
            || self->m_loginGeneration != loginGeneration
            || self->m_actionGeneration != actionGeneration) {
            return;
        }
        self->setLoading(false);
        if (!error.isEmpty() || week < 1) {
            self->setError(error.isEmpty() ? QStringLiteral("当前教学周无效") : error);
        } else if (self->applyCurrentWeek(week)) {
            emit self->currentWeekSynced(week);
            self->m_statusMessage = QStringLiteral("已同步到第 %1 教学周").arg(week);
            emit self->statusChanged();
        }
    });
    return true;
}

bool ScheduleImportViewModel::ensureLoggedIn() {
    if (m_loggedIn) {
        return true;
    }

    clearLoggedOutState();
    setError(QStringLiteral("请先登录后导入教务课表"));
    return false;
}

quint64 ScheduleImportViewModel::beginAction() {
    ++m_actionGeneration;
    clearActionPresentationState();
    return m_actionGeneration;
}

void ScheduleImportViewModel::clearActionPresentationState() {
    setError({});
    if (!m_statusMessage.isEmpty()) {
        m_statusMessage.clear();
        emit statusChanged();
    }
    if (m_importComplete) {
        m_importComplete = false;
        emit importCompleteChanged();
    }
}

void ScheduleImportViewModel::clearLoggedOutState() {
    setLoading(false);
    if (!m_semesters.isEmpty()) {
        m_semesters.clear();
        emit semestersChanged();
    }

    m_pendingCourses.clear();
    m_pendingSemesterName.clear();
    if (m_hasConflict || !m_conflictMessage.isEmpty()) {
        m_hasConflict = false;
        m_conflictMessage.clear();
        emit conflictChanged();
    }

    clearActionPresentationState();
}

void ScheduleImportViewModel::setLoading(bool loading) {
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

bool ScheduleImportViewModel::applyCurrentWeek(int currentWeek) {
    if (!m_repo || currentWeek < 1) return false;

    ScheduleConfig config = m_repo->currentScheduleConfig();
    QDate newStart = ScheduleImportService::calculateStartDateFromCurrentWeek(currentWeek);
    config.semesterStartDate = newStart;

    if (!m_repo->saveScheduleConfig(config)) {
        setError(QStringLiteral("保存当前教学周失败"));
        return false;
    }
    return true;
}

void ScheduleImportViewModel::setError(const QString& message) {
    if (m_errorMessage == message) return;
    m_errorMessage = message;
    emit errorChanged();
}

// Property accessors
bool ScheduleImportViewModel::loading() const { return m_loading; }
QString ScheduleImportViewModel::errorMessage() const { return m_errorMessage; }
QString ScheduleImportViewModel::statusMessage() const { return m_statusMessage; }
bool ScheduleImportViewModel::hasConflict() const { return m_hasConflict; }
QString ScheduleImportViewModel::conflictMessage() const { return m_conflictMessage; }
bool ScheduleImportViewModel::importComplete() const { return m_importComplete; }
bool ScheduleImportViewModel::loggedIn() const { return m_loggedIn; }
bool ScheduleImportViewModel::loginRequired() const { return !m_loggedIn; }

} // namespace SCUNexus

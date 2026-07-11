#include "ScheduleViewModel.h"
#include "../services/course/CourseValidator.h"
#include <QUuid>
#include <QDebug>
#include <QDate>
#include <algorithm>

namespace SCUNexus {

// ===================== CourseListModel =====================

CourseListModel::CourseListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int CourseListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_courses.size();
}

QVariant CourseListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_courses.size())
        return {};

    const auto& lc = m_courses[index.row()];
    const Course& c = lc.course;

    switch (role) {
    case IdRole:
    case CourseIdAliasRole: return c.id;
    case NameRole:
    case CourseNameAliasRole: return c.name;
    case TeacherRole: return c.teacher;
    case LocationRole: return c.location;
    case StartWeekRole: return c.startWeek;
    case EndWeekRole: return c.endWeek;
    case DayOfWeekRole: return c.dayOfWeek;
    case StartSectionRole: return c.startSection;
    case EndSectionRole: return c.endSection;
    case ColorValueRole: return c.colorValue;
    case WeekTypeRole: return weekTypeToInt(c.weekType);
    case ActiveRole: return lc.active;
    case ConflictRole: return lc.conflict;
    case TrackRole: return lc.track;
    case TotalTracksRole: return lc.totalTracks;
    default: return {};
    }
}

QHash<int, QByteArray> CourseListModel::roleNames() const {
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {TeacherRole, "teacher"},
        {LocationRole, "location"},
        {StartWeekRole, "startWeek"},
        {EndWeekRole, "endWeek"},
        {DayOfWeekRole, "dayOfWeek"},
        {StartSectionRole, "startSection"},
        {EndSectionRole, "endSection"},
        {ColorValueRole, "colorValue"},
        {WeekTypeRole, "weekType"},
        {ActiveRole, "active"},
        {ConflictRole, "conflict"},
        {TrackRole, "track"},
        {TotalTracksRole, "totalTracks"},
        {CourseIdAliasRole, "courseId"},
        {CourseNameAliasRole, "courseName"},
    };
}

void CourseListModel::setCourses(const QList<CourseLayoutService::LayoutCourse>& courses) {
    beginResetModel();
    m_courses = courses;
    endResetModel();
}

void CourseListModel::clear() {
    beginResetModel();
    m_courses.clear();
    endResetModel();
}

// ===================== ScheduleViewModel =====================

ScheduleViewModel::ScheduleViewModel(QObject* parent)
    : QObject(parent)
    , m_courseListModel(new CourseListModel(this))
{
}

void ScheduleViewModel::setRepository(ScheduleRepository* repo) {
    m_repo = repo;
}

void ScheduleViewModel::load() {
    if (!m_repo) return;

    m_loading = true;
    emit loadingChanged();

    if (!m_repo->initialized()) {
        m_errorMessage = QStringLiteral("课表数据尚未初始化");
        m_loading = false;
        emit errorChanged();
        emit loadingChanged();
        return;
    }

    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.clear();
        emit errorChanged();
    }

    loadCurrentWeek();
    refreshCourses();

    m_loading = false;
    emit loadingChanged();
    emit schedulesChanged();
    emit configChanged();
    emit currentWeekChanged();
    emit coursesChanged();
}

void ScheduleViewModel::refreshCourses() {
    if (!m_repo || !hasSchedule()) {
        m_courseListModel->clear();
        return;
    }

    QList<Course> allCourses = m_repo->currentCourses();
    // Apply merge same-slot courses
    allCourses = CourseLayoutService::mergeSameSlotCourses(allCourses);

    auto layoutCourses = CourseLayoutService::selectVisibleCoursesForWeek(
        allCourses, m_displayWeek, m_showNonCurrentWeekCourses);

    m_courseListModel->setCourses(layoutCourses);
    emit coursesChanged();
}

void ScheduleViewModel::loadCurrentWeek() {
    if (!m_repo || !hasSchedule()) {
        m_displayWeek = 1;
        return;
    }
    ScheduleConfig config = m_repo->currentScheduleConfig();
    m_displayWeek = config.currentWeek();
}

void ScheduleViewModel::switchSchedule(const QString& scheduleId) {
    if (!m_repo) return;
    if (m_repo->switchSchedule(scheduleId)) {
        loadCurrentWeek();
        refreshCourses();
        emit schedulesChanged();
        emit configChanged();
        emit currentWeekChanged();
    }
}

QVariantList ScheduleViewModel::schedules() const {
    QVariantList result;
    if (!m_repo) return result;

    auto all = m_repo->allSchedules();
    for (const auto& s : all) {
        QVariantMap map;
        map["id"] = s.id;
        map["semesterName"] = s.semesterName;
        map["totalWeeks"] = s.totalWeeks;
        map["isCurrent"] = (s.id == m_repo->currentScheduleId());
        result.append(map);
    }
    return result;
}

bool ScheduleViewModel::createSchedule(const QString& name) {
    if (!m_repo || name.trimmed().isEmpty()) {
        setError(QStringLiteral("课表名称不能为空"));
        return false;
    }

    ScheduleConfig config = ScheduleConfig::createDefault(name.trimmed());
    if (!m_repo->addSchedule(config) || !m_repo->switchSchedule(config.id)) {
        setError(QStringLiteral("创建课表失败"));
        return false;
    }

    setError({});
    loadCurrentWeek();
    refreshCourses();
    emit schedulesChanged();
    emit configChanged();
    emit currentWeekChanged();
    return true;
}

bool ScheduleViewModel::renameSchedule(const QString& scheduleId, const QString& name) {
    if (!m_repo || name.trimmed().isEmpty()) {
        setError(QStringLiteral("课表名称不能为空"));
        return false;
    }

    for (auto config : m_repo->allSchedules()) {
        if (config.id != scheduleId) {
            continue;
        }
        config.semesterName = name.trimmed();
        if (!m_repo->saveScheduleConfig(config)) {
            setError(QStringLiteral("重命名课表失败"));
            return false;
        }
        setError({});
        emit schedulesChanged();
        emit configChanged();
        return true;
    }

    setError(QStringLiteral("课表不存在"));
    return false;
}

bool ScheduleViewModel::deleteSchedule(const QString& scheduleId) {
    if (!m_repo || !m_repo->deleteSchedule(scheduleId)) {
        setError(QStringLiteral("删除课表失败"));
        return false;
    }

    setError({});
    loadCurrentWeek();
    refreshCourses();
    emit schedulesChanged();
    emit configChanged();
    emit currentWeekChanged();
    return true;
}

QVariantMap ScheduleViewModel::currentConfig() const {
    QVariantMap result;
    if (!m_repo || !hasSchedule()) {
        return result;
    }

    const ScheduleConfig config = m_repo->currentScheduleConfig();
    result["id"] = config.id;
    result["semesterName"] = config.semesterName;
    result["semesterStartDate"] = config.semesterStartDate.toString(Qt::ISODate);
    result["totalWeeks"] = config.totalWeeks;
    result["morningSections"] = config.morningSections;
    result["afternoonSections"] = config.afternoonSections;
    result["eveningSections"] = config.eveningSections;
    result["courseDuration"] = config.courseDuration;
    result["breakDuration"] = config.breakDuration;
    result["autoSyncTime"] = config.autoSyncTime;
    result["timeSlots"] = timeSlots();
    if (config.timeSlots == ScheduleConfig::jiangAnTimeSlots()) {
        result["timeSlotPreset"] = QStringLiteral("jiangAn");
    } else if (config.timeSlots == ScheduleConfig::wangJiangHuaXiTimeSlots()) {
        result["timeSlotPreset"] = QStringLiteral("wangJiangHuaXi");
    } else {
        result["timeSlotPreset"] = QStringLiteral("custom");
    }
    return result;
}

bool ScheduleViewModel::saveCurrentConfig(const QVariantMap& data) {
    if (!m_repo || !hasSchedule()) {
        setError(QStringLiteral("当前没有课表"));
        return false;
    }

    ScheduleConfig config = m_repo->currentScheduleConfig();
    if (data.contains("semesterName")) {
        config.semesterName = data.value("semesterName").toString().trimmed();
    }
    if (data.contains("semesterStartDate")) {
        config.semesterStartDate = QDate::fromString(
            data.value("semesterStartDate").toString(), Qt::ISODate);
    }
    if (data.contains("totalWeeks")) config.totalWeeks = data.value("totalWeeks").toInt();
    if (data.contains("morningSections")) config.morningSections = data.value("morningSections").toInt();
    if (data.contains("afternoonSections")) config.afternoonSections = data.value("afternoonSections").toInt();
    if (data.contains("eveningSections")) config.eveningSections = data.value("eveningSections").toInt();
    if (data.contains("courseDuration")) config.courseDuration = data.value("courseDuration").toInt();
    if (data.contains("breakDuration")) config.breakDuration = data.value("breakDuration").toInt();
    if (data.contains("autoSyncTime")) config.autoSyncTime = data.value("autoSyncTime").toBool();

    const QString preset = data.value("timeSlotPreset").toString();
    if (preset == QStringLiteral("jiangAn")) {
        config.timeSlots = ScheduleConfig::jiangAnTimeSlots();
    } else if (preset == QStringLiteral("wangJiangHuaXi")) {
        config.timeSlots = ScheduleConfig::wangJiangHuaXiTimeSlots();
    } else if (preset == QStringLiteral("custom")) {
        QVector<TimeSlot> customSlots;
        const QVariantList values = data.value("timeSlots").toList();
        customSlots.reserve(values.size());
        for (const QVariant& value : values) {
            const QVariantMap item = value.toMap();
            const TimeSlot slot{
                QTime::fromString(item.value("startTime").toString(), QStringLiteral("HH:mm")),
                QTime::fromString(item.value("endTime").toString(), QStringLiteral("HH:mm")),
            };
            if (!slot.isValid()) {
                setError(QStringLiteral("自定义时间表包含无效时间"));
                return false;
            }
            customSlots.append(slot);
        }
        config.timeSlots = customSlots;
    }

    if (config.semesterName.isEmpty() || !config.semesterStartDate.isValid()
        || config.totalWeeks < 1 || config.morningSections < 1
        || config.afternoonSections < 1 || config.eveningSections < 0) {
        setError(QStringLiteral("课表设置无效"));
        return false;
    }

    for (const Course& course : m_repo->currentCourses()) {
        if (course.endWeek > config.totalWeeks
            || course.endSection > config.sectionsPerDay()) {
            setError(QStringLiteral("设置会导致课程 \"%1\" 超出周次或节次范围")
                         .arg(course.name));
            return false;
        }
    }

    if (config.timeSlots.size() != config.sectionsPerDay()) {
        setError(QStringLiteral("时间表节次数量必须与每日节数一致"));
        return false;
    }

    if (!m_repo->saveScheduleConfig(config)) {
        setError(QStringLiteral("保存课表设置失败"));
        return false;
    }

    setError({});
    m_displayWeek = std::clamp(m_displayWeek, 1, config.totalWeeks);
    refreshCourses();
    emit schedulesChanged();
    emit configChanged();
    emit currentWeekChanged();
    return true;
}

void ScheduleViewModel::updateCurrentWeek(int week) {
    if (!m_repo || !hasSchedule()) return;

    ScheduleConfig config = m_repo->currentScheduleConfig();
    m_displayWeek = std::clamp(week, 1, config.totalWeeks);
    refreshCourses();
    emit currentWeekChanged();
}

void ScheduleViewModel::goPreviousWeek() {
    if (!m_repo || !hasSchedule()) return;

    ScheduleConfig config = m_repo->currentScheduleConfig();
    if (m_displayWeek > 1) {
        m_displayWeek--;
    }
    refreshCourses();
    emit currentWeekChanged();
}

void ScheduleViewModel::goNextWeek() {
    if (!m_repo || !hasSchedule()) return;

    ScheduleConfig config = m_repo->currentScheduleConfig();
    if (m_displayWeek < config.totalWeeks) {
        m_displayWeek++;
    }
    refreshCourses();
    emit currentWeekChanged();
}

void ScheduleViewModel::goToCurrentWeek() {
    if (!m_repo || !hasSchedule()) return;
    ScheduleConfig config = m_repo->currentScheduleConfig();
    m_displayWeek = config.currentWeek();
    refreshCourses();
    emit currentWeekChanged();
}

bool ScheduleViewModel::addCourse(const QVariantMap& data) {
    if (!m_repo) {
        setError(QStringLiteral("数据仓库未初始化"));
        return false;
    }
    if (!hasSchedule()) {
        setError(QStringLiteral("当前没有课表"));
        return false;
    }

    const ScheduleConfig config = m_repo->currentScheduleConfig();
    Course course;
    course.id = data.value("id", QUuid::createUuid().toString(QUuid::WithoutBraces)).toString();
    course.name = data.value("name").toString();
    course.teacher = data.value("teacher").toString();
    course.location = data.value("location").toString();
    course.startWeek = data.value("startWeek", 1).toInt();
    course.endWeek = data.value("endWeek", config.totalWeeks).toInt();
    course.dayOfWeek = data.value("dayOfWeek", 1).toInt();
    course.startSection = data.value("startSection", 1).toInt();
    course.endSection = data.value("endSection", 2).toInt();
    course.colorValue = static_cast<quint32>(data.value("colorValue", 0xFF42A5F5).toUInt());
    course.weekType = intToWeekType(data.value("weekType", 0).toInt());

    const CourseValidationResult validation = CourseValidator::validate(
        course, config, m_repo->currentCourses());
    if (!validation.valid) {
        setError(validation.message);
        return false;
    }

    const bool added = m_repo->addCourse(course);
    if (!added) {
        setError(QStringLiteral("保存失败，请重试"));
        return false;
    }

    setError({});
    refreshCourses();
    return added;
}

bool ScheduleViewModel::updateCourse(const QString& id, const QVariantMap& data) {
    if (!m_repo) {
        setError(QStringLiteral("数据仓库未初始化"));
        return false;
    }
    if (!hasSchedule()) {
        setError(QStringLiteral("当前没有课表"));
        return false;
    }

    const QList<Course> courses = m_repo->currentCourses();
    for (Course course : courses) {
        if (course.id == id) {
            if (data.contains("name")) course.name = data["name"].toString();
            if (data.contains("teacher")) course.teacher = data["teacher"].toString();
            if (data.contains("location")) course.location = data["location"].toString();
            if (data.contains("startWeek")) course.startWeek = data["startWeek"].toInt();
            if (data.contains("endWeek")) course.endWeek = data["endWeek"].toInt();
            if (data.contains("dayOfWeek")) course.dayOfWeek = data["dayOfWeek"].toInt();
            if (data.contains("startSection")) course.startSection = data["startSection"].toInt();
            if (data.contains("endSection")) course.endSection = data["endSection"].toInt();
            if (data.contains("colorValue")) course.colorValue = static_cast<quint32>(data["colorValue"].toUInt());
            if (data.contains("weekType")) course.weekType = intToWeekType(data["weekType"].toInt());

            const CourseValidationResult validation = CourseValidator::validate(
                course, m_repo->currentScheduleConfig(), courses, id);
            if (!validation.valid) {
                setError(validation.message);
                return false;
            }

            const bool updated = m_repo->updateCourse(course);
            if (!updated) {
                setError(QStringLiteral("保存失败，请重试"));
                return false;
            }

            setError({});
            refreshCourses();
            return updated;
        }
    }

    setError(QStringLiteral("课程不存在"));
    return false;
}

void ScheduleViewModel::deleteCourse(const QString& id) {
    if (!m_repo) return;
    if (m_repo->deleteCourse(id)) {
        refreshCourses();
    }
}

bool ScheduleViewModel::clearAllCourseData() {
    if (!m_repo) {
        setError(QStringLiteral("数据仓库未初始化"));
        return false;
    }
    if (!m_repo->clearAllCourseData()) {
        setError(QStringLiteral("清除本地课表数据失败"));
        return false;
    }
    setError({});
    m_displayWeek = 1;
    m_courseListModel->clear();
    emit currentWeekChanged();
    emit schedulesChanged();
    emit configChanged();
    emit coursesChanged();
    emit dataReset();
    return true;
}

QVariantMap ScheduleViewModel::courseById(const QString& id) const {
    QVariantMap result;
    if (!m_repo) return result;

    QList<Course> courses = m_repo->currentCourses();
    for (const auto& c : courses) {
        if (c.id == id) {
            result["id"] = c.id;
            result["name"] = c.name;
            result["teacher"] = c.teacher;
            result["location"] = c.location;
            result["startWeek"] = c.startWeek;
            result["endWeek"] = c.endWeek;
            result["dayOfWeek"] = c.dayOfWeek;
            result["startSection"] = c.startSection;
            result["endSection"] = c.endSection;
            result["colorValue"] = c.colorValue;
            result["weekType"] = weekTypeToInt(c.weekType);
            break;
        }
    }
    return result;
}

// Property accessors
bool ScheduleViewModel::loading() const { return m_loading; }
bool ScheduleViewModel::hasSchedule() const {
    return m_repo && !m_repo->currentScheduleId().isEmpty();
}
int ScheduleViewModel::currentWeek() const { return m_displayWeek; }
int ScheduleViewModel::totalWeeks() const {
    if (!m_repo || !hasSchedule()) return 20;
    return m_repo->currentScheduleConfig().totalWeeks;
}
int ScheduleViewModel::sectionsPerDay() const {
    if (!m_repo || !hasSchedule()) return 12;
    return m_repo->currentScheduleConfig().sectionsPerDay();
}
QString ScheduleViewModel::currentScheduleName() const {
    if (!m_repo || !hasSchedule()) return QString();
    return m_repo->currentScheduleConfig().semesterName;
}
QVariantList ScheduleViewModel::timeSlots() const {
    QVariantList result;
    if (!m_repo || !hasSchedule()) return result;
    for (const TimeSlot& slot : m_repo->currentScheduleConfig().timeSlots) {
        QVariantMap item;
        item["startTime"] = slot.startTime.toString(QStringLiteral("HH:mm"));
        item["endTime"] = slot.endTime.toString(QStringLiteral("HH:mm"));
        result.append(item);
    }
    return result;
}
QString ScheduleViewModel::errorMessage() const { return m_errorMessage; }
QObject* ScheduleViewModel::courseListModel() { return m_courseListModel; }
bool ScheduleViewModel::showNonCurrentWeekCourses() const { return m_showNonCurrentWeekCourses; }
void ScheduleViewModel::setShowNonCurrentWeekCourses(bool v) {
    if (m_showNonCurrentWeekCourses != v) {
        m_showNonCurrentWeekCourses = v;
        refreshCourses();
        emit displayOptionsChanged();
    }
}

void ScheduleViewModel::setError(const QString& message) {
    if (m_errorMessage == message) return;
    m_errorMessage = message;
    emit errorChanged();
}

} // namespace SCUNexus

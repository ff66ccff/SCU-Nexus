#include "ScheduleViewModel.h"
#include <QUuid>
#include <QDebug>
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
    case IdRole: return c.id;
    case NameRole: return c.name;
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
        {IdRole, "courseId"},
        {NameRole, "courseName"},
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

    if (!m_repo->init()) {
        m_errorMessage = QStringLiteral("数据库初始化失败");
        m_loading = false;
        emit errorChanged();
        emit loadingChanged();
        return;
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

void ScheduleViewModel::addCourse(const QVariantMap& data) {
    if (!m_repo || !hasSchedule()) return;

    Course course;
    course.id = data.value("id", QUuid::createUuid().toString(QUuid::WithoutBraces)).toString();
    course.name = data.value("name").toString();
    course.teacher = data.value("teacher").toString();
    course.location = data.value("location").toString();
    course.startWeek = data.value("startWeek", 1).toInt();
    course.endWeek = data.value("endWeek", m_repo->currentScheduleConfig().totalWeeks).toInt();
    course.dayOfWeek = data.value("dayOfWeek", 1).toInt();
    course.startSection = data.value("startSection", 1).toInt();
    course.endSection = data.value("endSection", 2).toInt();
    course.colorValue = static_cast<quint32>(data.value("colorValue", 0xFF42A5F5).toUInt());
    course.weekType = intToWeekType(data.value("weekType", 0).toInt());

    if (m_repo->addCourse(course)) {
        refreshCourses();
    }
}

void ScheduleViewModel::updateCourse(const QString& id, const QVariantMap& data) {
    if (!m_repo || !hasSchedule()) return;

    QList<Course> courses = m_repo->currentCourses();
    for (auto& course : courses) {
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

            if (m_repo->updateCourse(course)) {
                refreshCourses();
            }
            return;
        }
    }
}

void ScheduleViewModel::deleteCourse(const QString& id) {
    if (!m_repo) return;
    if (m_repo->deleteCourse(id)) {
        refreshCourses();
    }
}

void ScheduleViewModel::clearAllCourseData() {
    if (!m_repo) return;
    m_repo->clearAllCourseData();
    m_displayWeek = 1;
    m_courseListModel->clear();
    emit currentWeekChanged();
    emit configChanged();
    emit coursesChanged();
    emit dataReset();
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
QString ScheduleViewModel::currentScheduleName() const {
    if (!m_repo || !hasSchedule()) return QString();
    return m_repo->currentScheduleConfig().semesterName;
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

} // namespace SCUNexus

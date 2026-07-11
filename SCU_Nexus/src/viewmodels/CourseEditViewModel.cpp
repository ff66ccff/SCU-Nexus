#include "CourseEditViewModel.h"
#include "../services/course/CourseValidator.h"
#include <QUuid>
#include <QDebug>

namespace SCUNexus {

static const QList<quint32> COLOR_POOL = {
    0xFFEF5350, 0xFFEC407A, 0xFFAB47BC, 0xFF7E57C2,
    0xFF5C6BC0, 0xFF42A5F5, 0xFF26C6DA, 0xFF26A69A,
    0xFF66BB6A, 0xFF9CCC65, 0xFFFFA726, 0xFF8D6E63,
};

CourseEditViewModel::CourseEditViewModel(QObject* parent)
    : QObject(parent)
{
}

void CourseEditViewModel::setRepository(ScheduleRepository* repo) {
    m_repo = repo;
}

void CourseEditViewModel::initForAdd(int dayOfWeek, int startSection) {
    m_isEditMode = false;
    m_editingCourse = Course();
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.clear();
        emit errorChanged();
    }

    ScheduleConfig config = m_repo ? m_repo->currentScheduleConfig() : ScheduleConfig();

    m_editingCourse.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_editingCourse.name.clear();
    m_editingCourse.teacher.clear();
    m_editingCourse.location.clear();
    m_editingCourse.startWeek = 1;
    m_editingCourse.endWeek = config.totalWeeks > 0 ? config.totalWeeks : 20;
    m_editingCourse.dayOfWeek = std::clamp(dayOfWeek, 1, 7);
    m_editingCourse.startSection = std::max(1, startSection);
    m_editingCourse.endSection = std::min(m_editingCourse.startSection + 1,
                                          config.sectionsPerDay());
    m_editingCourse.weekType = WeekType::Every;
    m_editingCourse.colorValue = nextColor();

    emit modeChanged();
    emit courseChanged();
    emit configChanged();
}

void CourseEditViewModel::initForEdit(const QString& courseId) {
    m_isEditMode = true;

    if (!m_repo) return;

    QList<Course> courses = m_repo->currentCourses();
    for (const auto& c : courses) {
        if (c.id == courseId) {
            m_editingCourse = c;
            if (!m_errorMessage.isEmpty()) {
                m_errorMessage.clear();
                emit errorChanged();
            }
            emit modeChanged();
            emit courseChanged();
            return;
        }
    }

    m_errorMessage = QStringLiteral("课程不存在");
    emit errorChanged();
}

bool CourseEditViewModel::save() {
    if (!m_repo) {
        m_errorMessage = QStringLiteral("数据仓库未初始化");
        emit errorChanged();
        return false;
    }

    const Course candidate = buildCourse();
    QString error;
    if (!validateInternal(error)) {
        m_errorMessage = error;
        emit errorChanged();
        return false;
    }

    bool ok;
    if (m_isEditMode) {
        ok = m_repo->updateCourse(candidate);
    } else {
        ok = m_repo->addCourse(candidate);
    }

    if (!ok) {
        m_errorMessage = QStringLiteral("保存失败，请重试");
        emit errorChanged();
        return false;
    }

    m_errorMessage.clear();
    emit errorChanged();
    emit saved(m_editingCourse.id);
    return true;
}

bool CourseEditViewModel::validate() {
    QString error;
    if (!validateInternal(error)) {
        m_errorMessage = error;
        emit errorChanged();
        return false;
    }
    m_errorMessage.clear();
    emit errorChanged();
    return true;
}

bool CourseEditViewModel::validateInternal(QString& outError) const {
    const ScheduleConfig config = m_repo
        ? m_repo->currentScheduleConfig()
        : ScheduleConfig();
    const QList<Course> existing = m_repo
        ? m_repo->currentCourses()
        : QList<Course>{};
    const QString excludeId = m_isEditMode ? m_editingCourse.id : QString{};
    const CourseValidationResult result = CourseValidator::validate(
        buildCourse(), config, existing, excludeId);
    outError = result.message;
    return result.valid;
}

Course CourseEditViewModel::buildCourse() const {
    return m_editingCourse;
}

quint32 CourseEditViewModel::nextColor() {
    quint32 color = COLOR_POOL[m_colorIndex % COLOR_POOL.size()];
    m_colorIndex++;
    return color;
}

// Property accessors
bool CourseEditViewModel::isEditMode() const { return m_isEditMode; }
QString CourseEditViewModel::courseId() const { return m_editingCourse.id; }
QString CourseEditViewModel::name() const { return m_editingCourse.name; }
QString CourseEditViewModel::teacher() const { return m_editingCourse.teacher; }
QString CourseEditViewModel::location() const { return m_editingCourse.location; }
int CourseEditViewModel::startWeek() const { return m_editingCourse.startWeek; }
int CourseEditViewModel::endWeek() const { return m_editingCourse.endWeek; }
int CourseEditViewModel::dayOfWeek() const { return m_editingCourse.dayOfWeek; }
int CourseEditViewModel::startSection() const { return m_editingCourse.startSection; }
int CourseEditViewModel::endSection() const { return m_editingCourse.endSection; }
int CourseEditViewModel::weekType() const { return weekTypeToInt(m_editingCourse.weekType); }
quint32 CourseEditViewModel::colorValue() const { return m_editingCourse.colorValue; }
QString CourseEditViewModel::errorMessage() const { return m_errorMessage; }
int CourseEditViewModel::sectionsPerDay() const {
    return m_repo ? m_repo->currentScheduleConfig().sectionsPerDay() : 12;
}

void CourseEditViewModel::setName(const QString& v) { m_editingCourse.name = v; emit courseChanged(); }
void CourseEditViewModel::setTeacher(const QString& v) { m_editingCourse.teacher = v; emit courseChanged(); }
void CourseEditViewModel::setLocation(const QString& v) { m_editingCourse.location = v; emit courseChanged(); }
void CourseEditViewModel::setStartWeek(int v) { m_editingCourse.startWeek = v; emit courseChanged(); }
void CourseEditViewModel::setEndWeek(int v) { m_editingCourse.endWeek = v; emit courseChanged(); }
void CourseEditViewModel::setDayOfWeek(int v) { m_editingCourse.dayOfWeek = v; emit courseChanged(); }
void CourseEditViewModel::setStartSection(int v) { m_editingCourse.startSection = v; emit courseChanged(); }
void CourseEditViewModel::setEndSection(int v) { m_editingCourse.endSection = v; emit courseChanged(); }
void CourseEditViewModel::setWeekType(int v) { m_editingCourse.weekType = intToWeekType(v); emit courseChanged(); }
void CourseEditViewModel::setColorValue(quint32 v) { m_editingCourse.colorValue = v; emit courseChanged(); }

} // namespace SCUNexus

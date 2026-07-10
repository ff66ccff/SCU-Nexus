#include "CourseEditViewModel.h"
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

    QString error;
    if (!validateInternal(error)) {
        m_errorMessage = error;
        emit errorChanged();
        return false;
    }

    // Check conflicts
    const Course candidate = buildCourse();
    for (const Course& existing : m_repo->currentCourses()) {
        if (m_isEditMode && existing.id == m_editingCourse.id) {
            continue;
        }
        if (candidate.conflictsWith(existing)) {
            m_errorMessage = QStringLiteral("课程时间冲突：与“%1”（周%2，第 %3-%4 节）冲突。")
                .arg(existing.name)
                .arg(existing.dayOfWeek)
                .arg(existing.startSection)
                .arg(existing.endSection);
            emit errorChanged();
            return false;
        }
    }

    bool ok;
    if (m_isEditMode) {
        ok = m_repo->updateCourse(buildCourse());
    } else {
        ok = m_repo->addCourse(buildCourse());
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
    if (m_editingCourse.name.trimmed().isEmpty()) {
        outError = QStringLiteral("课程名不能为空");
        return false;
    }

    if (m_editingCourse.startWeek > m_editingCourse.endWeek) {
        outError = QStringLiteral("起始周不能大于结束周");
        return false;
    }

    if (m_editingCourse.startSection > m_editingCourse.endSection) {
        outError = QStringLiteral("起始节不能大于结束节");
        return false;
    }

    if (m_editingCourse.dayOfWeek < 1 || m_editingCourse.dayOfWeek > 7) {
        outError = QStringLiteral("星期必须在 1 到 7 之间");
        return false;
    }

    if (m_repo) {
        ScheduleConfig config = m_repo->currentScheduleConfig();
        int totalSections = config.sectionsPerDay();

        if (m_editingCourse.startWeek < 1 || m_editingCourse.endWeek > config.totalWeeks) {
            outError = QStringLiteral("周次必须在 1 到 %1 之间").arg(config.totalWeeks);
            return false;
        }

        if (m_editingCourse.startSection < 1) {
            outError = QStringLiteral("节次必须从 1 开始");
            return false;
        }

        if (m_editingCourse.endSection > totalSections) {
            outError = QStringLiteral("节次不能超过当前配置（%1）").arg(totalSections);
            return false;
        }

        // Check cross-period
        int morningEnd = config.morningSections;
        int afternoonEnd = morningEnd + config.afternoonSections;

        if (m_editingCourse.startSection <= morningEnd && m_editingCourse.endSection > morningEnd) {
            outError = QStringLiteral("一门课不能跨越上午、下午或晚上时段。");
            return false;
        }
        if (m_editingCourse.startSection <= afternoonEnd && m_editingCourse.endSection > afternoonEnd) {
            outError = QStringLiteral("一门课不能跨越上午、下午或晚上时段。");
            return false;
        }
    }

    return true;
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

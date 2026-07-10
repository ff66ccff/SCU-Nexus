#include "ScheduleRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QDebug>

namespace SCUNexus {

static const QString METADATA_KEY_CURRENT_SCHEDULE = QStringLiteral("currentScheduleId");

namespace {

bool insertCourses(QSqlDatabase& db, const QString& scheduleId, const QList<Course>& courses) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO courses (id, schedule_id, name, teacher, location, "
        "start_week, end_week, day_of_week, start_section, end_section, "
        "color_value, week_type) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

    for (const Course& course : courses) {
        const QString id = course.id.isEmpty()
            ? QUuid::createUuid().toString(QUuid::WithoutBraces)
            : course.id;
        query.addBindValue(id);
        query.addBindValue(scheduleId);
        query.addBindValue(course.name);
        query.addBindValue(course.teacher);
        query.addBindValue(course.location);
        query.addBindValue(course.startWeek);
        query.addBindValue(course.endWeek);
        query.addBindValue(course.dayOfWeek);
        query.addBindValue(course.startSection);
        query.addBindValue(course.endSection);
        query.addBindValue(static_cast<int>(course.colorValue));
        query.addBindValue(weekTypeToInt(course.weekType));
        if (!query.exec()) {
            qWarning() << "Failed to insert imported course:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool writeCurrentScheduleMetadata(QSqlDatabase& db, const QString& scheduleId) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO metadata (key, value) VALUES (?, ?)"));
    query.addBindValue(METADATA_KEY_CURRENT_SCHEDULE);
    query.addBindValue(scheduleId.isNull() ? QStringLiteral("") : scheduleId);
    if (!query.exec()) {
        qWarning() << "Failed to save current schedule metadata:" << query.lastError().text();
        return false;
    }
    return true;
}

} // namespace

ScheduleRepository::ScheduleRepository(QObject* parent)
    : QObject(parent)
{
    m_dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
               + QStringLiteral("/scu_nexus.db");
    m_connectionName = QStringLiteral("schedule_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

ScheduleRepository::~ScheduleRepository() {
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

void ScheduleRepository::setDatabasePath(const QString& path) {
    m_dbPath = path;
}

QString ScheduleRepository::databasePath() const {
    return m_dbPath;
}

bool ScheduleRepository::init() {
    if (m_initialized) return true;

    // Ensure directory exists
    QDir dir = QFileInfo(m_dbPath).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        qWarning() << "ScheduleRepository: Failed to open database:" << db.lastError().text();
        return false;
    }

    // Enable foreign keys
    QSqlQuery query(db);
    query.exec(QStringLiteral("PRAGMA foreign_keys = ON"));

    if (!createTables()) {
        return false;
    }

    if (!loadMetadata()) {
        return false;
    }

    if (!loadSchedulesCache()) {
        return false;
    }

    if (!loadCoursesCache()) {
        return false;
    }

    m_initialized = true;
    return true;
}

bool ScheduleRepository::createTables() {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Metadata table
    if (!query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS metadata ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ")"))) {
        qWarning() << "Failed to create metadata table:" << query.lastError().text();
        return false;
    }

    // Schedules table
    if (!query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS schedules ("
        "  id TEXT PRIMARY KEY,"
        "  config_json TEXT NOT NULL"
        ")"))) {
        qWarning() << "Failed to create schedules table:" << query.lastError().text();
        return false;
    }

    // Courses table
    if (!query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS courses ("
        "  id TEXT PRIMARY KEY,"
        "  schedule_id TEXT NOT NULL,"
        "  name TEXT,"
        "  teacher TEXT,"
        "  location TEXT,"
        "  start_week INTEGER,"
        "  end_week INTEGER,"
        "  day_of_week INTEGER,"
        "  start_section INTEGER,"
        "  end_section INTEGER,"
        "  color_value INTEGER,"
        "  week_type INTEGER,"
        "  FOREIGN KEY (schedule_id) REFERENCES schedules(id) ON DELETE CASCADE"
        ")"))) {
        qWarning() << "Failed to create courses table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool ScheduleRepository::loadMetadata() {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT value FROM metadata WHERE key = ?"));
    query.addBindValue(METADATA_KEY_CURRENT_SCHEDULE);

    if (query.exec() && query.next()) {
        m_currentScheduleId = query.value(0).toString();
    } else {
        // New install: no current schedule
        m_currentScheduleId.clear();
    }

    return true;
}

bool ScheduleRepository::saveMetadata(const QString& key, const QString& value) {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO metadata (key, value) VALUES (?, ?)"));
    query.addBindValue(key);
    query.addBindValue(value.isNull() ? QStringLiteral("") : value);

    if (!query.exec()) {
        qWarning() << "Failed to save metadata:" << query.lastError().text();
        return false;
    }
    return true;
}

bool ScheduleRepository::loadSchedulesCache() {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT config_json FROM schedules"))) {
        qWarning() << "Failed to load schedules cache:" << query.lastError().text();
        return false;
    }

    m_schedulesCache.clear();
    while (query.next()) {
        QJsonDocument doc = QJsonDocument::fromJson(query.value(0).toString().toUtf8());
        if (!doc.isNull()) {
            m_schedulesCache.append(ScheduleConfig::fromJson(doc.object()));
        }
    }

    return true;
}

bool ScheduleRepository::loadCoursesCache() {
    if (m_currentScheduleId.isEmpty()) {
        m_coursesCache.clear();
        return true;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM courses WHERE schedule_id = ?"));
    query.addBindValue(m_currentScheduleId);

    m_coursesCache.clear();
    if (!query.exec()) {
        qWarning() << "Failed to load courses cache:" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        m_coursesCache.append(courseFromQuery(query));
    }

    return true;
}

Course ScheduleRepository::courseFromQuery(const QSqlQuery& query) const {
    Course course;
    course.id = query.value(QStringLiteral("id")).toString();
    course.name = query.value(QStringLiteral("name")).toString();
    course.teacher = query.value(QStringLiteral("teacher")).toString();
    course.location = query.value(QStringLiteral("location")).toString();
    course.startWeek = query.value(QStringLiteral("start_week")).toInt();
    course.endWeek = query.value(QStringLiteral("end_week")).toInt();
    course.dayOfWeek = query.value(QStringLiteral("day_of_week")).toInt();
    course.startSection = query.value(QStringLiteral("start_section")).toInt();
    course.endSection = query.value(QStringLiteral("end_section")).toInt();
    course.colorValue = static_cast<quint32>(query.value(QStringLiteral("color_value")).toInt());
    course.weekType = intToWeekType(query.value(QStringLiteral("week_type")).toInt());
    return course;
}

// ---- Schedule-level operations ----

QString ScheduleRepository::currentScheduleId() const {
    return m_currentScheduleId;
}

QList<ScheduleConfig> ScheduleRepository::allSchedules() const {
    return m_schedulesCache;
}

ScheduleConfig ScheduleRepository::currentScheduleConfig() const {
    for (const auto& s : m_schedulesCache) {
        if (s.id == m_currentScheduleId) {
            return s;
        }
    }
    // Return a placeholder config for UI protection
    ScheduleConfig placeholder = ScheduleConfig::createDefault(QString());
    placeholder.id.clear();
    return placeholder;
}

bool ScheduleRepository::switchSchedule(const QString& scheduleId) {
    if (scheduleId == m_currentScheduleId) return true;

    // Verify schedule exists
    bool exists = false;
    for (const auto& s : m_schedulesCache) {
        if (s.id == scheduleId) {
            exists = true;
            break;
        }
    }
    if (!exists && !scheduleId.isEmpty()) return false;

    const QString previousId = m_currentScheduleId;
    m_currentScheduleId = scheduleId;
    if (!saveMetadata(METADATA_KEY_CURRENT_SCHEDULE, scheduleId)) {
        m_currentScheduleId = previousId;
        return false;
    }

    if (!loadCoursesCache()) {
        m_currentScheduleId = previousId;
        saveMetadata(METADATA_KEY_CURRENT_SCHEDULE, previousId);
        loadCoursesCache();
        return false;
    }

    emit dataChanged();
    return true;
}

bool ScheduleRepository::addSchedule(const ScheduleConfig& config) {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) return false;

    const bool isFirstSchedule = m_schedulesCache.isEmpty();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO schedules (id, config_json) VALUES (?, ?)"));
    query.addBindValue(config.id);
    query.addBindValue(QString::fromUtf8(QJsonDocument(config.toJson()).toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        db.rollback();
        qWarning() << "Failed to add schedule:" << query.lastError().text();
        return false;
    }

    if ((isFirstSchedule && !writeCurrentScheduleMetadata(db, config.id))
        || !db.commit()) {
        db.rollback();
        return false;
    }

    if (isFirstSchedule) m_currentScheduleId = config.id;
    if (!loadSchedulesCache() || !loadCoursesCache()) return false;
    emit dataChanged();
    return true;
}

bool ScheduleRepository::deleteSchedule(const QString& scheduleId) {
    const bool exists = std::any_of(m_schedulesCache.cbegin(), m_schedulesCache.cend(),
                                    [&](const ScheduleConfig& config) {
                                        return config.id == scheduleId;
                                    });
    if (!exists) {
        return false;
    }

    QString replacementId = m_currentScheduleId;
    if (scheduleId == m_currentScheduleId) {
        replacementId.clear();
        for (const ScheduleConfig& config : std::as_const(m_schedulesCache)) {
            if (config.id != scheduleId) {
                replacementId = config.id;
                break;
            }
        }
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!db.transaction()) {
        return false;
    }

    query.prepare(QStringLiteral("DELETE FROM schedules WHERE id = ?"));
    query.addBindValue(scheduleId);
    if (!query.exec() || query.numRowsAffected() != 1) {
        db.rollback();
        qWarning() << "Failed to delete schedule:" << query.lastError().text();
        return false;
    }

    if (scheduleId == m_currentScheduleId
        && !writeCurrentScheduleMetadata(db, replacementId)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        db.rollback();
        return false;
    }

    m_currentScheduleId = replacementId;
    if (!loadSchedulesCache() || !loadCoursesCache()) return false;
    emit dataChanged();

    return true;
}

bool ScheduleRepository::saveScheduleConfig(const ScheduleConfig& config) {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE schedules SET config_json = ? WHERE id = ?"));
    query.addBindValue(QString::fromUtf8(QJsonDocument(config.toJson()).toJson(QJsonDocument::Compact)));
    query.addBindValue(config.id);

    if (!query.exec() || query.numRowsAffected() != 1) {
        qWarning() << "Failed to save schedule config:" << query.lastError().text();
        return false;
    }

    // Refresh cache
    loadSchedulesCache();
    emit dataChanged();
    return true;
}

// ---- Course-level operations ----

QList<Course> ScheduleRepository::currentCourses() const {
    return m_coursesCache;
}

QList<Course> ScheduleRepository::coursesForSchedule(const QString& scheduleId) const {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM courses WHERE schedule_id = ?"));
    query.addBindValue(scheduleId);

    QList<Course> courses;
    if (!query.exec()) {
        qWarning() << "Failed to query courses:" << query.lastError().text();
        return courses;
    }

    while (query.next()) {
        courses.append(courseFromQuery(query));
    }

    return courses;
}

bool ScheduleRepository::addCourse(const Course& course) {
    if (m_currentScheduleId.isEmpty()) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO courses (id, schedule_id, name, teacher, location, "
        "start_week, end_week, day_of_week, start_section, end_section, "
        "color_value, week_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(course.id);
    query.addBindValue(m_currentScheduleId);
    query.addBindValue(course.name);
    query.addBindValue(course.teacher);
    query.addBindValue(course.location);
    query.addBindValue(course.startWeek);
    query.addBindValue(course.endWeek);
    query.addBindValue(course.dayOfWeek);
    query.addBindValue(course.startSection);
    query.addBindValue(course.endSection);
    query.addBindValue(static_cast<int>(course.colorValue));
    query.addBindValue(weekTypeToInt(course.weekType));

    if (!query.exec()) {
        qWarning() << "Failed to add course:" << query.lastError().text();
        return false;
    }

    m_coursesCache.append(course);
    emit dataChanged();
    return true;
}

bool ScheduleRepository::updateCourse(const Course& course) {
    if (m_currentScheduleId.isEmpty()) return false;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "UPDATE courses SET name=?, teacher=?, location=?, start_week=?, end_week=?, "
        "day_of_week=?, start_section=?, end_section=?, color_value=?, week_type=? "
        "WHERE id=? AND schedule_id=?"));
    query.addBindValue(course.name);
    query.addBindValue(course.teacher);
    query.addBindValue(course.location);
    query.addBindValue(course.startWeek);
    query.addBindValue(course.endWeek);
    query.addBindValue(course.dayOfWeek);
    query.addBindValue(course.startSection);
    query.addBindValue(course.endSection);
    query.addBindValue(static_cast<int>(course.colorValue));
    query.addBindValue(weekTypeToInt(course.weekType));
    query.addBindValue(course.id);
    query.addBindValue(m_currentScheduleId);

    if (!query.exec() || query.numRowsAffected() != 1) {
        qWarning() << "Failed to update course:" << query.lastError().text();
        return false;
    }

    // Update cache
    for (int i = 0; i < m_coursesCache.size(); ++i) {
        if (m_coursesCache[i].id == course.id) {
            m_coursesCache[i] = course;
            break;
        }
    }

    emit dataChanged();
    return true;
}

bool ScheduleRepository::deleteCourse(const QString& courseId) {
    if (m_currentScheduleId.isEmpty()) return false;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM courses WHERE id = ? AND schedule_id = ?"));
    query.addBindValue(courseId);
    query.addBindValue(m_currentScheduleId);

    if (!query.exec() || query.numRowsAffected() != 1) {
        qWarning() << "Failed to delete course:" << query.lastError().text();
        return false;
    }

    // Remove from cache
    m_coursesCache.erase(
        std::remove_if(m_coursesCache.begin(), m_coursesCache.end(),
                       [&](const Course& c) { return c.id == courseId; }),
        m_coursesCache.end());

    emit dataChanged();
    return true;
}

bool ScheduleRepository::replaceScheduleCourses(const QString& scheduleId, const QList<Course>& courses) {
    const bool scheduleExists = std::any_of(m_schedulesCache.cbegin(), m_schedulesCache.cend(),
                                            [&](const ScheduleConfig& config) {
                                                return config.id == scheduleId;
                                            });
    if (!scheduleExists) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        return false;
    }

    // Delete all existing courses for this schedule
    QSqlQuery delQuery(db);
    delQuery.prepare(QStringLiteral("DELETE FROM courses WHERE schedule_id = ?"));
    delQuery.addBindValue(scheduleId);
    if (!delQuery.exec()) {
        db.rollback();
        qWarning() << "Failed to delete courses for replace:" << delQuery.lastError().text();
        return false;
    }

    // Insert new courses (generate new IDs to avoid PK conflicts)
    QSqlQuery insQuery(db);
    insQuery.prepare(QStringLiteral(
        "INSERT INTO courses (id, schedule_id, name, teacher, location, "
        "start_week, end_week, day_of_week, start_section, end_section, "
        "color_value, week_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

    for (const auto& course : courses) {
        QString newId = course.id.isEmpty()
            ? QUuid::createUuid().toString(QUuid::WithoutBraces)
            : course.id;
        insQuery.addBindValue(newId);
        insQuery.addBindValue(scheduleId);
        insQuery.addBindValue(course.name);
        insQuery.addBindValue(course.teacher);
        insQuery.addBindValue(course.location);
        insQuery.addBindValue(course.startWeek);
        insQuery.addBindValue(course.endWeek);
        insQuery.addBindValue(course.dayOfWeek);
        insQuery.addBindValue(course.startSection);
        insQuery.addBindValue(course.endSection);
        insQuery.addBindValue(static_cast<int>(course.colorValue));
        insQuery.addBindValue(weekTypeToInt(course.weekType));

        if (!insQuery.exec()) {
            db.rollback();
            qWarning() << "Failed to insert course during replace:" << insQuery.lastError().text();
            return false;
        }
    }

    if (!db.commit()) {
        qWarning() << "Failed to commit replace:" << db.lastError().text();
        db.rollback();
        return false;
    }

    // Refresh cache if this was the current schedule
    if (scheduleId == m_currentScheduleId) {
        loadCoursesCache();
    }

    emit dataChanged();
    return true;
}

bool ScheduleRepository::addScheduleWithCoursesAndSwitch(
    const ScheduleConfig& config, const QList<Course>& courses)
{
    if (config.id.isEmpty()) return false;
    const bool exists = std::any_of(m_schedulesCache.cbegin(), m_schedulesCache.cend(),
                                    [&](const ScheduleConfig& item) {
                                        return item.id == config.id;
                                    });
    if (exists) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) return false;

    QSqlQuery scheduleQuery(db);
    scheduleQuery.prepare(QStringLiteral(
        "INSERT INTO schedules (id, config_json) VALUES (?, ?)"));
    scheduleQuery.addBindValue(config.id);
    scheduleQuery.addBindValue(
        QString::fromUtf8(QJsonDocument(config.toJson()).toJson(QJsonDocument::Compact)));
    if (!scheduleQuery.exec()
        || !insertCourses(db, config.id, courses)
        || !writeCurrentScheduleMetadata(db, config.id)
        || !db.commit()) {
        db.rollback();
        return false;
    }

    m_currentScheduleId = config.id;
    if (!loadSchedulesCache() || !loadCoursesCache()) return false;
    emit dataChanged();
    return true;
}

bool ScheduleRepository::replaceScheduleCoursesAndSwitch(
    const QString& scheduleId, const QList<Course>& courses)
{
    const bool exists = std::any_of(m_schedulesCache.cbegin(), m_schedulesCache.cend(),
                                    [&](const ScheduleConfig& config) {
                                        return config.id == scheduleId;
                                    });
    if (!exists) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) return false;

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare(QStringLiteral("DELETE FROM courses WHERE schedule_id = ?"));
    deleteQuery.addBindValue(scheduleId);
    if (!deleteQuery.exec()
        || !insertCourses(db, scheduleId, courses)
        || !writeCurrentScheduleMetadata(db, scheduleId)
        || !db.commit()) {
        db.rollback();
        return false;
    }

    m_currentScheduleId = scheduleId;
    if (!loadSchedulesCache() || !loadCoursesCache()) return false;
    emit dataChanged();
    return true;
}

bool ScheduleRepository::hasConflict(const Course& course, const QString& excludeId) const {
    for (const auto& existing : m_coursesCache) {
        if (!excludeId.isEmpty() && existing.id == excludeId) {
            continue;
        }
        if (course.conflictsWith(existing)) {
            return true;
        }
    }
    return false;
}

bool ScheduleRepository::clearAllCourseData() {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!db.transaction()
        || !query.exec(QStringLiteral("DELETE FROM courses"))
        || !query.exec(QStringLiteral("DELETE FROM schedules"))
        || !query.exec(QStringLiteral("DELETE FROM metadata"))
        || !db.commit()) {
        db.rollback();
        return false;
    }

    m_currentScheduleId.clear();
    m_schedulesCache.clear();
    m_coursesCache.clear();

    emit dataChanged();
    return true;
}

} // namespace SCUNexus

#ifndef COURSEEDITVIEWMODEL_H
#define COURSEEDITVIEWMODEL_H

#include <QObject>
#include <QVariantMap>
#include <QString>
#include "../models/Course.h"
#include "../models/ScheduleConfig.h"
#include "../repositories/ScheduleRepository.h"

namespace SCUNexus {

class CourseEditViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isEditMode READ isEditMode NOTIFY modeChanged)
    Q_PROPERTY(QString courseId READ courseId NOTIFY courseChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY courseChanged)
    Q_PROPERTY(QString teacher READ teacher WRITE setTeacher NOTIFY courseChanged)
    Q_PROPERTY(QString location READ location WRITE setLocation NOTIFY courseChanged)
    Q_PROPERTY(int startWeek READ startWeek WRITE setStartWeek NOTIFY courseChanged)
    Q_PROPERTY(int endWeek READ endWeek WRITE setEndWeek NOTIFY courseChanged)
    Q_PROPERTY(int dayOfWeek READ dayOfWeek WRITE setDayOfWeek NOTIFY courseChanged)
    Q_PROPERTY(int startSection READ startSection WRITE setStartSection NOTIFY courseChanged)
    Q_PROPERTY(int endSection READ endSection WRITE setEndSection NOTIFY courseChanged)
    Q_PROPERTY(int weekType READ weekType WRITE setWeekType NOTIFY courseChanged)
    Q_PROPERTY(quint32 colorValue READ colorValue WRITE setColorValue NOTIFY courseChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(int sectionsPerDay READ sectionsPerDay NOTIFY configChanged)

public:
    explicit CourseEditViewModel(QObject* parent = nullptr);

    void setRepository(ScheduleRepository* repo);

    // Initialize for adding a new course
    Q_INVOKABLE void initForAdd(int dayOfWeek, int startSection);

    // Initialize for editing an existing course
    Q_INVOKABLE void initForEdit(const QString& courseId);

    // Save the course (validates and calls repo)
    Q_INVOKABLE bool save();

    // Validate without saving
    Q_INVOKABLE bool validate();

    // Property accessors
    bool isEditMode() const;
    QString courseId() const;
    QString name() const;
    QString teacher() const;
    QString location() const;
    int startWeek() const;
    int endWeek() const;
    int dayOfWeek() const;
    int startSection() const;
    int endSection() const;
    int weekType() const;
    quint32 colorValue() const;
    QString errorMessage() const;
    int sectionsPerDay() const;

    void setName(const QString& v);
    void setTeacher(const QString& v);
    void setLocation(const QString& v);
    void setStartWeek(int v);
    void setEndWeek(int v);
    void setDayOfWeek(int v);
    void setStartSection(int v);
    void setEndSection(int v);
    void setWeekType(int v);
    void setColorValue(quint32 v);

signals:
    void modeChanged();
    void courseChanged();
    void errorChanged();
    void configChanged();
    void saved(const QString& courseId);
    void cancelled();

private:
    Course buildCourse() const;
    bool validateInternal(QString& outError) const;
    quint32 nextColor();

    ScheduleRepository* m_repo = nullptr;
    Course m_editingCourse;
    bool m_isEditMode = false;
    QString m_errorMessage;
    int m_colorIndex = 0;
};

} // namespace SCUNexus

#endif // COURSEEDITVIEWMODEL_H

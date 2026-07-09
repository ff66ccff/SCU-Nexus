/****************************************************************************
** Meta object code from reading C++ file 'ScheduleViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/viewmodels/ScheduleViewModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ScheduleViewModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN8SCUNexus15CourseListModelE_t {};
} // unnamed namespace

template <> constexpr inline auto SCUNexus::CourseListModel::qt_create_metaobjectdata<qt_meta_tag_ZN8SCUNexus15CourseListModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SCUNexus::CourseListModel"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CourseListModel, qt_meta_tag_ZN8SCUNexus15CourseListModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SCUNexus::CourseListModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus15CourseListModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus15CourseListModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8SCUNexus15CourseListModelE_t>.metaTypes,
    nullptr
} };

void SCUNexus::CourseListModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CourseListModel *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *SCUNexus::CourseListModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SCUNexus::CourseListModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus15CourseListModelE_t>.strings))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int SCUNexus::CourseListModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_tag_ZN8SCUNexus17ScheduleViewModelE_t {};
} // unnamed namespace

template <> constexpr inline auto SCUNexus::ScheduleViewModel::qt_create_metaobjectdata<qt_meta_tag_ZN8SCUNexus17ScheduleViewModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SCUNexus::ScheduleViewModel",
        "loadingChanged",
        "",
        "schedulesChanged",
        "currentWeekChanged",
        "configChanged",
        "errorChanged",
        "coursesChanged",
        "displayOptionsChanged",
        "dataReset",
        "load",
        "switchSchedule",
        "scheduleId",
        "schedules",
        "QVariantList",
        "updateCurrentWeek",
        "week",
        "goPreviousWeek",
        "goNextWeek",
        "goToCurrentWeek",
        "addCourse",
        "QVariantMap",
        "data",
        "updateCourse",
        "id",
        "deleteCourse",
        "clearAllCourseData",
        "courseById",
        "loading",
        "hasSchedule",
        "currentWeek",
        "totalWeeks",
        "currentScheduleName",
        "errorMessage",
        "courseListModel",
        "showNonCurrentWeekCourses"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'loadingChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'schedulesChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentWeekChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'configChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'coursesChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'displayOptionsChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'dataReset'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'load'
        QtMocHelpers::MethodData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'switchSchedule'
        QtMocHelpers::MethodData<void(const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Method 'schedules'
        QtMocHelpers::MethodData<QVariantList() const>(13, 2, QMC::AccessPublic, 0x80000000 | 14),
        // Method 'updateCurrentWeek'
        QtMocHelpers::MethodData<void(int)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 16 },
        }}),
        // Method 'goPreviousWeek'
        QtMocHelpers::MethodData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'goNextWeek'
        QtMocHelpers::MethodData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'goToCurrentWeek'
        QtMocHelpers::MethodData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'addCourse'
        QtMocHelpers::MethodData<void(const QVariantMap &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Method 'updateCourse'
        QtMocHelpers::MethodData<void(const QString &, const QVariantMap &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 }, { 0x80000000 | 21, 22 },
        }}),
        // Method 'deleteCourse'
        QtMocHelpers::MethodData<void(const QString &)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'clearAllCourseData'
        QtMocHelpers::MethodData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'courseById'
        QtMocHelpers::MethodData<QVariantMap(const QString &) const>(27, 2, QMC::AccessPublic, 0x80000000 | 21, {{
            { QMetaType::QString, 24 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'loading'
        QtMocHelpers::PropertyData<bool>(28, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'hasSchedule'
        QtMocHelpers::PropertyData<bool>(29, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'currentWeek'
        QtMocHelpers::PropertyData<int>(30, QMetaType::Int, QMC::DefaultPropertyFlags, 2),
        // property 'totalWeeks'
        QtMocHelpers::PropertyData<int>(31, QMetaType::Int, QMC::DefaultPropertyFlags, 3),
        // property 'currentScheduleName'
        QtMocHelpers::PropertyData<QString>(32, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'errorMessage'
        QtMocHelpers::PropertyData<QString>(33, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'courseListModel'
        QtMocHelpers::PropertyData<QObject*>(34, QMetaType::QObjectStar, QMC::DefaultPropertyFlags, 5),
        // property 'showNonCurrentWeekCourses'
        QtMocHelpers::PropertyData<bool>(35, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 6),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ScheduleViewModel, qt_meta_tag_ZN8SCUNexus17ScheduleViewModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SCUNexus::ScheduleViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus17ScheduleViewModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus17ScheduleViewModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8SCUNexus17ScheduleViewModelE_t>.metaTypes,
    nullptr
} };

void SCUNexus::ScheduleViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ScheduleViewModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->loadingChanged(); break;
        case 1: _t->schedulesChanged(); break;
        case 2: _t->currentWeekChanged(); break;
        case 3: _t->configChanged(); break;
        case 4: _t->errorChanged(); break;
        case 5: _t->coursesChanged(); break;
        case 6: _t->displayOptionsChanged(); break;
        case 7: _t->dataReset(); break;
        case 8: _t->load(); break;
        case 9: _t->switchSchedule((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: { QVariantList _r = _t->schedules();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 11: _t->updateCurrentWeek((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->goPreviousWeek(); break;
        case 13: _t->goNextWeek(); break;
        case 14: _t->goToCurrentWeek(); break;
        case 15: _t->addCourse((*reinterpret_cast<std::add_pointer_t<QVariantMap>>(_a[1]))); break;
        case 16: _t->updateCourse((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QVariantMap>>(_a[2]))); break;
        case 17: _t->deleteCourse((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: _t->clearAllCourseData(); break;
        case 19: { QVariantMap _r = _t->courseById((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ScheduleViewModel::*)()>(_a, &ScheduleViewModel::loadingChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleViewModel::*)()>(_a, &ScheduleViewModel::schedulesChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleViewModel::*)()>(_a, &ScheduleViewModel::currentWeekChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleViewModel::*)()>(_a, &ScheduleViewModel::configChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleViewModel::*)()>(_a, &ScheduleViewModel::errorChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleViewModel::*)()>(_a, &ScheduleViewModel::coursesChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleViewModel::*)()>(_a, &ScheduleViewModel::displayOptionsChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleViewModel::*)()>(_a, &ScheduleViewModel::dataReset, 7))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->loading(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->hasSchedule(); break;
        case 2: *reinterpret_cast<int*>(_v) = _t->currentWeek(); break;
        case 3: *reinterpret_cast<int*>(_v) = _t->totalWeeks(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->currentScheduleName(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->errorMessage(); break;
        case 6: *reinterpret_cast<QObject**>(_v) = _t->courseListModel(); break;
        case 7: *reinterpret_cast<bool*>(_v) = _t->showNonCurrentWeekCourses(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 7: _t->setShowNonCurrentWeekCourses(*reinterpret_cast<bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *SCUNexus::ScheduleViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SCUNexus::ScheduleViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus17ScheduleViewModelE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SCUNexus::ScheduleViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 20;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void SCUNexus::ScheduleViewModel::loadingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SCUNexus::ScheduleViewModel::schedulesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SCUNexus::ScheduleViewModel::currentWeekChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SCUNexus::ScheduleViewModel::configChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SCUNexus::ScheduleViewModel::errorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SCUNexus::ScheduleViewModel::coursesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SCUNexus::ScheduleViewModel::displayOptionsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SCUNexus::ScheduleViewModel::dataReset()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP

/****************************************************************************
** Meta object code from reading C++ file 'ScheduleImportViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/viewmodels/ScheduleImportViewModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ScheduleImportViewModel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8SCUNexus23ScheduleImportViewModelE_t {};
} // unnamed namespace

template <> constexpr inline auto SCUNexus::ScheduleImportViewModel::qt_create_metaobjectdata<qt_meta_tag_ZN8SCUNexus23ScheduleImportViewModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SCUNexus::ScheduleImportViewModel",
        "loadingChanged",
        "",
        "errorChanged",
        "statusChanged",
        "conflictChanged",
        "importCompleteChanged",
        "importFinished",
        "scheduleId",
        "availableSemesters",
        "QVariantList",
        "importSchedule",
        "planCode",
        "semesterLabel",
        "importFromJson",
        "QJsonObject",
        "rawJson",
        "resolveConflict",
        "strategy",
        "syncCurrentWeek",
        "currentWeek",
        "loading",
        "errorMessage",
        "statusMessage",
        "hasConflict",
        "conflictMessage",
        "importComplete"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'loadingChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'statusChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'conflictChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'importCompleteChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'importFinished'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Method 'availableSemesters'
        QtMocHelpers::MethodData<QVariantList() const>(9, 2, QMC::AccessPublic, 0x80000000 | 10),
        // Method 'importSchedule'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 12 }, { QMetaType::QString, 13 },
        }}),
        // Method 'importFromJson'
        QtMocHelpers::MethodData<bool(const QString &, const QString &, const QJsonObject &)>(14, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 12 }, { QMetaType::QString, 13 }, { 0x80000000 | 15, 16 },
        }}),
        // Method 'resolveConflict'
        QtMocHelpers::MethodData<void(const QString &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 18 },
        }}),
        // Method 'syncCurrentWeek'
        QtMocHelpers::MethodData<bool(int)>(19, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 20 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'loading'
        QtMocHelpers::PropertyData<bool>(21, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'errorMessage'
        QtMocHelpers::PropertyData<QString>(22, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'statusMessage'
        QtMocHelpers::PropertyData<QString>(23, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'hasConflict'
        QtMocHelpers::PropertyData<bool>(24, QMetaType::Bool, QMC::DefaultPropertyFlags, 3),
        // property 'conflictMessage'
        QtMocHelpers::PropertyData<QString>(25, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'importComplete'
        QtMocHelpers::PropertyData<bool>(26, QMetaType::Bool, QMC::DefaultPropertyFlags, 4),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ScheduleImportViewModel, qt_meta_tag_ZN8SCUNexus23ScheduleImportViewModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SCUNexus::ScheduleImportViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus23ScheduleImportViewModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus23ScheduleImportViewModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8SCUNexus23ScheduleImportViewModelE_t>.metaTypes,
    nullptr
} };

void SCUNexus::ScheduleImportViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ScheduleImportViewModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->loadingChanged(); break;
        case 1: _t->errorChanged(); break;
        case 2: _t->statusChanged(); break;
        case 3: _t->conflictChanged(); break;
        case 4: _t->importCompleteChanged(); break;
        case 5: _t->importFinished((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: { QVariantList _r = _t->availableSemesters();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->importSchedule((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->importFromJson((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[3])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 9: _t->resolveConflict((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: { bool _r = _t->syncCurrentWeek((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ScheduleImportViewModel::*)()>(_a, &ScheduleImportViewModel::loadingChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleImportViewModel::*)()>(_a, &ScheduleImportViewModel::errorChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleImportViewModel::*)()>(_a, &ScheduleImportViewModel::statusChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleImportViewModel::*)()>(_a, &ScheduleImportViewModel::conflictChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleImportViewModel::*)()>(_a, &ScheduleImportViewModel::importCompleteChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScheduleImportViewModel::*)(const QString & )>(_a, &ScheduleImportViewModel::importFinished, 5))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->loading(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->errorMessage(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->statusMessage(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->hasConflict(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->conflictMessage(); break;
        case 5: *reinterpret_cast<bool*>(_v) = _t->importComplete(); break;
        default: break;
        }
    }
}

const QMetaObject *SCUNexus::ScheduleImportViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SCUNexus::ScheduleImportViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus23ScheduleImportViewModelE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SCUNexus::ScheduleImportViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void SCUNexus::ScheduleImportViewModel::loadingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SCUNexus::ScheduleImportViewModel::errorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SCUNexus::ScheduleImportViewModel::statusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SCUNexus::ScheduleImportViewModel::conflictChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SCUNexus::ScheduleImportViewModel::importCompleteChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SCUNexus::ScheduleImportViewModel::importFinished(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}
QT_WARNING_POP

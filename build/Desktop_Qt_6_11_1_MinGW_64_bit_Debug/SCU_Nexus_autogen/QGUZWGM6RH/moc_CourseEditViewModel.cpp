/****************************************************************************
** Meta object code from reading C++ file 'CourseEditViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/viewmodels/CourseEditViewModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CourseEditViewModel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8SCUNexus19CourseEditViewModelE_t {};
} // unnamed namespace

template <> constexpr inline auto SCUNexus::CourseEditViewModel::qt_create_metaobjectdata<qt_meta_tag_ZN8SCUNexus19CourseEditViewModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SCUNexus::CourseEditViewModel",
        "modeChanged",
        "",
        "courseChanged",
        "errorChanged",
        "configChanged",
        "saved",
        "courseId",
        "cancelled",
        "initForAdd",
        "dayOfWeek",
        "startSection",
        "initForEdit",
        "save",
        "validate",
        "isEditMode",
        "name",
        "teacher",
        "location",
        "startWeek",
        "endWeek",
        "endSection",
        "weekType",
        "colorValue",
        "errorMessage",
        "sectionsPerDay"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'modeChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'courseChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'configChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'saved'
        QtMocHelpers::SignalData<void(const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Signal 'cancelled'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'initForAdd'
        QtMocHelpers::MethodData<void(int, int)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 }, { QMetaType::Int, 11 },
        }}),
        // Method 'initForEdit'
        QtMocHelpers::MethodData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Method 'save'
        QtMocHelpers::MethodData<bool()>(13, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'validate'
        QtMocHelpers::MethodData<bool()>(14, 2, QMC::AccessPublic, QMetaType::Bool),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'isEditMode'
        QtMocHelpers::PropertyData<bool>(15, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'courseId'
        QtMocHelpers::PropertyData<QString>(7, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'name'
        QtMocHelpers::PropertyData<QString>(16, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'teacher'
        QtMocHelpers::PropertyData<QString>(17, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'location'
        QtMocHelpers::PropertyData<QString>(18, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'startWeek'
        QtMocHelpers::PropertyData<int>(19, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'endWeek'
        QtMocHelpers::PropertyData<int>(20, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'dayOfWeek'
        QtMocHelpers::PropertyData<int>(10, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'startSection'
        QtMocHelpers::PropertyData<int>(11, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'endSection'
        QtMocHelpers::PropertyData<int>(21, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'weekType'
        QtMocHelpers::PropertyData<int>(22, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'colorValue'
        QtMocHelpers::PropertyData<quint32>(23, QMetaType::UInt, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'errorMessage'
        QtMocHelpers::PropertyData<QString>(24, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'sectionsPerDay'
        QtMocHelpers::PropertyData<int>(25, QMetaType::Int, QMC::DefaultPropertyFlags, 3),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CourseEditViewModel, qt_meta_tag_ZN8SCUNexus19CourseEditViewModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SCUNexus::CourseEditViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus19CourseEditViewModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus19CourseEditViewModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8SCUNexus19CourseEditViewModelE_t>.metaTypes,
    nullptr
} };

void SCUNexus::CourseEditViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CourseEditViewModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->modeChanged(); break;
        case 1: _t->courseChanged(); break;
        case 2: _t->errorChanged(); break;
        case 3: _t->configChanged(); break;
        case 4: _t->saved((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->cancelled(); break;
        case 6: _t->initForAdd((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 7: _t->initForEdit((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: { bool _r = _t->save();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 9: { bool _r = _t->validate();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CourseEditViewModel::*)()>(_a, &CourseEditViewModel::modeChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CourseEditViewModel::*)()>(_a, &CourseEditViewModel::courseChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CourseEditViewModel::*)()>(_a, &CourseEditViewModel::errorChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CourseEditViewModel::*)()>(_a, &CourseEditViewModel::configChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CourseEditViewModel::*)(const QString & )>(_a, &CourseEditViewModel::saved, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (CourseEditViewModel::*)()>(_a, &CourseEditViewModel::cancelled, 5))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->isEditMode(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->courseId(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->name(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->teacher(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->location(); break;
        case 5: *reinterpret_cast<int*>(_v) = _t->startWeek(); break;
        case 6: *reinterpret_cast<int*>(_v) = _t->endWeek(); break;
        case 7: *reinterpret_cast<int*>(_v) = _t->dayOfWeek(); break;
        case 8: *reinterpret_cast<int*>(_v) = _t->startSection(); break;
        case 9: *reinterpret_cast<int*>(_v) = _t->endSection(); break;
        case 10: *reinterpret_cast<int*>(_v) = _t->weekType(); break;
        case 11: *reinterpret_cast<quint32*>(_v) = _t->colorValue(); break;
        case 12: *reinterpret_cast<QString*>(_v) = _t->errorMessage(); break;
        case 13: *reinterpret_cast<int*>(_v) = _t->sectionsPerDay(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 2: _t->setName(*reinterpret_cast<QString*>(_v)); break;
        case 3: _t->setTeacher(*reinterpret_cast<QString*>(_v)); break;
        case 4: _t->setLocation(*reinterpret_cast<QString*>(_v)); break;
        case 5: _t->setStartWeek(*reinterpret_cast<int*>(_v)); break;
        case 6: _t->setEndWeek(*reinterpret_cast<int*>(_v)); break;
        case 7: _t->setDayOfWeek(*reinterpret_cast<int*>(_v)); break;
        case 8: _t->setStartSection(*reinterpret_cast<int*>(_v)); break;
        case 9: _t->setEndSection(*reinterpret_cast<int*>(_v)); break;
        case 10: _t->setWeekType(*reinterpret_cast<int*>(_v)); break;
        case 11: _t->setColorValue(*reinterpret_cast<quint32*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *SCUNexus::CourseEditViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SCUNexus::CourseEditViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8SCUNexus19CourseEditViewModelE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SCUNexus::CourseEditViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void SCUNexus::CourseEditViewModel::modeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SCUNexus::CourseEditViewModel::courseChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SCUNexus::CourseEditViewModel::errorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SCUNexus::CourseEditViewModel::configChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SCUNexus::CourseEditViewModel::saved(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void SCUNexus::CourseEditViewModel::cancelled()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP

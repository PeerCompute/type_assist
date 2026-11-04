/****************************************************************************
** Meta object code from reading C++ file 'suggestionpopup.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../suggestionpopup.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'suggestionpopup.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
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
struct qt_meta_tag_ZN15SuggestionPopupE_t {};
} // unnamed namespace

template <> constexpr inline auto SuggestionPopup::qt_create_metaobjectdata<qt_meta_tag_ZN15SuggestionPopupE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SuggestionPopup",
        "suggestionChosen",
        "",
        "word",
        "addToDictionaryRequested",
        "ignoreRequested",
        "ignoreOnceRequested",
        "ignoreAllRequested",
        "mouseEntered",
        "mouseLeft"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'suggestionChosen'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'addToDictionaryRequested'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'ignoreRequested'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'ignoreOnceRequested'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'ignoreAllRequested'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'mouseEntered'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'mouseLeft'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SuggestionPopup, qt_meta_tag_ZN15SuggestionPopupE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SuggestionPopup::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15SuggestionPopupE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15SuggestionPopupE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15SuggestionPopupE_t>.metaTypes,
    nullptr
} };

void SuggestionPopup::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SuggestionPopup *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->suggestionChosen((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->addToDictionaryRequested(); break;
        case 2: _t->ignoreRequested(); break;
        case 3: _t->ignoreOnceRequested(); break;
        case 4: _t->ignoreAllRequested(); break;
        case 5: _t->mouseEntered(); break;
        case 6: _t->mouseLeft(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SuggestionPopup::*)(const QString & )>(_a, &SuggestionPopup::suggestionChosen, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SuggestionPopup::*)()>(_a, &SuggestionPopup::addToDictionaryRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SuggestionPopup::*)()>(_a, &SuggestionPopup::ignoreRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SuggestionPopup::*)()>(_a, &SuggestionPopup::ignoreOnceRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (SuggestionPopup::*)()>(_a, &SuggestionPopup::ignoreAllRequested, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (SuggestionPopup::*)()>(_a, &SuggestionPopup::mouseEntered, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (SuggestionPopup::*)()>(_a, &SuggestionPopup::mouseLeft, 6))
            return;
    }
}

const QMetaObject *SuggestionPopup::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SuggestionPopup::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15SuggestionPopupE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SuggestionPopup::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void SuggestionPopup::suggestionChosen(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void SuggestionPopup::addToDictionaryRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SuggestionPopup::ignoreRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SuggestionPopup::ignoreOnceRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SuggestionPopup::ignoreAllRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SuggestionPopup::mouseEntered()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SuggestionPopup::mouseLeft()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP

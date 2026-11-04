/****************************************************************************
** Meta object code from reading C++ file 'textedit.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../textedit.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'textedit.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8TextEditE_t {};
} // unnamed namespace

template <> constexpr inline auto TextEdit::qt_create_metaobjectdata<qt_meta_tag_ZN8TextEditE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TextEdit",
        "statusMessage",
        "",
        "msg",
        "wordAddedToDictionary",
        "word",
        "zoomChanged",
        "percent",
        "cursorInfoChanged",
        "line",
        "col",
        "wordCount",
        "charCount",
        "onSuggestionChosen",
        "onHoverTimeout",
        "onCloseTimeout",
        "updateCursorInfo"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'statusMessage'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'wordAddedToDictionary'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Signal 'zoomChanged'
        QtMocHelpers::SignalData<void(int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 },
        }}),
        // Signal 'cursorInfoChanged'
        QtMocHelpers::SignalData<void(int, int, int, int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 10 }, { QMetaType::Int, 11 }, { QMetaType::Int, 12 },
        }}),
        // Slot 'onSuggestionChosen'
        QtMocHelpers::SlotData<void(const QString &)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Slot 'onHoverTimeout'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCloseTimeout'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateCursorInfo'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TextEdit, qt_meta_tag_ZN8TextEditE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TextEdit::staticMetaObject = { {
    QMetaObject::SuperData::link<QTextEdit::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8TextEditE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8TextEditE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8TextEditE_t>.metaTypes,
    nullptr
} };

void TextEdit::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TextEdit *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->statusMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->wordAddedToDictionary((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->zoomChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->cursorInfoChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4]))); break;
        case 4: _t->onSuggestionChosen((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onHoverTimeout(); break;
        case 6: _t->onCloseTimeout(); break;
        case 7: _t->updateCursorInfo(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TextEdit::*)(const QString & )>(_a, &TextEdit::statusMessage, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEdit::*)(const QString & )>(_a, &TextEdit::wordAddedToDictionary, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEdit::*)(int )>(_a, &TextEdit::zoomChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEdit::*)(int , int , int , int )>(_a, &TextEdit::cursorInfoChanged, 3))
            return;
    }
}

const QMetaObject *TextEdit::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TextEdit::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8TextEditE_t>.strings))
        return static_cast<void*>(this);
    return QTextEdit::qt_metacast(_clname);
}

int TextEdit::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void TextEdit::statusMessage(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void TextEdit::wordAddedToDictionary(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void TextEdit::zoomChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void TextEdit::cursorInfoChanged(int _t1, int _t2, int _t3, int _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2, _t3, _t4);
}
QT_WARNING_POP

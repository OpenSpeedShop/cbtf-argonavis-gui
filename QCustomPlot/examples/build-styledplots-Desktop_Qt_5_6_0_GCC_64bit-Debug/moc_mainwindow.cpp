/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.6.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../styledplots/mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.6.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[29];
    char stringdata0[365];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 16), // "titleDoubleClick"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 12), // "QMouseEvent*"
QT_MOC_LITERAL(4, 42, 5), // "event"
QT_MOC_LITERAL(5, 48, 13), // "QCPPlotTitle*"
QT_MOC_LITERAL(6, 62, 5), // "title"
QT_MOC_LITERAL(7, 68, 20), // "axisLabelDoubleClick"
QT_MOC_LITERAL(8, 89, 8), // "QCPAxis*"
QT_MOC_LITERAL(9, 98, 4), // "axis"
QT_MOC_LITERAL(10, 103, 23), // "QCPAxis::SelectablePart"
QT_MOC_LITERAL(11, 127, 4), // "part"
QT_MOC_LITERAL(12, 132, 17), // "legendDoubleClick"
QT_MOC_LITERAL(13, 150, 10), // "QCPLegend*"
QT_MOC_LITERAL(14, 161, 6), // "legend"
QT_MOC_LITERAL(15, 168, 22), // "QCPAbstractLegendItem*"
QT_MOC_LITERAL(16, 191, 4), // "item"
QT_MOC_LITERAL(17, 196, 16), // "selectionChanged"
QT_MOC_LITERAL(18, 213, 10), // "mousePress"
QT_MOC_LITERAL(19, 224, 10), // "mouseWheel"
QT_MOC_LITERAL(20, 235, 14), // "addRandomGraph"
QT_MOC_LITERAL(21, 250, 19), // "removeSelectedGraph"
QT_MOC_LITERAL(22, 270, 15), // "removeAllGraphs"
QT_MOC_LITERAL(23, 286, 18), // "contextMenuRequest"
QT_MOC_LITERAL(24, 305, 3), // "pos"
QT_MOC_LITERAL(25, 309, 10), // "moveLegend"
QT_MOC_LITERAL(26, 320, 12), // "graphClicked"
QT_MOC_LITERAL(27, 333, 21), // "QCPAbstractPlottable*"
QT_MOC_LITERAL(28, 355, 9) // "plottable"

    },
    "MainWindow\0titleDoubleClick\0\0QMouseEvent*\0"
    "event\0QCPPlotTitle*\0title\0"
    "axisLabelDoubleClick\0QCPAxis*\0axis\0"
    "QCPAxis::SelectablePart\0part\0"
    "legendDoubleClick\0QCPLegend*\0legend\0"
    "QCPAbstractLegendItem*\0item\0"
    "selectionChanged\0mousePress\0mouseWheel\0"
    "addRandomGraph\0removeSelectedGraph\0"
    "removeAllGraphs\0contextMenuRequest\0"
    "pos\0moveLegend\0graphClicked\0"
    "QCPAbstractPlottable*\0plottable"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   74,    2, 0x08 /* Private */,
       7,    2,   79,    2, 0x08 /* Private */,
      12,    2,   84,    2, 0x08 /* Private */,
      17,    0,   89,    2, 0x08 /* Private */,
      18,    0,   90,    2, 0x08 /* Private */,
      19,    0,   91,    2, 0x08 /* Private */,
      20,    0,   92,    2, 0x08 /* Private */,
      21,    0,   93,    2, 0x08 /* Private */,
      22,    0,   94,    2, 0x08 /* Private */,
      23,    1,   95,    2, 0x08 /* Private */,
      25,    0,   98,    2, 0x08 /* Private */,
      26,    1,   99,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 10,    9,   11,
    QMetaType::Void, 0x80000000 | 13, 0x80000000 | 15,   14,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   24,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 27,   28,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        MainWindow *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->titleDoubleClick((*reinterpret_cast< QMouseEvent*(*)>(_a[1])),(*reinterpret_cast< QCPPlotTitle*(*)>(_a[2]))); break;
        case 1: _t->axisLabelDoubleClick((*reinterpret_cast< QCPAxis*(*)>(_a[1])),(*reinterpret_cast< QCPAxis::SelectablePart(*)>(_a[2]))); break;
        case 2: _t->legendDoubleClick((*reinterpret_cast< QCPLegend*(*)>(_a[1])),(*reinterpret_cast< QCPAbstractLegendItem*(*)>(_a[2]))); break;
        case 3: _t->selectionChanged(); break;
        case 4: _t->mousePress(); break;
        case 5: _t->mouseWheel(); break;
        case 6: _t->addRandomGraph(); break;
        case 7: _t->removeSelectedGraph(); break;
        case 8: _t->removeAllGraphs(); break;
        case 9: _t->contextMenuRequest((*reinterpret_cast< QPoint(*)>(_a[1]))); break;
        case 10: _t->moveLegend(); break;
        case 11: _t->graphClicked((*reinterpret_cast< QCPAbstractPlottable*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QCPPlotTitle* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QCPAxis* >(); break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QCPAxis::SelectablePart >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QCPAbstractLegendItem* >(); break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QCPLegend* >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QCPAbstractPlottable* >(); break;
            }
            break;
        }
    }
}

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow.data,
      qt_meta_data_MainWindow,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}
QT_END_MOC_NAMESPACE

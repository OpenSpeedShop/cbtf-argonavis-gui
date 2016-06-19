#-------------------------------------------------
#
# Project created by QtCreator 2016-03-30T18:41:01
#
#-------------------------------------------------

!greaterThan(QT_VERSION, "4.6.2"): error("Minimum required version of Qt is 4.6.3")

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent printsupport

CONFIG += c++11

TARGET = openss-gui
TEMPLATE = app

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
}
CONFIG(release, debug|release) {
    DESTDIR = build/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

KRELL_ROOT = /devel/DEVEL/krellroot_v2.2.2
CBTF_ROOT = /devel/DEVEL/cbtf_v2.2.2
OSS_CBTF_ROOT = /devel/DEVEL/osscbtf_v2.2.2
BOOST_ROOT = $$KRELL_ROOT
PYTHON_ROOT = $$KRELL_ROOT

LIBS += -L$$KRELL_ROOT/lib64
LIBS += -Wl,-rpath $$KRELL_ROOT/lib64
LIBS += -lxerces-c-3.1 -lxplat -lmrnet

INCLUDEPATH += $$CBTF_ROOT/include
LIBS += -L$$CBTF_ROOT/lib64
LIBS += -largonavis-base -largonavis-cuda

CBTF_MESSAGES_PATH = $$CBTF_ROOT
include(CBTF-Messages.pri)

LIBOPENSS_INC = $$OSS_CBTF_ROOT
OPENSS_PATH = $$OSS_CBTF_ROOT
include(OpenSS-CLI.pri)
LIBS += -Wl,-rpath $$OSS_CBTF_ROOT/lib64

#DEFINES += USE_DISCRETE_SAMPLES
DEFINES += USE_PERIODIC_SAMPLE_AVG
DEFINES += HAS_PARALLEL_PROCESS_METRIC_VIEW
#DEFINES += HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG
#DEFINES += HAS_DESTROY_SINGLETONS
#DEFINES += HAS_ITEM_CLICK_DEBUG
#DEFINES += HAS_PROCESS_METRIC_VIEW_DEBUG

INCLUDEPATH += $$BOOST_ROOT/include $$BOOST_ROOT/include/boost
LIBS += -L$$BOOST_ROOT/lib -lboost_system -lboost_program_options -lboost_thread -lboost_date_time -lboost_filesystem -lboost_unit_test_framework
LIBS += -Wl,-rpath $$BOOST_ROOT/lib
LIBS += -lgomp

QCUSTOMPLOTDIR = $$PWD/QCustomPlot
INCLUDEPATH += $$QCUSTOMPLOTDIR


SOURCES += \
    QCustomPlot/qcustomplot.cpp \
    QCustomPlot/CustomPlot.cpp \
    main/main.cpp \
    main/MainWindow.cpp \
    widgets/TreeItem.cpp \
    widgets/TreeModel.cpp \
    widgets/ExperimentPanel.cpp \
    graphitems/OSSDataTransferItem.cpp \
    graphitems/OSSKernelExecutionItem.cpp \
    graphitems/OSSEventItem.cpp \
    graphitems/OSSPeriodicSampleItem.cpp \
    managers/PerformanceDataManager.cpp \
    widgets/PerformanceDataPlotView.cpp \
    widgets/PerformanceDataMetricView.cpp

greaterThan(QT_MAJOR_VERSION, 4): {
DEFINES += HAS_OSSCUDA2XML
SOURCES += \
    util/osscuda2xml.cxx \
}

HEADERS += \
    QCustomPlot/qcustomplot.h \
    QCustomPlot/CustomPlot.h \
    main/MainWindow.h \
    widgets/TreeItem.h \
    widgets/TreeModel.h \
    widgets/ExperimentPanel.h \
    graphitems/OSSDataTransferItem.h \
    graphitems/OSSKernelExecutionItem.h \
    graphitems/OSSEventItem.h \
    graphitems/OSSPeriodicSampleItem.h \
    managers/PerformanceDataManager.h \
    widgets/PerformanceDataPlotView.h \
    widgets/PerformanceDataMetricView.h \
    common/openss-gui-config.h

FORMS += main/mainwindow.ui \
    widgets/PerformanceDataPlotView.ui \
    widgets/PerformanceDataMetricView.ui

RESOURCES += \
    openss-gui.qrc

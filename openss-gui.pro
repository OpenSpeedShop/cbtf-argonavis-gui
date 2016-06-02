#-------------------------------------------------
#
# Project created by QtCreator 2016-03-30T18:41:01
#
#-------------------------------------------------

QT += core gui concurrent
QT += printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = openss-gui
TEMPLATE = app

KRELL_ROOT = /opt/DEVEL/krellroot_v2.2.2
CBTF_ROOT = /opt/DEVEL/cbtf_v2.2.2
OSS_CBTF_ROOT = /opt/DEVEL/osscbtf_v2.2.2
BOOST_ROOT = /opt/boost

LIBS += -L$$KRELL_ROOT/lib64
LIBS += -lxerces-c-3.1 -lxplat -lmrnet

INCLUDEPATH += $$CBTF_ROOT/include
LIBS += -L$$CBTF_ROOT/lib64
LIBS += -largonavis-base -largonavis-cuda

CBTF_MESSAGES_PATH = $$CBTF_ROOT
include(CBTF-Messages.pri)

LIBOPENSS_INC = $$OSS_CBTF_ROOT
OPENSS_PATH = $$OSS_CBTF_ROOT
include(OpenSS-CLI.pri)

#DEFINES += USE_DISCRETE_SAMPLES
DEFINES += USE_PERIODIC_SAMPLE_AVG
#DEFINES += HAS_DESTROY_SINGLETONS
#DEFINES += HAS_ITEM_CLICK_DEBUG
#DEFINES += HAS_PROCESS_METRIC_VIEW_DEBUG

INCLUDEPATH += $$BOOST_ROOT/include $$BOOST_ROOT/include/boost
LIBS += -L/opt/boost/lib -lboost_system-mt -lboost_program_options-mt

QCUSTOMPLOTDIR = $$PWD/QCustomPlot
INCLUDEPATH += $$QCUSTOMPLOTDIR


SOURCES += \
    QCustomPlot/qcustomplot.cpp \
    main/main.cpp \
    main/MainWindow.cpp \
    widgets/TreeItem.cpp \
    widgets/TreeModel.cpp \
    widgets/ExperimentPanel.cpp \
    graphitems/OSSDataTransferItem.cpp \
    graphitems/OSSKernelExecutionItem.cpp \
    graphitems/OSSEventItem.cpp \
    util/osscuda2xml.cxx \
    graphitems/OSSPeriodicSampleItem.cpp \
    managers/PerformanceDataManager.cpp \
    widgets/PerformanceDataPlotView.cpp \
    widgets/PerformanceDataMetricView.cpp \
    managers/LoadExperimentTaskWatcher.cpp

HEADERS += \
    QCustomPlot/qcustomplot.h \
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
    managers/LoadExperimentTaskWatcher.h

FORMS += main/mainwindow.ui \
    widgets/PerformanceDataPlotView.ui \
    widgets/PerformanceDataMetricView.ui

RESOURCES += \
    openss-gui.qrc

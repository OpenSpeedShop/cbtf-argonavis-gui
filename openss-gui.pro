#-------------------------------------------------
#
# Project created by QtCreator 2016-03-30T18:41:01
#
#-------------------------------------------------

# NOTE: previous minimum required version was Qt 4.6.3 and may still work but build has not been recently tested
!greaterThan(QT_VERSION, "4.8.4"): error("Minimum required version of Qt is 4.8.5")

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent printsupport

DEFINES += APP_MAJOR_VERSION=1
DEFINES += APP_MINOR_VERSION=3
DEFINES += APP_SUBMINOR_VERSION=2
DEFINES += APP_BUILD_VERSION=0000

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
else: QMAKE_CXXFLAGS += -std=c++11

# comment out when need to look for deprecated declarations within this source-code
QMAKE_CXXFLAGS += -Wno-deprecated-declarations
# comment out when need to observe the occurrence of these types of warnings
QMAKE_CXXFLAGS += -Wno-unused-local-typedefs -Wno-placement-new -Wno-implicit-fallthrough

TARGET = openss-gui
TEMPLATE = app

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
}
CONFIG(release, debug|release) {
    DESTDIR = build/release
    DEFINES += QT_NO_DEBUG_OUTPUT
}

BUILD = $$(BUILD)

# BUILD should be automatically set to x86 or x86_64 unless passed in by user.
isEmpty(BUILD) {
    BUILD = $$QMAKE_HOST.arch
}

contains(BUILD, x86_64) {
    BUILDLIB = lib64
    ALT_BUILDLIB = lib
}
else {
    BUILDLIB = lib
    ALT_BUILDLIB = lib64
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

KRELL_ROOT_MRNET = $$(KRELL_ROOT_MRNET)
KRELL_ROOT_XERCES = $$(KRELL_ROOT_XERCES)
CBTF_ROOT = $$(CBTF_ROOT)
CBTF_KRELL_ROOT = $$(CBTF_KRELL_ROOT)
CBTF_ARGONAVIS_ROOT = $$(CBTF_ARGONAVIS_ROOT)
OSS_CBTF_ROOT = $$(OSS_CBTF_ROOT)
GRAPHVIZ_ROOT = $$(GRAPHVIZ_ROOT)
QTGRAPHLIB_ROOT = $$(QTGRAPHLIB_ROOT)
BOOST_ROOT = $$(BOOST_ROOT)
BOOST_LIB_DIR = $$(BOOST_LIB_DIR)

CONFIG_H = $$quote(common/config.h)

config.target = $$CONFIG_H
config.commands = $$system("sh ./generate-config.sh $$OSS_CBTF_ROOT $$CONFIG_H")

QMAKE_EXTRA_TARGETS += $$CONFIG_H

PRE_TARGETDEPS += $$CONFIG_H

target.path = $$(INSTALL_PATH)/bin
INSTALLS += target

exists($$KRELL_ROOT_XERCES/$$ALT_BUILDLIB): LIBS += -L$$KRELL_ROOT_XERCES/$$ALT_BUILDLIB
exists($$KRELL_ROOT_XERCES/$$BUILDLIB): LIBS += -L$$KRELL_ROOT_XERCES/$$BUILDLIB
exists($$KRELL_ROOT_XERCES/$$ALT_BUILDLIB): LIBS += -Wl,-rpath $$KRELL_ROOT_XERCES/$$ALT_BUILDLIB
exists($$KRELL_ROOT_XERCES/$$BUILDLIB): LIBS += -Wl,-rpath $$KRELL_ROOT_XERCES/$$BUILDLIB
LIBS += -lxerces-c-3.1

exists($$KRELL_ROOT_MRNET/$$ALT_BUILDLIB): LIBS += -L$$KRELL_ROOT_MRNET/$$ALT_BUILDLIB
exists($$KRELL_ROOT_MRNET/$$BUILDLIB): LIBS += -L$$KRELL_ROOT_MRNET/$$BUILDLIB
exists($$KRELL_ROOT_MRNET/$$ALT_BUILDLIB): LIBS += -Wl,-rpath $$KRELL_ROOT_MRNET/$$ALT_BUILDLIB
exists($$KRELL_ROOT_MRNET/$$BUILDLIB): LIBS += -Wl,-rpath $$KRELL_ROOT_MRNET/$$BUILDLIB
LIBS += -lmrnet -lxplat

INCLUDEPATH += $$CBTF_ROOT/include
INCLUDEPATH += $$CBTF_KRELL_ROOT/include
INCLUDEPATH += $$CBTF_ARGONAVIS_ROOT/include

LIBS += -L$$CBTF_ARGONAVIS_ROOT/$$BUILDLIB
LIBS += -largonavis-base -largonavis-cuda

CBTF_MESSAGES_PATH = $$CBTF_ROOT
CBTF_KRELL_MESSAGES_PATH = $$CBTF_KRELL_ROOT
CBTF_ARGONAVIS_MESSAGES_PATH = $$CBTF_ARGONAVIS_ROOT
include(CBTF-Messages.pri)

LIBOPENSS_INC = $$OSS_CBTF_ROOT
OPENSS_PATH = $$OSS_CBTF_ROOT
include(OpenSS-CLI.pri)

INCLUDEPATH += $$CBTF_KRELL_ROOT/include/collectors

DEFINES += HAS_METRIC_TYPES
#DEFINES += HAS_RESAMPLED_COUNTERS
#DEFINES += USE_DISCRETE_SAMPLES
#DEFINES += USE_PERIODIC_SAMPLE_AVG
DEFINES += HAS_PARALLEL_PROCESS_METRIC_VIEW
#DEFINES += HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG
DEFINES += HAS_DESTROY_SINGLETONS
#DEFINES += HAS_ITEM_CLICK_DEBUG
#DEFINES += HAS_PROCESS_METRIC_VIEW_DEBUG
DEFINES += HAS_STRIP_DOMAIN_NAME
#DEFINES += HAS_REAL_SAMPLE_COUNTER_NAME
greaterThan(QT_MAJOR_VERSION, 4) {
# uncommenting this is experimental
# NOTE: For now only allow for Qt5
#DEFINES += HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE
}
DEFINES += HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
#DEFINES += HAS_TIMER_THREAD_DESTROYED_CHECKING
#DEFINES += HAS_PROCESS_EVENT_DEBUG
#DEFINES += HAS_TEST_DATA_RANGE_CONSTRAINT
DEFINES += HAS_SOURCE_CODE_LINE_HIGHLIGHTS
DEFINES += HAS_STACKED_BAR_GRAPHS

message("BOOST_ROOT="$$BOOST_ROOT)
INCLUDEPATH += $$BOOST_ROOT/include
exists($$BOOST_LIB_DIR): LIBS += -L$$BOOST_LIB_DIR/lib
else: LIBS += -L$$BOOST_ROOT/lib
LIBS += -lboost_system -lboost_thread -lboost_program_options
message("LIBS="$$LIBS)
LIBS += -lboost_date_time -lboost_filesystem -lboost_unit_test_framework
LIBS += -lgomp

# select QCustomPlot version
QCUSTOMPLOTVER = 1.3.2
#QCUSTOMPLOTVER = 2.0.0

equals(QCUSTOMPLOTVER, 2.0.0) {
DEFINES += HAS_QCUSTOMPLOT_V2
greaterThan(QT_MAJOR_VERSION, 4): DEFINES += QCUSTOMPLOT_USE_OPENGL
}

QCUSTOMPLOTDIR = $$PWD/QCustomPlot/$$QCUSTOMPLOTVER
INCLUDEPATH += $$QCUSTOMPLOTDIR

INCLUDEPATH += $$GRAPHVIZ_ROOT/include/graphviz
LIBS += -L$$GRAPHVIZ_ROOT/lib -lcdt -lgvc -lcgraph

INCLUDEPATH += $$QTGRAPHLIB_ROOT/include
CONFIG(debug, debug|release) {
#LIBS += -L$$QTGRAPHLIB_ROOT/lib64/$$QT_VERSION -lQtGraphd
#LIBS += -L$$QTGRAPHLIB_ROOT/lib64 -lQtGraphd
exists($$QTGRAPHLIB_ROOT/$$ALT_BUILDLIB/$$QT_VERSION): LIBS += -L$$QTGRAPHLIB_ROOT/$$ALT_BUILDLIB/$$QT_VERSION -lQtGraphd
exists($$QTGRAPHLIB_ROOT/$$BUILDLIB/$$QT_VERSION): LIBS += -L$$QTGRAPHLIB_ROOT/$$BUILDLIB/$$QT_VERSION -lQtGraphd
}
CONFIG(release, debug|release) {
#LIBS += -L$$QTGRAPHLIB_ROOT/lib64/$$QT_VERSION -lQtGraph
#LIBS += -L$$QTGRAPHLIB_ROOT/lib64 -lQtGraph
exists($$QTGRAPHLIB_ROOT/$$ALT_BUILDLIB/$$QT_VERSION): LIBS += -L$$QTGRAPHLIB_ROOT/$$ALT_BUILDLIB/$$QT_VERSION -lQtGraph
exists($$QTGRAPHLIB_ROOT/$$BUILDLIB/$$QT_VERSION): LIBS += -L$$QTGRAPHLIB_ROOT/$$BUILDLIB/$$QT_VERSION -lQtGraph
}

message("LD_LIBRARY_PATH="$$LD_LIBRARY_PATH)

SOURCES += \
    QCustomPlot/$$QCUSTOMPLOTVER/qcustomplot.cpp \
    QCustomPlot/CustomPlot.cpp \
    main/main.cpp \
    main/MainWindow.cpp \
    graphitems/OSSDataTransferItem.cpp \
    graphitems/OSSKernelExecutionItem.cpp \
    graphitems/OSSEventItem.cpp \
    graphitems/OSSPeriodicSampleItem.cpp \
    graphitems/OSSEventsSummaryItem.cpp \
    graphitems/OSSTraceItem.cpp \
    widgets/TreeItem.cpp \
    widgets/TreeModel.cpp \
    widgets/ExperimentPanel.cpp \
    widgets/PerformanceDataMetricView.cpp \
    SourceView/SourceView.cpp \
    SourceView/SyntaxHighlighter.cpp \
    managers/PerformanceDataManager.cpp \
    managers/BackgroundGraphRendererBackend.cpp \
    managers/BackgroundGraphRenderer.cpp \
    managers/UserGraphRangeChangeManager.cpp \
    SourceView/ModifyPathSubstitutionsDialog.cpp \
    CBTF-ArgoNavis-Ext/DataTransferDetails.cpp \
    CBTF-ArgoNavis-Ext/KernelExecutionDetails.cpp \
    widgets/ViewSortFilterProxyModel.cpp \
    CBTF-ArgoNavis-Ext/ClusterNameBuilder.cpp \
    managers/CalltreeGraphManager.cpp \
    widgets/CalltreeGraphView.cpp \
    widgets/MetricViewManager.cpp \
    widgets/MetricViewDelegate.cpp \
    managers/ApplicationOverrideCursorManager.cpp \
    widgets/ShowDeviceDetailsDialog.cpp \
    CBTF-ArgoNavis-Ext/CudaDeviceHelper.cpp \
    widgets/ThreadSelectionCommand.cpp \
    managers/MetricTableViewInfo.cpp \
    SourceView/SourceViewMetricsCache.cpp \
    graphitems/OSSHighlightItem.cpp \
    widgets/MetricViewFilterDialog.cpp \
    widgets/DefaultSortFilterProxyModel.cpp \
    widgets/FilterExpressionValidator.cpp \
    widgets/PerformanceDataTimelineView.cpp \
    widgets/PerformanceDataGraphView.cpp \
    managers/DerivedMetricsSolver.cpp \
    widgets/DerivedMetricInformationDialog.cpp \
    widgets/ConfigureUserDerivedMetricsDialog.cpp \
    widgets/DerivedMetricInformation.cpp

greaterThan(QT_MAJOR_VERSION, 4): {
# uncomment the following to produce XML dump of database
#DEFINES += HAS_OSSCUDA2XML
contains(DEFINES, HAS_OSSCUDA2XML): {
    SOURCES += \
    util/osscuda2xml.cxx \
}
}

HEADERS += \
    common/openss-gui-config.h \
    QCustomPlot/$$QCUSTOMPLOTVER/qcustomplot.h \
    QCustomPlot/CustomPlot.h \
    main/MainWindow.h \
    graphitems/OSSDataTransferItem.h \
    graphitems/OSSKernelExecutionItem.h \
    graphitems/OSSEventItem.h \
    graphitems/OSSPeriodicSampleItem.h \
    graphitems/OSSEventsSummaryItem.h \
    graphitems/OSSTraceItem.h \
    widgets/TreeItem.h \
    widgets/TreeModel.h \
    widgets/ExperimentPanel.h \
    widgets/PerformanceDataMetricView.h \
    SourceView/SourceView.h \
    SourceView/SyntaxHighlighter.h \
    managers/PerformanceDataManager.h \
    managers/BackgroundGraphRendererBackend.h \
    managers/BackgroundGraphRenderer.h \
    managers/UserGraphRangeChangeManager.h \
    SourceView/ModifyPathSubstitutionsDialog.h \
    CBTF-ArgoNavis-Ext/DataTransferDetails.h \
    CBTF-ArgoNavis-Ext/KernelExecutionDetails.h \
    widgets/ViewSortFilterProxyModel.h \
    CBTF-ArgoNavis-Ext/ClusterNameBuilder.h \
    managers/CalltreeGraphManager.h \
    widgets/CalltreeGraphView.h \
    widgets/MetricViewManager.h \
    widgets/MetricViewDelegate.h \
    managers/ApplicationOverrideCursorManager.h \
    widgets/ShowDeviceDetailsDialog.h \
    CBTF-ArgoNavis-Ext/NameValueDefines.h \
    CBTF-ArgoNavis-Ext/CudaDeviceHelper.h \
    widgets/ThreadSelectionCommand.h \
    managers/MetricTableViewInfo.h \
    SourceView/SourceViewMetricsCache.h \
    graphitems/OSSHighlightItem.h \
    widgets/MetricViewFilterDialog.h \
    widgets/DefaultSortFilterProxyModel.h \
    widgets/FilterExpressionValidator.h \
    widgets/PerformanceDataTimelineView.h \
    widgets/PerformanceDataGraphView.h \
    managers/DerivedMetricsSolver.h \
    widgets/DerivedMetricInformationDialog.h \
    widgets/ConfigureUserDerivedMetricsDialog.h \
    widgets/DerivedMetricInformation.h

FORMS += main/mainwindow.ui \
    widgets/PerformanceDataMetricView.ui \
    SourceView/ModifyPathSubstitutionsDialog.ui \
    widgets/MetricViewManager.ui \
    widgets/ShowDeviceDetailsDialog.ui \
    widgets/MetricViewFilterDialog.ui \
    widgets/PerformanceDataTimelineView.ui \
    widgets/PerformanceDataGraphView.ui \
    widgets/DerivedMetricInformationDialog.ui \
    widgets/ConfigureUserDerivedMetricsDialog.ui

RESOURCES += \
    openss-gui.qrc

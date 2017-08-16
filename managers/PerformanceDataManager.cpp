/*!
   \file PerformanceDataManager.cpp
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2016 Argo Navis Technologies, LLC

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by the
   Free Software Foundation; either version 2.1 of the License, or (at your
   option) any later version.

   This library is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
   for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "PerformanceDataManager.h"

#include "common/openss-gui-config.h"

#include "managers/BackgroundGraphRenderer.h"
#include "managers/ApplicationOverrideCursorManager.h"
#include "CBTF-ArgoNavis-Ext/DataTransferDetails.h"
#include "CBTF-ArgoNavis-Ext/KernelExecutionDetails.h"
#include "CBTF-ArgoNavis-Ext/ClusterNameBuilder.h"
#include "CBTF-ArgoNavis-Ext/CudaDeviceHelper.h"

#include <numeric>
#include <utility>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <QApplication>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QThread>
#include <QTimer>
#include <QMap>
#include <qmath.h>

#include <ArgoNavis/CUDA/PerformanceData.hpp>
#include <ArgoNavis/CUDA/DataTransfer.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>
#include <ArgoNavis/CUDA/stringify.hpp>
#include <ArgoNavis/CUDA/CounterKind.hpp>

#include "collectors/usertime/UserTimeDetail.hxx"
#include "collectors/cuda/CUDAExecDetail.hxx"
#include "collectors/mpi/MPIDetail.hxx"
#include "collectors/pthreads/PthreadsDetail.hxx"
#include "collectors/omptp/OmptPDetail.hxx"
#include "collectors/mpit/MPITDetail.hxx"
#include "collectors/mpip/MPIPDetail.hxx"
#include "collectors/io/IODetail.hxx"
#include "collectors/iop/IOPDetail.hxx"
#include "collectors/iot/IOTDetail.hxx"
#include "collectors/hwctime/HWTimeDetail.hxx"

#include "ToolAPI.hxx"
#include "Queries.hxx"
#include "CUDAQueries.hxx"

using namespace OpenSpeedShop;
using namespace OpenSpeedShop::Framework;
using namespace OpenSpeedShop::Queries;

#if defined(HAS_OSSCUDA2XML)
extern int cuda2xml(const QString& dbFilename, QTextStream& xml);
#endif

Q_DECLARE_METATYPE( ArgoNavis::Base::Time )
Q_DECLARE_METATYPE( ArgoNavis::CUDA::DataTransfer )
Q_DECLARE_METATYPE( ArgoNavis::CUDA::KernelExecution )


namespace ArgoNavis { namespace GUI {


QString PerformanceDataManager::s_percentageTitle( tr("% of Time") );
QString PerformanceDataManager::s_timeTitle( tr("Time (msec)") );
QString PerformanceDataManager::s_functionTitle( tr("Function (defining location)") );
QString PerformanceDataManager::s_minimumTitle( tr("Minimum (msec)") );
QString PerformanceDataManager::s_minimumThreadTitle( tr("Minimum (name)") );
QString PerformanceDataManager::s_maximumTitle( tr("Maximum (msec)") );
QString PerformanceDataManager::s_maximumThreadTitle( tr("Maximum (name)") );
QString PerformanceDataManager::s_meanTitle( tr("Average (msec)") );
QString PerformanceDataManager::s_meanThreadTitle( tr("Thread Nearest Avg (name)") );

// define list of supported trace experiment types
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
QStringList PerformanceDataManager::s_TRACING_EXPERIMENTS = { "mpit" };
#else
QStringList PerformanceDataManager::s_TRACING_EXPERIMENTS = QStringList() << "mpit";
#endif

// define list of supported sampling experiment types
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
QStringList PerformanceDataManager::s_SAMPLING_EXPERIMENTS = { "hwctime" };
#else
QStringList PerformanceDataManager::s_SAMPLING_EXPERIMENTS = QStringList() << "hwctime";
#endif

const QString CUDA_EVENT_DETAILS_METRIC = QStringLiteral( "Details" );
const QString TRACE_EVENT_DETAILS_METRIC = QStringLiteral( "Trace" );
const QString ALL_EVENTS_DETAILS_VIEW = QStringLiteral( "All Events" );
const QString KERNEL_EXECUTION_DETAILS_VIEW = QStringLiteral( "Kernel Execution" );
const QString DATA_TRANSFER_DETAILS_VIEW = QStringLiteral( "Data Transfer" );
const QString TIME_METRIC = QStringLiteral( "time" );
const QString DETAIL_METRIC = QStringLiteral( "detail" );

QAtomicPointer< PerformanceDataManager > PerformanceDataManager::s_instance;

#if defined(HAS_OSSCUDA2XML)
/**
 * @brief PerformanceDataManager::xmlDump
 * @param filePath  Filename path to experiment database file with CUDA data collection (.openss file)
 *
 * Generate a XML formatted dump of the experiment database.  Will write output to a file located in the directory
 * where the experiment database is located having the same name as the experiment database with ".xml" suffix.
 */
void PerformanceDataManager::xmlDump(const QString &filePath)
{
    QString xmlFilename( filePath + ".xml" );
    QFile file( xmlFilename );
    if ( file.open( QFile::WriteOnly | QFile::Truncate ) ) {
        QTextStream xml( &file );
        cuda2xml( filePath, xml );
        file.close();
    }
}
#endif

/**
 * @brief Get_Subextents_To_Object
 * @param tgrp - the set of threads
 * @param object - the view object (Function, Statements, LinkedObject, Loops, etc)
 * @param subextents - the extents in the view object for the entire set of threads
 *
 * Return the extents in the view object for the entire set of threads.
 */
template <typename TS>
void Get_Subextents_To_Object (
        const Framework::ThreadGroup& tgrp,
        TS& object,
        Framework::ExtentGroup& subextents)
{
    for (ThreadGroup::iterator ti = tgrp.begin(); ti != tgrp.end(); ti++) {
        ExtentGroup newExtents = object.getExtentIn (*ti);
        for (ExtentGroup::iterator ei = newExtents.begin(); ei != newExtents.end(); ei++) {
            subextents.push_back(*ei);
        }
    }
}

/**
 * @brief Get_Subextents_To_Object_Map
 * @param tgrp - the set of threads
 * @param object - the view object (Function, Statements, LinkedObject, Loops, etc)
 * @param subextents_map - a map for each thread to its extents in the view object
 *
 * Return the map for each thread to its extents in the view object.
 */
template <typename TS>
void Get_Subextents_To_Object_Map (
        const Framework::ThreadGroup& tgrp,
        TS& object,
        std::map<Framework::Thread, Framework::ExtentGroup>& subextents_map)
{
    for (ThreadGroup::iterator ti = tgrp.begin(); ti != tgrp.end(); ti++) {
        ExtentGroup newExtents = object.getExtentIn (*ti);
        Framework::ExtentGroup subextents;

        for (ExtentGroup::iterator ei = newExtents.begin(); ei != newExtents.end(); ei++) {
            subextents.push_back(*ei);
        }

        subextents_map[*ti] = subextents;
    }
}

/**
 * @brief stack_contains_N_calls
 * @param st - the stack frame object
 * @param subextents - the relevant extents for the view object and thread(s) desired
 * @return - the number of calls within the extent
 *
 * Determine the number of calls found in the stack frame within the extent.
 */
inline int64_t stack_contains_N_calls(const Framework::StackTrace& st, Framework::ExtentGroup& subextents)
{
    int64_t num_calls = 0;

    Time t = st.getTime();

    for ( ExtentGroup::iterator ei = subextents.begin(); ei != subextents.end(); ei++ ) {
        Extent check( *ei );
        if ( ! check.isEmpty() ) {
            TimeInterval time = check.getTimeInterval();
            AddressRange addr = check.getAddressRange();

            for ( std::size_t sti = 0; sti < st.size(); sti++ ) {
                const Address& a = st[sti];

                if ( time.doesContain(t) && addr.doesContain(a) ) {
                    num_calls++;
                }
            }
        }
    }

    return num_calls;
}

/**
 * @brief The ltST struct provides a Function object (functor) to sort a container of Framework::StackTrace items.
 */
struct ltST {
    bool operator() (Framework::StackTrace stl, Framework::StackTrace str) {
        if (stl.getTime() < str.getTime()) { return true; }
        if (stl.getTime() > str.getTime()) { return false; }

        if (stl.getThread() < str.getThread()) { return true; }
        if (stl.getThread() > str.getThread()) { return false; }

        return stl < str;
    }
};


/**
 * @brief PerformanceDataManager::PerformanceDataManager
 * @param parent - the parent QObject (in any - ie non-NULL)
 *
 * Private constructor to construct a singleton PerformanceDataManager instance.
 */
PerformanceDataManager::PerformanceDataManager(QObject *parent)
    : QObject( parent )
    , m_renderer( new BackgroundGraphRenderer )
{
    qRegisterMetaType< Base::Time >("Base::Time");
    qRegisterMetaType< CUDA::DataTransfer >("CUDA::DataTransfer");
    qRegisterMetaType< CUDA::KernelExecution >("CUDA::KernelExecution");
    qRegisterMetaType< QVector< QString > >("QVector< QString >");
    qRegisterMetaType< QVector< bool > >("QVector< bool >");

#if defined(HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE)
    m_thread.start();
    m_renderer->moveToThread( &m_thread );
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( this, &PerformanceDataManager::loadComplete, m_renderer, &BackgroundGraphRenderer::signalProcessCudaEventView );
    connect( this, &PerformanceDataManager::graphRangeChanged, m_renderer, &BackgroundGraphRenderer::handleGraphRangeChanged );
    connect( this, &PerformanceDataManager::graphRangeChanged, this, &PerformanceDataManager::handleLoadCudaMetricViews );
    connect( m_renderer, &BackgroundGraphRenderer::signalCudaEventSnapshot, this, &PerformanceDataManager::addCudaEventSnapshot );
    connect( &m_userChangeMgr, &UserGraphRangeChangeManager::timeoutGroup, this, &PerformanceDataManager::handleLoadCudaMetricViewsTimeout );
    connect( this, &PerformanceDataManager::signalSelectedClustersChanged, this, &PerformanceDataManager::handleSelectedClustersChanged );
#else
    connect( this, SIGNAL(loadComplete()), m_renderer, SIGNAL(signalProcessCudaEventView()) );
    connect( this, SIGNAL(graphRangeChanged(QString,QString,double,double,QSize)), m_renderer, SLOT(handleGraphRangeChanged(QString,QString,double,double,QSize)) );
    connect( this, SIGNAL(graphRangeChanged(QString,QString,double,double,QSize)), this, SLOT(handleLoadCudaMetricViews(QString,QString,double,double)) );
    connect( m_renderer, SIGNAL(signalCudaEventSnapshot(QString,QString,double,double,QImage)), this, SIGNAL(addCudaEventSnapshot(QString,QString,double,double,QImage)) );
    connect( &m_userChangeMgr, SIGNAL(timeoutGroup(QString,double,double,QSize)), this, SLOT(handleLoadCudaMetricViewsTimeout(QString,double,double)) );
    connect( this, SIGNAL(signalSelectedClustersChanged(QString,QSet<QString>)), this, SLOT(handleSelectedClustersChanged(QString,QSet<QString>)) );
#endif
}

/**
 * @brief PerformanceDataManager::~PerformanceDataManager
 *
 * Destroys the PerformanceDataManager instance.
 */
PerformanceDataManager::~PerformanceDataManager()
{
#if defined(HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE)
    m_thread.quit();
    m_thread.wait();
#endif
    delete m_renderer;
}

/**
 * @brief PerformanceDataManager::instance
 * @return - return a pointer to the singleton instance
 *
 * This method provides a pointer to the singleton instance.
 */
PerformanceDataManager *PerformanceDataManager::instance()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    PerformanceDataManager* inst = s_instance.loadAcquire();
#else
    PerformanceDataManager* inst = s_instance;
#endif

    if ( ! inst ) {
        inst = new PerformanceDataManager();
        if ( ! s_instance.testAndSetRelease( 0, inst ) ) {
            delete inst;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            inst = s_instance.loadAcquire();
#else
            inst = s_instance;
#endif
        }
    }

    return inst;
}

/**
 * @brief PerformanceDataManager::destroy
 *
 * Static method to destroy the singleton instance.
 */
void PerformanceDataManager::destroy()
{
    if ( s_instance )
        delete s_instance;

    s_instance = Q_NULLPTR;
}

/**
 * @brief PerformanceDataManager::handleRequestMetricView
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 *
 * Handler for external request to produce metric view data for specified metric view.
 */
void PerformanceDataManager::handleRequestMetricView(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName)
{
    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) || metricName.isEmpty() || viewName.isEmpty() )
        return;

#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleRequestMetricView: clusteringCriteriaName=" << clusteringCriteriaName << "metric=" << metricName << "view=" << viewName;
#endif

    MetricTableViewInfo& info = m_tableViewInfo[ clusteringCriteriaName ];

    const QString metricViewName = metricName + "-" + viewName;

    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QString("generate-%1").arg(metricViewName) );
    }

    info.addMetricView( metricViewName );

    QFutureSynchronizer<void> synchronizer;
    QMap< QString, QFuture<void> > futures;

    const CollectorGroup collectors = info.getCollectors();
    const ThreadGroup all_threads = info.getThreads();
    const TimeInterval interval = info.getInterval();

    if ( collectors.size() > 0 ) {
        const Collector& collector( *collectors.begin() );
        const QString collectorId( collector.getMetadata().getUniqueId().c_str() );

        if ( s_SAMPLING_EXPERIMENTS.contains( collectorId ) ) {
            QFuture<void> future = QtConcurrent::run( this, &PerformanceDataManager::handleRequestSampleCountersView, clusteringCriteriaName, metricName, viewName );
            synchronizer.addFuture( future );
        }
        else {
            loadCudaMetricViews(synchronizer,
                                futures,
                                clusteringCriteriaName,
                                QStringList() << metricName,
                                QStringList() << viewName,
                                collectors,
                                all_threads,
                                interval );
        }

        if ( futures.size() > 0 ) {
            // Determine full time interval extent of this experiment
            Extent extent = info.getExtent();
            Base::TimeInterval experimentInterval = ConvertToArgoNavis( extent.getTimeInterval() );

            Base::TimeInterval graphInterval = ConvertToArgoNavis( interval );

            double lower = ( graphInterval.begin() - experimentInterval.begin() ) / 1000000.0;
            double upper = ( graphInterval.end() - experimentInterval.begin() ) / 1000000.0;

            synchronizer.waitForFinished();

            emit requestMetricViewComplete( clusteringCriteriaName, metricName, viewName, lower, upper );
        }
    }

    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QString("generate-%1").arg(metricViewName) );
    }
}

/**
 * @brief PerformanceDataManager::handleRequestLoadBalanceView
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 *
 * Handler for external request to produce load balance data for the load balance view.
 */
void PerformanceDataManager::handleRequestLoadBalanceView(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName)
{
    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) || metricName.isEmpty() || viewName.isEmpty() )
        return;

    MetricTableViewInfo& info = m_tableViewInfo[ clusteringCriteriaName ];

#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleRequestLoadBalanceView: clusteringCriteriaName=" << clusteringCriteriaName << "metric=" << metricName << "view=" << viewName;
#endif

    const QString metricViewName = metricName + "-" + viewName;
    const QString loadBalanceViewName = QStringLiteral("Load Balance-") + metricViewName;

    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QString("generate-%1").arg(loadBalanceViewName) );
    }

    info.addMetricView( loadBalanceViewName );

    const TimeInterval interval( info.getInterval() );

    if ( viewName == QStringLiteral("Functions") ) {
        processLoadBalanceView<Function>( info.getCollectors(), info.getThreads(), interval, clusteringCriteriaName, metricName );
    }

    else if ( viewName == QStringLiteral("Statements") ) {
        processLoadBalanceView<Statement>( info.getCollectors(), info.getThreads(), interval, clusteringCriteriaName, metricName );
    }

    else if ( viewName == QStringLiteral("LinkedObjects") ) {
        processLoadBalanceView<LinkedObject>( info.getCollectors(), info.getThreads(), interval, clusteringCriteriaName, metricName );
    }

    else if ( viewName == QStringLiteral("Loops") ) {
        processLoadBalanceView<Loop>( info.getCollectors(), info.getThreads(), interval, clusteringCriteriaName, metricName );
    }

    // Determine full time interval extent of this experiment
    Extent extent = info.getExtent();
    Base::TimeInterval experimentInterval = ConvertToArgoNavis( extent.getTimeInterval() );

    Base::TimeInterval graphInterval = ConvertToArgoNavis( interval );

    double lower = ( graphInterval.begin() - experimentInterval.begin() ) / 1000000.0;
    double upper = ( graphInterval.end() - experimentInterval.begin() ) / 1000000.0;

    emit requestMetricViewComplete( clusteringCriteriaName, QStringLiteral("Load Balance"), metricViewName, lower, upper );

    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QString("generate-%1").arg(loadBalanceViewName) );
    }
}

/**
 * @brief PerformanceDataManager::handleRequestCompareView
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param compareMode - the specific compare mode requested
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 *
 * Handler for external request to produce compare view data for specified compare view.
 */
void PerformanceDataManager::handleRequestCompareView(const QString &clusteringCriteriaName, const QString &compareMode, const QString &metricName, const QString &viewName)
{
    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) || compareMode.isEmpty() || metricName.isEmpty() || viewName.isEmpty() )
        return;

    MetricTableViewInfo& info = m_tableViewInfo[ clusteringCriteriaName ];

#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleRequestCompareView: clusteringCriteriaName=" << clusteringCriteriaName << "metric=" << metricName << "view=" << viewName;
#endif

    const QString metricViewName = metricName + "-" + viewName;
    const QString compareViewName = compareMode + QStringLiteral("-") + metricViewName;

    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QString("generate-%1").arg(compareViewName) );
    }

    info.addMetricView( compareViewName );

    const TimeInterval interval( info.getInterval() );

    if ( viewName == QStringLiteral("Functions") ) {
        processCompareThreadView<Function>( info.getCollectors(), info.getThreads(), interval, clusteringCriteriaName, metricName, compareMode );
    }

    else if ( viewName == QStringLiteral("Statements") ) {
        processCompareThreadView<Statement>( info.getCollectors(), info.getThreads(), interval, clusteringCriteriaName, metricName, compareMode );
    }

    else if ( viewName == QStringLiteral("LinkedObjects") ) {
        processCompareThreadView<LinkedObject>( info.getCollectors(), info.getThreads(), interval, clusteringCriteriaName, metricName, compareMode );
    }

    else if ( viewName == QStringLiteral("Loops") ) {
        processCompareThreadView<Loop>( info.getCollectors(), info.getThreads(), interval, clusteringCriteriaName, metricName, compareMode );
    }

    // Determine full time interval extent of this experiment
    Extent extent = info.getExtent();
    Base::TimeInterval experimentInterval = ConvertToArgoNavis( extent.getTimeInterval() );

    Base::TimeInterval graphInterval = ConvertToArgoNavis( interval );

    double lower = ( graphInterval.begin() - experimentInterval.begin() ) / 1000000.0;
    double upper = ( graphInterval.end() - experimentInterval.begin() ) / 1000000.0;

    emit requestMetricViewComplete( clusteringCriteriaName, compareMode, metricViewName, lower, upper );

    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QString("generate-%1").arg(compareViewName) );
    }
}

/**
 * @brief PerformanceDataManager::handleProcessDetailViews
 * @param clusterName - the clustering criteria name
 *
 * This method handles a request for a new detail view.  After building the ArgoNavis::CUDA::PerformanceData object for threads of interest,
 * it will begin processing of the all CUDA event types by executing the visitor methods in a separate thread (via QtConcurrent::run) over
 * the entire duration of the experiment and waits for the thread to complete.
 */
void PerformanceDataManager::handleProcessDetailViews(const QString &clusteringCriteriaName)
{ 
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleRequestDetailView: clusteringCriteriaName=" << clusteringCriteriaName;
#endif

    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) )
        return;

    MetricTableViewInfo& info = m_tableViewInfo[ clusteringCriteriaName ];

    const QString metricViewName = CUDA_EVENT_DETAILS_METRIC + "-" + ALL_EVENTS_DETAILS_VIEW;

    // the details view should only be setup once
    info.addMetricView( metricViewName );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    for ( auto metricName : { ALL_EVENTS_DETAILS_VIEW, KERNEL_EXECUTION_DETAILS_VIEW, DATA_TRANSFER_DETAILS_VIEW } ) {
#else
    foreach ( const QString& metricName, QStringList() << ALL_EVENTS_DETAILS_VIEW << KERNEL_EXECUTION_DETAILS_VIEW << DATA_TRANSFER_DETAILS_VIEW ) {
#endif
        info.addMetricView( CUDA_EVENT_DETAILS_METRIC + "-" + metricName );
    }

    ThreadGroup all_threads = info.getThreads();
    CollectorGroup collectors = info.getCollectors();

    boost::optional<Collector> collector;
    for ( CollectorGroup::const_iterator i = collectors.begin(); i != collectors.end(); ++i ) {
        if ( "cuda" == (*i).getMetadata().getUniqueId() ) {
            collector = *i;
            break;
        }
    }

    if ( ! collector )
        return;

    std::set<int> ranks;
    CUDA::PerformanceData data;
    QMap< Base::ThreadName, Thread> threads;

    for (ThreadGroup::const_iterator i = all_threads.begin(); i != all_threads.end(); ++i) {
        std::pair<bool, int> rank = i->getMPIRank();

        if ( ranks.empty() || ( rank.first && (ranks.find(rank.second) != ranks.end() )) ) {
            GetCUDAPerformanceData( collector.get(), *i, data );

            threads.insert( ConvertToArgoNavis(*i), *i );
        }
    }

    // defines columns of model for both Data Transfer and Kernel Execution events
    QStringList tableColumnList = ArgoNavis::CUDA::getKernelExecutionDetailsHeaderList();  // intialize with columns for Kernel Execution events
    // defines columns common to Kernel Execution and Data Transfer events to be used for "All Events" details view
    QStringList commonColumnList;

    // Generate the "All Events" details view column list with the common columns from the "Data Transfers" and "Kernel Executions" column lists.
    // Generate the detail view model column list by adding the unique columns from the "Data Transfers" column list appended to the end of the
    // "Kernel Executions" column list.
    foreach( const QString& columnName, ArgoNavis::CUDA::getDataTransferDetailsHeaderList() ) {
        if ( tableColumnList.contains( columnName ) )
            commonColumnList << columnName;
        else
            tableColumnList << columnName;
    }

    // for details view emit signal to create just the model
    emit addMetricView( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, tableColumnList );

    // build the proxy views and tree views for the three details views: "All Events", "Kernel Executions" and "Data Transfers"
    emit addAssociatedMetricView( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, metricViewName, commonColumnList );
    emit addAssociatedMetricView( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, KERNEL_EXECUTION_DETAILS_VIEW, metricViewName, ArgoNavis::CUDA::getKernelExecutionDetailsHeaderList() );
    emit addAssociatedMetricView( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, DATA_TRANSFER_DETAILS_VIEW, metricViewName, ArgoNavis::CUDA::getDataTransferDetailsHeaderList() );

    QFutureSynchronizer<void> synchronizer;

    const Base::TimeInterval interval( ConvertToArgoNavis( info.getInterval() ) );

    foreach( const ArgoNavis::Base::ThreadName thread, threads.keys() ) {
        synchronizer.addFuture( QtConcurrent::run( &data, &CUDA::PerformanceData::visitDataTransfers, thread, interval,
                                                   boost::bind( &PerformanceDataManager::processDataTransferDetails, this,
                                                                boost::cref(clusteringCriteriaName), boost::cref(data.interval().begin()), _1 ) ) );

        synchronizer.addFuture( QtConcurrent::run( &data, &CUDA::PerformanceData::visitKernelExecutions, thread, interval,
                                                   boost::bind( &PerformanceDataManager::processKernelExecutionDetails, this,
                                                                boost::cref(clusteringCriteriaName), boost::cref(data.interval().begin()), _1 ) ) );
    }

    // Determine full time interval extent of this experiment
    Extent extent = info.getExtent();

    Base::TimeInterval experimentInterval = ConvertToArgoNavis( extent.getTimeInterval() );

    const double lower = ( interval.begin() - experimentInterval.begin() ) / 1000000.0;
    const double upper = ( interval.end() - experimentInterval.begin() ) / 1000000.0;

    synchronizer.waitForFinished();

    emit requestMetricViewComplete( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, lower, upper );
    emit requestMetricViewComplete( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, KERNEL_EXECUTION_DETAILS_VIEW, lower, upper );
    emit requestMetricViewComplete( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, DATA_TRANSFER_DETAILS_VIEW, lower, upper );
}

/**
 * @brief PerformanceDataManager::handleRequestTraceView
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 *
 * Handler for external request to produce trace view data for trace view.
 */
void PerformanceDataManager::handleRequestTraceView(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName)
{
    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) || metricName.isEmpty() || viewName.isEmpty() )
        return;

    MetricTableViewInfo& info = m_tableViewInfo[ clusteringCriteriaName ];

#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleRequestTraceView: clusteringCriteriaName=" << clusteringCriteriaName << "metric=" << metricName << "view=" << viewName;
#endif

    if ( 0 == info.getCollectors().size() )
        return;

    const Collector collector( *info.getCollectors().begin() );
    const QString collectorId( collector.getMetadata().getUniqueId().c_str() );

    if ( ! s_TRACING_EXPERIMENTS.contains( collectorId ) )
        return;

    const QString metricViewName = metricName + "-" + viewName;

    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QString("generate-%1").arg(metricViewName) );
    }

    info.addMetricView( metricViewName );

    const TimeInterval interval( info.getInterval() );
    const std::set< Function > functions( info.getThreads().getFunctions() );

    for ( std::set< Function >::iterator iter = functions.begin(); iter != functions.end(); iter++ ) {
        const Function& function( *iter );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        const QString functionName = QString::fromStdString( function.getDemangledName() );
#else
        const QString functionName = QString( function.getDemangledName().c_str() );
#endif
        info.addMetricView( TRACE_EVENT_DETAILS_METRIC + "-" + functionName );
    }

    // Determine full time interval extent of this experiment
    Extent extent = info.getExtent();
    Base::TimeInterval experimentInterval = ConvertToArgoNavis( extent.getTimeInterval() );
    Base::TimeInterval graphInterval = ConvertToArgoNavis( interval );
    const double lower = ( graphInterval.begin() - experimentInterval.begin() ) / 1000000.0;
    const double upper = ( graphInterval.end() - experimentInterval.begin() ) / 1000000.0;
    const double time_origin = ( experimentInterval.begin() ) / 1000000.0;

    if ( collectorId == "mpit" ) {
        ShowTraceDetail< std::vector<Framework::MPITDetail> >( clusteringCriteriaName, collector, info.getThreads(), time_origin, lower, upper, interval, functions, "exclusive_details" );
    }

    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QString("generate-%1").arg(metricViewName) );
    }
}

/**
 * @brief PerformanceDataManager::handleRequestSampleCountersView
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 *
 * Handler for external request to produce metric view data for sampling experiments.
 */
void PerformanceDataManager::handleRequestSampleCountersView(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName)
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleRequestSampleCountersView: clusteringCriteriaName=" << clusteringCriteriaName << "metric=" << metricName << "view=" << viewName;
#endif
    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) || metricName.isEmpty() || viewName.isEmpty() )
        return;

    MetricTableViewInfo& info = m_tableViewInfo[ clusteringCriteriaName ];

    if ( 0 == info.getCollectors().size() )
        return;

    const Collector collector( *info.getCollectors().begin() );
    const QString collectorId( collector.getMetadata().getUniqueId().c_str() );

    if ( ! s_SAMPLING_EXPERIMENTS.contains( collectorId ) )
        return;

    const QString metricViewName = metricName + "-" + viewName;

    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QString("generate-%1").arg(metricViewName) );
    }

    info.addMetricView( metricViewName );

    // Determine full time interval extent of this experiment
    const TimeInterval interval( info.getInterval() );
    const Extent extent = info.getExtent();
    const Base::TimeInterval experimentInterval = ConvertToArgoNavis( extent.getTimeInterval() );
    const Base::TimeInterval graphInterval = ConvertToArgoNavis( interval );
    const double lower = ( graphInterval.begin() - experimentInterval.begin() ) / 1000000.0;
    const double upper = ( graphInterval.end() - experimentInterval.begin() ) / 1000000.0;

    if ( collectorId == "hwctime" ) {
        if ( viewName == QStringLiteral("Functions") )
            ShowSampleCountersDetail< Framework::Function, Framework::HWTimeDetail >( clusteringCriteriaName, collector, info.getThreads(), lower, upper, interval, metricName, viewName );
        else if ( viewName == QStringLiteral("Statements") )
            ShowSampleCountersDetail< Framework::Statement, Framework::HWTimeDetail >( clusteringCriteriaName, collector, info.getThreads(), lower, upper, interval, metricName, viewName );
        else if ( viewName == QStringLiteral("LinkedObjects") )
            ShowSampleCountersDetail< Framework::LinkedObject, Framework::HWTimeDetail >( clusteringCriteriaName, collector, info.getThreads(), lower, upper, interval, metricName, viewName );
        else if ( viewName == QStringLiteral("Loops") )
            ShowSampleCountersDetail< Framework::Loop, Framework::HWTimeDetail >( clusteringCriteriaName, collector, info.getThreads(), lower, upper, interval, metricName, viewName );
    }

    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QString("generate-%1").arg(metricViewName) );
    }
}

/**
 * @brief PerformanceDataManager::processDataTransferEvent
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the data transfer event
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * Emit signal for the data transfer event collected from the CUDA collector at the current experiment time.
 */
bool PerformanceDataManager::processDataTransferEvent(const Base::Time& time_origin,
                                                      const CUDA::DataTransfer& details,
                                                      const QString& clusterName,
                                                      const QString& clusteringCriteriaName)
{
    emit addDataTransfer( clusteringCriteriaName, clusterName, time_origin, details );

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::processKernelExecutionEvent
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the kernel execution event
 * @param clusterName - the cluster group name
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * Emit signal for the kernel execution event collected from the CUDA collector at the current experiment time.
 */
bool PerformanceDataManager::processKernelExecutionEvent(const Base::Time& time_origin,
                                                         const CUDA::KernelExecution& details,
                                                         const QString& clusterName,
                                                         const QString& clusteringCriteriaName)
{
    emit addKernelExecution( clusteringCriteriaName, clusterName, time_origin, details );

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::processPeriodicSample
 * @param time_origin - the time origin of the experiment
 * @param time - the time of the period sample collection
 * @param counts - the vector of periodic sample counts for the time period
 * @param gpuCounterIndexes - the list of GPU sample counter indexes
 * @param clusterName - the cluster group name
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * Emit signal for each periodic sample collected at the indicated experiment time.
 */
bool PerformanceDataManager::processPeriodicSample(const Base::Time& time_origin,
                                                   const Base::Time& time,
                                                   const std::vector<uint64_t>& counts,
                                                   const QSet< int >& gpuCounterIndexes,
                                                   const QString& clusterName,
                                                   const QString& clusteringCriteriaName)
{
    Q_UNUSED( gpuCounterIndexes )

    double timeStamp = static_cast<uint64_t>( time - time_origin ) / 1000000.0;
    double lastTimeStamp;

    if ( m_sampleKeys.size() > 0 )
        lastTimeStamp = m_sampleKeys.last();
    else
        lastTimeStamp = 0.0;

    m_sampleKeys << timeStamp;

    double value( 0.0 );

    for ( std::vector<uint64_t>::size_type i = 0; i < counts.size(); ++i ) {
        const QVector< double >& counterValues = m_rawValues[i];

        if ( counterValues.size() != 0 ) {
            value = counts[i] - counterValues.last();
#ifdef USE_PERIODIC_SAMPLE_AVG
            double duration( timeStamp - lastTimeStamp );
            if ( duration > 0.0 )
                value /= duration;
#endif
        }

        m_sampleValues[i] << value;
        m_rawValues[i] << counts[i];

        if ( value > 0 && timeStamp > lastTimeStamp ) {
            // only add non-zero periodic sample bins
            emit addPeriodicSample( clusteringCriteriaName, clusterName, lastTimeStamp, timeStamp, value );
        }
    }

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::convert_performance_data
 * @param data - the performance data structure to be parsed
 * @param thread - the thread of interest
 * @param gpuCounterIndexes - the list of GPU sample counter indexes
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * Initiate visitations for data transfer and kernel execution events and period sample data.
 * Emit signals for each of these to be handled by the performance data view to build graph items for plotting.
 */
bool PerformanceDataManager::processPerformanceData(const CUDA::PerformanceData& data,
                                                    const Base::ThreadName& thread,
                                                    const QSet< int >& gpuCounterIndexes,
                                                    const QString& clusteringCriteriaName)
{
    QString clusterName = ArgoNavis::CUDA::getUniqueClusterName( thread );

#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::processPerformanceData: cluster name: " << clusterName;
#endif
#ifdef HAS_RESAMPLED_COUNTERS
    const Base::TimeInterval duration = data.interval();
    const Base::Time rate( 1000000 );
    std::size_t numCounters = data.counters().size();
    for ( std::size_t counter = 0; counter < numCounters; counter++ ) {
        Base::PeriodicSamples samples = data.periodic( thread, duration, counter );
        Base::PeriodicSamples intervalSamples = samples.resample( duration, rate );
        intervalSamples.visit( duration,
                               boost::bind( &PerformanceDataManager::processPeriodicSample, instance(),
                                            boost::cref(duration.begin()), _1, _2,
                                            boost::cref(gpuCounterIndexes),
                                            boost::cref(clusterName), boost::cref(clusteringCriteriaName) )
                               );
    }
#else
    data.visitPeriodicSamples(
                thread, data.interval(),
                boost::bind( &PerformanceDataManager::processPeriodicSample, instance(),
                             boost::cref(data.interval().begin()), _1, _2,
                             boost::cref(gpuCounterIndexes),
                             boost::cref(clusterName), boost::cref(clusteringCriteriaName) )
                );
#endif

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::processDataTransferDetails
 * @param clusteringCriteriaName - the clustering criteria name
 * @param time_origin - the time origin of the experiment
 * @param details - the CUDA data transfer details
 * @return - indicates whether visitation should continue (always true)
 *
 * Visitation method to process each CUDA data transfer event and provide to CUDA event details view.
 */
bool PerformanceDataManager::processDataTransferDetails(const QString &clusteringCriteriaName, const Base::Time &time_origin, const CUDA::DataTransfer &details)
{
    QVariantList detailsData = ArgoNavis::CUDA::getDataTransferDetailsDataList( time_origin, details );

    emit addMetricViewData( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, detailsData, ArgoNavis::CUDA::getDataTransferDetailsHeaderList() );

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::processKernelExecutionDetails
 * @param clusterName - the clustering criteria name
 * @param time_origin - the time origin of the experiment
 * @param details - the CUDA kernel execution details
 * @return - indicates whether visitation should continue (always true)
 *
 * Visitation method to process each CUDA kernel executiopn event and provide to CUDA event details view.
 */
bool PerformanceDataManager::processKernelExecutionDetails(const QString &clusteringCriteriaName, const Base::Time &time_origin, const CUDA::KernelExecution &details)
{
    QVariantList detailsData = ArgoNavis::CUDA::getKernelExecutionDetailsDataList( time_origin, details );

    emit addMetricViewData( clusteringCriteriaName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, detailsData, ArgoNavis::CUDA::getKernelExecutionDetailsHeaderList() );

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::hasDataTransferEvents
 * @param flag - the flag for corresponding thread to indicate whether the thread has data transfer events
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * If the visitation reaches this just once then the flag will be set true.
 */
bool PerformanceDataManager::hasDataTransferEvents(const CUDA::DataTransfer& details,
                                                   bool& flag)
{
    Q_UNUSED( details )

    flag = true;

    return false; // only visit once
}

/**
 * @brief PerformanceDataManager::hasKernelExecutionEvent
 * @param details - the details of the kernel execution event
 * @param flag - the flag for corresponding thread to indicate whether the thread has kernel execution events
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * If the visitation reaches this just once then the flag will be set true.
 */
bool PerformanceDataManager::hasKernelExecutionEvents(const CUDA::KernelExecution& details,
                                                      bool& flag)
{
    Q_UNUSED( details )

    flag = true;

    return false; // only visit once
}

/**
 * @brief PerformanceDataManager::processPeriodicSample
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * If the visitation finds a thread which has CUDA periodic samples, then the flag will be set 'true' and visitation will halt.
 */
bool PerformanceDataManager::hasCudaPeriodicSamples(const QSet< int >& gpuCounterIndexes,
                                                    const std::vector<uint64_t>& counts,
                                                    bool& flag)
{
    for ( std::vector<uint64_t>::size_type i = 0; i < counts.size(); ++i ) {
        if ( gpuCounterIndexes.contains( i ) && counts[i] != 0 ) {
            flag = true;
            return false;
        }
    }

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::hasCudaEvents
 * @param data - the performance data structure to be parsed
 * @param thread - the thread of interest
 * @param flag - a mapping of thread names to boolean to indicate whether threads have any CUDA events
 * @return - continue (=true) or not continue (=false) the visitation
 */
bool PerformanceDataManager::hasCudaEvents(const CUDA::PerformanceData &data,
                                           const QSet< int >& gpuCounterIndexes,
                                           const Base::ThreadName &thread,
                                           QMap< Base::ThreadName, bool >& flags)
{
    bool& flag = flags[thread];

    data.visitDataTransfers(
                thread, data.interval(),
                boost::bind( &PerformanceDataManager::hasDataTransferEvents, instance(), _1, boost::ref(flag) )
                );

    if ( ! flag ) {
        data.visitKernelExecutions(
                    thread, data.interval(),
                    boost::bind( &PerformanceDataManager::hasKernelExecutionEvents, instance(), _1, boost::ref(flag) )
                    );
    }

    if ( ! flag ) {
        data.visitPeriodicSamples(
                    thread, data.interval(),
                    boost::bind( &PerformanceDataManager::hasCudaPeriodicSamples, instance(), boost::cref(gpuCounterIndexes), _2, boost::ref(flag) )
                    );
    }

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::getThreadSet<Function>
 * @param threads - the set of threads of interest
 * @return - the set of Function objects from the thread group
 *
 * This is a template specialization of getThreadSet template for the Function typename.
 * The function accesses the thread group object and returns the set of Function objects.
 */
template <>
std::set<Function> PerformanceDataManager::getThreadSet<Function>(const ThreadGroup& threads)
{
    return threads.getFunctions();
}

/**
 * @brief PerformanceDataManager::getThreadSet<Statement>
 * @param threads - the set of threads of interest
 * @return - the set of Statement objects from the thread group
 *
 * This is a template specialization of getThreadSet template for the Statement typename.
 * The function accesses the thread group object and returns the set of Statement objects.
 */
template <>
std::set<Statement> PerformanceDataManager::getThreadSet<Statement>(const ThreadGroup& threads)
{
    return threads.getStatements();
}

/**
 * @brief PerformanceDataManager::getThreadSet<LinkedObject>
 * @param threads - the set of threads of interest
 * @return - the set of LinkedObject objects from the thread group
 *
 * This is a template specialization of getThreadSet template for the LinkedObject typename.
 * The function accesses the thread group object and returns the set of LinkedObject objects.
 */
template <>
std::set<LinkedObject> PerformanceDataManager::getThreadSet<LinkedObject>(const ThreadGroup& threads)
{
    return threads.getLinkedObjects();
}

/**
 * @brief PerformanceDataManager::getThreadSet<Loop>
 * @param threads - the set of threads of interest
 * @return - the set of Loops objects from the thread group
 *
 * This is a template specialization of getThreadSet template for the Loops typename.
 * The function accesses the thread group object and returns the set of Loops objects.
 */
template <>
std::set<Loop> PerformanceDataManager::getThreadSet<Loop>(const ThreadGroup& threads)
{
    return threads.getLoops();
}

/**
 * @brief getLocationInfo
 * @param metric - the Function metric object reference
 * @return - the defining location information form the metric object reference
 *
 * This is a template specialization of getLocationInfo template for the Function typename.
 * The function accesses the Function object and returns the defining location information,
 */
template <>
QString PerformanceDataManager::getLocationInfo(const Function& metric)
{
    QString locationInfo;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    locationInfo = QString::fromStdString( metric.getDemangledName() );
#else
    locationInfo = QString( metric.getDemangledName().c_str() );
#endif

    std::set<Statement> definitions = metric.getDefinitions();
    for(std::set<Statement>::const_iterator j = definitions.begin(); j != definitions.end(); ++j)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        locationInfo += " (" + QString::fromStdString( j->getPath().getDirName() ) + QString::fromStdString( j->getPath().getBaseName() ) + ", " + QString::number( j->getLine() ) + ")";
#else
        locationInfo += " (" + QString( j->getPath().getDirName().c_str() ) + QString( j->getPath().getBaseName().c_str() ) + ", " + QString::number( j->getLine() ) + ")";
#endif

    return locationInfo;
}

/**
 * @brief getLocationInfo
 * @param metric - the LinkedObject metric object reference
 * @return - the defining location information form the metric object reference
 *
 * This is a template specialization of getLocationInfo template for the LinkedObject typename.
 * The function accesses the LinkedObject object and returns the defining location information,
 */
template <>
QString PerformanceDataManager::getLocationInfo(const LinkedObject& metric)
{
    QString locationInfo;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    locationInfo = QString::fromStdString( metric.getPath() );
#else
    locationInfo = QString( metric.getPath().c_str() );
#endif

    return locationInfo;
}

/**
 * @brief getLocationInfo
 * @param metric - the Statement metric object reference
 * @return - the defining location information form the metric object reference
 *
 * This is a template specialization of getLocationInfo template for the Statement typename.
 * The function accesses the Statement object and returns the defining location information,
 */
template <>
QString PerformanceDataManager::getLocationInfo(const Statement& metric)
{
    QString locationInfo;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    locationInfo = QString::fromStdString( metric.getPath() );
#else
    locationInfo = QString( metric.getPath().c_str() );
#endif
    locationInfo += ", " + QString::number( metric.getLine() );

    return locationInfo;
}

/**
 * @brief getLocationInfo
 * @param metric - the Loop metric object reference
 * @return - the defining location information form the metric object reference
 *
 * This is a template specialization of getLocationInfo template for the Loop typename.
 * The function accesses the Loop object and returns the defining location information,
 */
template <>
QString PerformanceDataManager::getLocationInfo(const Loop& metric)
{
    QString locationInfo;

    std::set<Statement> definitions = metric.getDefinitions();
    for(std::set<Statement>::const_iterator j = definitions.begin(); j != definitions.end(); ++j)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
       locationInfo += QString::fromStdString( j->getPath().getDirName() ) + QString::fromStdString( j->getPath().getBaseName() ) + ", " + QString::number( j->getLine() );
#else
       locationInfo += QString( j->getPath().getDirName().c_str() ) + QString( j->getPath().getBaseName().c_str() ) + ", " + QString::number( j->getLine() );
#endif

    return locationInfo;
}

/**
 * @brief PerformanceDataManager::getViewName<Function>
 * @return - the view name appropriate for the metric type
 *
 * This is a template specialization of the getViewName template for the Function typename.
 * The function returns the view name for the Function metric view.
 */
template <>
QString PerformanceDataManager::getViewName<Function>() const
{
    return QStringLiteral("Functions");
}

/**
 * @brief PerformanceDataManager::getViewName<Statement>
 * @return - the view name appropriate for the metric type
 *
 * This is a template specialization of the getViewName template for the Statement typename.
 * The function returns the view name for the Statement metric view.
 */
template <>
QString PerformanceDataManager::getViewName<Statement>() const
{
    return QStringLiteral("Statements");
}

/**
 * @brief PerformanceDataManager::getViewName<LinkedObject>
 * @return - the view name appropriate for the metric type
 *
 * This is a template specialization of the getViewName template for the LinkedObject typename.
 * The function returns the view name for the LinkedObject metric view.
 */
template <>
QString PerformanceDataManager::getViewName<LinkedObject>() const
{
    return QStringLiteral("LinkedObjects");
}

/**
 * @brief PerformanceDataManager::getViewName<Loop>
 * @return - the view name appropriate for the metric type
 *
 * This is a template specialization of the getViewName template for the Loop typename.
 * The function returns the view name for the Loop metric view.
 */
template <>
QString PerformanceDataManager::getViewName<Loop>() const
{
    return QStringLiteral("Loops");
}

/**
 * @brief PerformanceDataManager::processCompareThreadView
 * @param collectors - the set of collectors
 * @param all_threads - the set of all threads
 * @param interval - the time interval of interest
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metric - the metric to generate data for
 * @param compareMode - the specific compare mode
 *
 * Build function/statement view output for the specified metrics for all threads over the entire experiment time period.
 * NOTE: must be metrics providing time information.
 */
template <typename TS>
void PerformanceDataManager::processCompareThreadView(const CollectorGroup& collectors, const ThreadGroup& all_threads, const TimeInterval &interval, const QString &clusteringCriteriaName, QString metric, QString compareMode)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processCompareThreadView STARTED" << metric;
#endif
#if defined(HAS_CONCURRENT_PROCESSING_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processCompareThreadView: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif

    if ( 0 == collectors.size() )
        return;

    QList< ThreadGroup > threadGroupList;

    getListOfThreadGroupsFromSelectedClusters( clusteringCriteriaName, compareMode, all_threads, threadGroupList );

    const QString viewName = getViewName<TS>();
    const Collector collector( *collectors.begin() );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    std::string metricStr = metric.toStdString();
#else
    std::string metricStr = std::string( metric.toLatin1().data() );
#endif

    const QString metricViewName = metric + QStringLiteral("-") + viewName;

    QStringList metricDesc;
    metricDesc << s_functionTitle;

    QMap< TS, QVariantList > metricData;

    SmartPtr<std::map<TS, std::map<Thread, double> > > individual;

    int count(0);

    for ( QList< ThreadGroup >::iterator titer = threadGroupList.begin(); titer != threadGroupList.end(); ++titer, ++count ) {
        // make thread group with just the current thread
        ThreadGroup threads( *titer );
        // get metric values
        Queries::GetMetricValues( collector,
                                  metricStr,
                                  interval,
                                  threads,
                                  getThreadSet<TS>( threads ),
                                  individual );

        // compute summation
        SmartPtr<std::map<TS, double> > data = Queries::Reduction::Apply( individual, Queries::Reduction::Summation );

        // reset individual data
        individual = SmartPtr<std::map<TS, std::map<Thread, double> > >();

        // organize into rows by TS with columns for each threads value
        for( typename std::map<TS, double>::const_iterator i = data->begin(); i != data->end(); ++i ) {
            // get reference to QVariantList corresponding to TS
            QVariantList& vdata = metricData[ i->first ];
            if ( vdata.empty() ) {
                // add location information based on TS at the end
                vdata << getLocationInfo<TS>( i->first );
            }
            // fill in null values for each thread not containing TS
            while ( vdata.size() < count+1 ) {
                vdata << QVariant();
            }
            // scale value to milliseconds
            const double value( i->second * 1000.0 );
            // add actual value
            vdata << value;
        }

        // add column header values
        const QString columnName = getColumnNameForCompareView( compareMode, *(threads.begin()) );
        metricDesc << tr("%1 (msec)").arg(columnName);
    }

    emit addMetricView( clusteringCriteriaName, compareMode, metricViewName, metricDesc );

    for ( typename QMap< TS, QVariantList >::iterator i = metricData.begin(); i != metricData.end(); ++i ) {
        QVariantList& data = i.value();
        emit addMetricViewData( clusteringCriteriaName, compareMode, metricViewName, data );
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processCompareThreadView FINISHED" << metric;
#endif
}

/**
 * @brief PerformanceDataManager::processMetricView
 * @param collectors - the set of collectors
 * @param all_threads - the set of all threads
 * @param interval - the time interval of interest
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metric - the metric to generate data for
 *
 * Build function/statement view output for the specified metrics for all threads over the entire experiment time period.
 * NOTE: must be metrics providing time information.
 */
template <typename TS>
void PerformanceDataManager::processMetricView(const CollectorGroup &collectors, const ThreadGroup& all_threads, const TimeInterval &interval, const QString &clusteringCriteriaName, QString metric)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processMetricView STARTED" << metric;
#endif
#if defined(HAS_CONCURRENT_PROCESSING_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processMetricView: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif

    if ( 0 == collectors.size() )
        return;

    QStringList metricDesc;
    metricDesc << s_timeTitle << s_percentageTitle << s_functionTitle << s_minimumTitle << s_maximumTitle << s_meanTitle;

    ThreadGroup threadGroup;

    getThreadGroupFromSelectedClusters( clusteringCriteriaName, all_threads, threadGroup );

    const QString viewName = getViewName<TS>();
    const Collector collector( *collectors.begin() );

    // Evaluate the first collector's time metric for all functions
    SmartPtr<std::map<TS, std::map<Thread, double> > > individual;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    std::string metricStr = metric.toStdString();
#else
    std::string metricStr = std::string( metric.toLatin1().data() );
#endif
    Queries::GetMetricValues( collector,
                              metricStr,
                              interval,
                              threadGroup,
                              getThreadSet<TS>( threadGroup ),
                              individual );
    SmartPtr<std::map<TS, double> > data =
            Queries::Reduction::Apply( individual, Queries::Reduction::Summation );
    SmartPtr<std::map<TS, double> > dataMin =
            Queries::Reduction::Apply( individual, Queries::Reduction::Minimum );
    SmartPtr<std::map<TS, double> > dataMax =
            Queries::Reduction::Apply( individual, Queries::Reduction::Maximum );
    SmartPtr<std::map<TS, double> > dataMean =
            Queries::Reduction::Apply( individual, Queries::Reduction::ArithmeticMean );
    individual = SmartPtr<std::map<TS, std::map<Thread, double> > >();

    // Sort the results
    std::multimap<double, TS> sorted;
    double total( 0.0 );
    for( typename std::map<TS, double>::const_iterator i = data->begin(); i != data->end(); ++i ) {
        sorted.insert(std::make_pair(i->second, i->first));
        total += i->second;
    }

    // Display the results

#ifdef HAS_PROCESS_METRIC_VIEW_DEBUG
    std::cout << std::endl << std::endl
              << std::setw(10) << metricDesc[0].toStdString()
              << "    "
              << metricDesc[1].toStdString() << std::endl
              << std::endl;
#endif

    emit addMetricView( clusteringCriteriaName, metric, viewName, metricDesc );

    for ( typename std::multimap<double, TS>::reverse_iterator i = sorted.rbegin(); i != sorted.rend(); ++i ) {
        QVariantList metricData;

        const double value( i->first * 1000.0 );
        const double min( dataMin->at(i->second) * 1000.0 );
        const double max( dataMax->at(i->second) * 1000.0 );
        const double mean( dataMean->at(i->second) * 1000.0 );
        const double percentage( i->first / total * 100.0 );

        metricData << value;
        metricData << percentage;
        metricData << getLocationInfo<TS>( i->second );
        metricData << min;
        metricData << max;
        metricData << mean;

        emit addMetricViewData( clusteringCriteriaName, metric, viewName, metricData );
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processMetricView FINISHED" << metric;
#endif
}

/**
 * @brief PerformanceDataManager::processLoadBalanceView
 * @param collectors - the set of collectors
 * @param all_threads - the set of all threads
 * @param interval - the time interval of interest
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metric - the metric to generate data for
 *
 * Build function/statement view output for the specified metrics for all threads over the entire experiment time period.
 * NOTE: must be metrics providing time information.
 */
template <typename TS>
void PerformanceDataManager::processLoadBalanceView(const CollectorGroup& collectors, const ThreadGroup& all_threads, const TimeInterval &interval, const QString &clusteringCriteriaName, QString metric)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processLoadBalanceView STARTED" << metric;
#endif
#if defined(HAS_CONCURRENT_PROCESSING_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processLoadBalanceView: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif

    if ( 0 == collectors.size() )
        return;

    QStringList metricDesc;
    metricDesc << s_maximumTitle << s_maximumThreadTitle << s_minimumTitle << s_minimumThreadTitle << s_meanTitle << s_meanThreadTitle << s_functionTitle;

    ThreadGroup threadGroup;

    getThreadGroupFromSelectedClusters( clusteringCriteriaName, all_threads, threadGroup );

    const QString viewName = getViewName<TS>();
    const Collector collector( *collectors.begin() );

    // Evaluate the first collector's time metric for all functions
    SmartPtr<std::map<TS, std::map<Thread, double> > > individual;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    std::string metricStr = metric.toStdString();
#else
    std::string metricStr = std::string( metric.toLatin1().data() );
#endif

    Queries::GetMetricValues( collector,
                              metricStr,
                              interval,
                              threadGroup,
                              getThreadSet<TS>( threadGroup ),
                              individual );

    SmartPtr< std::map< TS, double > > dataMin =
            Queries::Reduction::Apply( individual, Queries::Reduction::Minimum );
    SmartPtr< std::map< TS, double > > dataMax =
            Queries::Reduction::Apply( individual, Queries::Reduction::Maximum );
    SmartPtr<std::map<TS, double> > dataMean =
            Queries::Reduction::Apply( individual, Queries::Reduction::ArithmeticMean );

    std::map< TS, Thread > minimumThreads;
    std::map< TS, Thread > maximumThreads;
    std::map< TS, Thread > meanThreads;

    // find thread exhbiting the minimum and maximum times for each TS item
    for( typename std::map< TS, std::map<Thread, double > >::const_iterator i = individual->begin(); i != individual->end(); ++i ) {
        const std::map<Thread, double>& threadMetricMap = i->second;
        const double min( dataMin->at( i->first ) );
        const double max( dataMax->at( i->first ) );
        const double mean( dataMean->at( i->first ) );
        double diff( std::numeric_limits< double >::max() );
        std::map< Thread, double >::const_iterator miter = threadMetricMap.begin();
        for( std::map< Thread, double >::const_iterator titer = threadMetricMap.begin(); titer != threadMetricMap.end(); ++titer ) {
            if ( min == titer->second ) {
                minimumThreads.insert( std::make_pair( i->first, titer->first ) );
            }
            if ( max == titer->second ) {
                maximumThreads.insert( std::make_pair( i->first, titer->first ) );
            }
            const double temp_diff = std::fabs( mean - titer->second );
            if ( temp_diff < diff ) {
                miter = titer;
                diff = temp_diff;
            }
        }
        meanThreads.insert( std::make_pair( i->first, miter->first ) );
    }

    // reset the individual instance
    individual = SmartPtr<std::map< TS, std::map< Thread, double > > >();

    // Sort the results
    std::multimap< double, TS > sorted;
    for( typename std::map< TS, double >::const_iterator i = dataMax->begin(); i != dataMax->end(); ++i ) {
        sorted.insert( std::make_pair( i->second, i->first ) );
    }

    emit addMetricView( clusteringCriteriaName, QStringLiteral("Load Balance"), metric + "-" + viewName, metricDesc );

    for( typename std::map< TS, double >::const_iterator i = dataMax->begin(); i != dataMax->end(); ++i ) {
        QVariantList metricData;

        const double max( i->second * 1000.0 );
        const double min( dataMin->at( i->first ) * 1000.0 );
        const double mean( dataMean->at( i->first ) * 1000.0 );

        metricData << max;
        metricData << ArgoNavis::CUDA::getUniqueClusterName( maximumThreads.at( i->first ) );
        metricData << min;
        metricData << ArgoNavis::CUDA::getUniqueClusterName( minimumThreads.at( i->first ) );
        metricData << mean;
        metricData << ArgoNavis::CUDA::getUniqueClusterName( meanThreads.at( i->first ) );
        metricData << getLocationInfo<TS>( i->first );

        emit addMetricViewData( clusteringCriteriaName, QStringLiteral("Load Balance"), metric + "-" + viewName, metricData );
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processLoadBalanceView FINISHED" << metric;
#endif
}

/**
 * @brief PerformanceDataManager::processCalltreeView
 * @param collector - a copy of the cuda collector
 * @param threads - all threads known by the cuda collector
 * @param interval - the time interval of interest
 * @param clusteringCriteriaName - the name of the clustering criteria
 *
 * Build calltree view output for the specified group of threads and time interval.
 */
void PerformanceDataManager::processCalltreeView(const Collector& collector, const ThreadGroup& threads, const TimeInterval& interval, const QString& clusteringCriteriaName)
{
    const std::set< Function > functions( threads.getFunctions() );

    QStringList metricDesc;
    metricDesc << QStringLiteral("Inclusive Time") << QStringLiteral("Inclusive Counts") << s_functionTitle;

    const std::string collectorId = collector.getMetadata().getUniqueId();

    if ( collectorId == "usertime" ) {
        ShowCalltreeDetail< Framework::UserTimeDetail >( collector, threads, interval, functions, "inclusive_detail", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "cuda" ) {
        ShowCalltreeDetail< std::vector<Framework::CUDAExecDetail> >( collector, threads, interval, functions, "exec_inclusive_details", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "mpi" ) {
        ShowCalltreeDetail< std::vector<Framework::MPIDetail> >( collector, threads, interval, functions, "inclusive_details", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "pthreads" ) {
        ShowCalltreeDetail< std::vector<Framework::PthreadsDetail> >( collector, threads, interval, functions, "inclusive_details", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "omptp" ) {
        ShowCalltreeDetail< Framework::OmptPDetail >( collector, threads, interval, functions, "inclusive_detail", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "mpit" ) {
        ShowCalltreeDetail< std::vector<Framework::MPITDetail> >( collector, threads, interval, functions, "inclusive_details", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "mpip" ) {
        ShowCalltreeDetail< Framework::MPIPDetail >( collector, threads, interval, functions, "inclusive_detail", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "io" ) {
        ShowCalltreeDetail< std::vector<Framework::IODetail> >( collector, threads, interval, functions, "inclusive_details", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "iot" ) {
        ShowCalltreeDetail< std::vector<Framework::IOTDetail> >( collector, threads, interval, functions, "inclusive_details", metricDesc, clusteringCriteriaName );
    }
    else if ( collectorId == "iop" ) {
        ShowCalltreeDetail< Framework::IOPDetail >( collector, threads, interval, functions, "inclusive_detail", metricDesc, clusteringCriteriaName );
    }
}

/**
 * @brief PerformanceDataManager::print_details
 * @param details_name - the details view name
 * @param details - the array of details information
 *
 * This method dumps the contents of the details information array for debugging purposes.
 */
void PerformanceDataManager::print_details(const std::string& details_name, const TDETAILS &details) const
{
    std::string name( details_name );
    std::size_t pos = name.find_first_of( "_" );
    if ( pos != std::string::npos )
        name[pos] = ' ';
    std::cout << "Reduced " << name << " (by function name):" << std::endl;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    for (const details_data_t& d : details) {
        std::cout << "\t" << std::left << std::setw(20) << std::fixed << std::setprecision(6) << std::get<1>(d) << std::setw(20) << std::get<0>(d) << "   ";
        std::ostringstream oss;
        for ( uint32_t i=0; i<std::get<3>(d); ++i ) oss << '>';
        const Function func( std::get<2>(d) );
#else
    for ( std::size_t i=0; i<details.size(); i++ ) {
        const details_data_t& d = details[i];
        std::cout << "\t" << std::left << std::setw(20) << std::fixed << std::setprecision(6) << boost::get<1>(d) << std::setw(20) << boost::get<0>(d) << "   ";
        std::ostringstream oss;
        for ( uint32_t i=0; i<boost::get<3>(d); ++i ) oss << '>';
        const Function func( boost::get<2>(d) );
#endif
        std::cout << oss.str() << func.getName() << " (" << func.getLinkedObject().getPath().getBaseName() << ")" << std::endl;
    }
}

/**
 * @brief PerformanceDataManager::getMetricNames
 * @param metrics - metric metadata from the collector
 * @param searchMetric - metric name substring to search for
 * @return - list of matching metric names
 *
 * This function returns a list of metric names matching the search criteria
 */
template <typename T>
QStringList PerformanceDataManager::getMetricNameList(const std::set<Metadata>& metrics, const QString& searchMetric)
{
    QStringList metricList;

    for ( std::set<Metadata>::iterator iter = metrics.begin();  iter != metrics.end(); iter++ ) {
        const Metadata metadata( *iter );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString metricName = QString::fromStdString( metadata.getUniqueId() );
#else
        QString metricName( metadata.getUniqueId().c_str() );
#endif
        if ( metricName.contains( searchMetric ) && metadata.isType( typeid(T) ) ) {
            metricList << metricName;
        }
    }

    return metricList;
}

/**
 * @brief PerformanceDataManager::asyncLoadCudaViews
 * @param filePath - filename of the experiment database to be opened and processed into the performance data manager and view for display
 *
 * Executes PerformanceDataManager::loadCudaViews asynchronously.
 */
void PerformanceDataManager::asyncLoadCudaViews(const QString& filePath)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::asyncLoadCudaViews: filePath=" << filePath;
#endif
    QtConcurrent::run( this, &PerformanceDataManager::loadCudaViews, filePath );
}

/**
 * @brief PerformanceDataManager::loadCudaViews
 * @param filePath - filename of the experiment database to be opened and processed into the performance data manager and view for display
 *
 * The method invokes various thread using the QtConcurrent::run method to process plot and metric view data.  Each thread is synchronized
 * and loadComplete() signal emitted upon completion of all threads.
 */
void PerformanceDataManager::loadCudaViews(const QString &filePath)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaViews: STARTED";
#endif

    Experiment* experiment;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    std::string filePathStr = filePath.toStdString();
#else
    std::string filePathStr = std::string( filePath.toLatin1().data() );
#endif
    if ( m_experiments.contains( filePath ) )
        experiment = m_experiments[ filePath ];
    else {
        experiment = new Experiment( filePathStr );
    }

    // Determine full time interval extent of this experiment
    Extent extent = experiment->getPerformanceDataExtent();
    // Get full experiment time interval
    TimeInterval experiment_interval = extent.getTimeInterval();
    // Compute time origin
    const double time_origin = experiment_interval.getBegin().getValue();
#ifdef HAS_TEST_DATA_RANGE_CONSTRAINT
    const Time stime = experiment_interval.getBegin();
    const Time etime = experiment_interval.getEnd();
    //TimeInterval interval = TimeInterval( stime + experiment_interval.getWidth() / 32, stime + experiment_interval.getWidth() / 16 );
    TimeInterval interval = TimeInterval( etime - experiment_interval.getWidth() / 2, etime );
#else
    TimeInterval interval = extent.getTimeInterval();
#endif

    const double lower = ( interval.getBegin().getValue() - time_origin ) / 1000000.0;
    const double upper = ( interval.getEnd().getValue() - time_origin ) / 1000000.0;

    QStringList metricList;

    CollectorGroup collectors = experiment->getCollectors();
    boost::optional<Collector> collector;
    bool foundOne( false );

    for ( CollectorGroup::const_iterator i = collectors.begin(); i != collectors.end() && ! foundOne; ++i ) {

        const QString collectorId( i->getMetadata().getUniqueId().c_str() );
        if ( collectorId == "hwctime" )
            metricList = getMetricNameList< std::map<Framework::StackTrace, Framework::HWTimeDetail> >( i->getMetrics(), DETAIL_METRIC );
        else
            metricList = getMetricNameList< double >( i->getMetrics(), TIME_METRIC );
        foundOne = ( metricList.size() > 0 );
        collector = *i;
    }

    if ( collector ) {
        const QString collectorId( collector.get().getMetadata().getUniqueId().c_str() );

        const bool hasCudaCollector( "cuda" == collectorId );

        const bool hasTraceExperiment = s_TRACING_EXPERIMENTS.contains( collectorId );

        QFutureSynchronizer<void> synchronizer;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString experimentFilename = QString::fromStdString( experiment->getName() );
#else
        QString experimentFilename( QString( experiment->getName().c_str() ) );
#endif

        QFileInfo fileInfo( experimentFilename );
        QString experimentName( fileInfo.fileName() );
        experimentName.replace( QString(".openss"), QString("") );

        const ThreadGroup group = experiment->getThreads();

        // Initially all default metric views are computed using all threads.  Set the set of threads for the
        // current clustering criteria to be all threads.

        QSet< QString> selected;
        int rankCount = 0;
        for ( ThreadGroup::iterator iter = group.begin(); iter != group.end(); ++iter ) {
            const Thread thread( *iter );
            int rank;
            bool valid;
            std::tie(valid,rank) = thread.getMPIRank();
            if ( valid && rank > rankCount ) {
                rankCount = rank;
            }
            selected.insert( ArgoNavis::CUDA::getUniqueClusterName( thread ) );
        }
        rankCount++;

        QString clusteringCriteriaName;
        if ( hasCudaCollector )
            clusteringCriteriaName = QStringLiteral( "GPU Compute / Data Transfer Ratio" );
        else
            clusteringCriteriaName = QStringLiteral( "Thread Groups" );

        {
            QMutexLocker guard( &m_mutex );

            m_selectedClusters[ clusteringCriteriaName ] = selected;
        }

        if ( ! m_experiments.contains( clusteringCriteriaName ) )
            m_experiments.insert( filePath, experiment );

        MetricTableViewInfo info( experiment, interval, metricList );

        m_tableViewInfo.insert( clusteringCriteriaName, info );

        QVector< QString > clusterNames;
        foreach( const QString& clusterName, selected ) {
            clusterNames << clusterName;
        }

        if ( hasCudaCollector ) {
            QFuture<void> future1 = QtConcurrent::run( this, &PerformanceDataManager::loadCudaView, experimentName, clusteringCriteriaName, collector.get(), experiment->getThreads() );
            synchronizer.addFuture( future1 );

            QFuture<void> future2 = QtConcurrent::run( this, &PerformanceDataManager::handleProcessDetailViews, clusteringCriteriaName );
            synchronizer.addFuture( future2 );
        }
        else {
            // set default metric view
            emit signalSetDefaultMetricView( hasTraceExperiment ? TIMELINE_VIEW : CALLTREE_VIEW, !s_SAMPLING_EXPERIMENTS.contains( collectorId ), hasTraceExperiment );

            QVector< bool > isGpuSampleCounters;
            QVector< QString > sampleCounterNames;

            emit addExperiment( experimentName, clusteringCriteriaName, clusterNames, isGpuSampleCounters, sampleCounterNames );

            if ( hasTraceExperiment ) {
                emit addCluster( clusteringCriteriaName, clusteringCriteriaName, lower, upper, true, -1.0, rankCount );

                QFuture<void> future = QtConcurrent::run( this, &PerformanceDataManager::handleRequestTraceView, clusteringCriteriaName, TRACE_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW );
                synchronizer.addFuture( future );
            }
        }

        const QString functionView = QStringLiteral("Functions");

        foreach ( const QString& metricName, metricList ) {
            handleRequestMetricView( clusteringCriteriaName, metricName, functionView );
        }

        synchronizer.waitForFinished();

        if ( hasTraceExperiment ) {
            foreach( const QString& clusterName, selected ) {
                emit setMetricDuration( clusteringCriteriaName, clusterName, lower, upper );
            }
            emit setMetricDuration( clusteringCriteriaName, clusteringCriteriaName, lower, upper );
        }
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaViews: ENDED";
#endif

    emit loadComplete();
}

/**
 * @brief PerformanceDataManager::handleLoadCudaMetricViews
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param clusterName - the cluster name
 * @param lower - the lower value of the interval to process
 * @param upper - the upper value of the interval to process
 *
 * Handles graph range change - initiates a timed monitor to actually process the graph range change if user hasn't
 * continued to update the graph range within a threshold period.
 */
void PerformanceDataManager::handleLoadCudaMetricViews(const QString& clusteringCriteriaName, const QString& clusterName, double lower, double upper)
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleLoadCudaMetricViews: clusteringCriteriaName=" << clusteringCriteriaName << "lower=" << lower << "upper=" << upper;
#endif

    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) || lower >= upper )
        return;

    // cancel any active processing for this cluster
    m_userChangeMgr.cancel( clusterName );

    // initiate a new timed monitor for this cluster
    m_userChangeMgr.create( clusteringCriteriaName, clusterName, lower, upper );
}

/**
 * @brief PerformanceDataManager::handleLoadCudaMetricViewsTimeout
 * @param clusteringCriteriaName - the clustering criteria name
 * @param lower - the lower value of the interval to process
 * @param upper - the upper value of the interval to process
 *
 * This handler in invoked when the waiting period has benn reached and actual processing of the CUDA metric view can proceed.
 */
void PerformanceDataManager::handleLoadCudaMetricViewsTimeout(const QString& clusteringCriteriaName, double lower, double upper)
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleLoadCudaMetricViewsTimeout: clusteringCriteriaName=" << clusteringCriteriaName << "lower=" << lower << "upper=" << upper;
#endif

    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) )
        return;

    MetricTableViewInfo& info = m_tableViewInfo[ clusteringCriteriaName ];

    // Determine time origin from extent of this experiment
    Extent extent = info.getExtent();
    Time timeOrigin = extent.getTimeInterval().getBegin();

    // Calculate new interval from currently selected graph range
    Time lowerTime = timeOrigin + lower * 1000000;
    Time upperTime = timeOrigin + upper * 1000000;

    // update interval
    info.setInterval( lowerTime, upperTime );

    foreach ( const QString& metricViewName, info.getMetricViewList() ) {
        QStringList tokens = metricViewName.split('-');
        if ( tokens.size() < 2 || tokens.size() > 3 )
            continue;
        if ( 2 == tokens.size() ) {
            if ( tokens[0] == CUDA_EVENT_DETAILS_METRIC || tokens[0] == TRACE_EVENT_DETAILS_METRIC ) {
                // Emit signal to update detail views corresponding to timeline in graph view
                emit metricViewRangeChanged( clusteringCriteriaName, tokens[0], tokens[1], lower, upper );
            }
            else {
                handleRequestMetricView( clusteringCriteriaName, tokens[0], tokens[1] );
            }
        }
        else {
            if ( tokens[0].startsWith("Compare") ) {
                handleRequestCompareView( clusteringCriteriaName, tokens[0], tokens[1], tokens[2] );
            }
            else if ( QStringLiteral("Load Balance") == tokens[0] ) {
                handleRequestLoadBalanceView( clusteringCriteriaName, tokens[1], tokens[2] );
            }
        }
    }
}

/**
 * @brief PerformanceDataManager::handleSelectedClustersChanged
 * @param criteriaName - the clustering criteria name associated with the cluster group
 * @param selected - the names of the selected set of clusters
 *
 * This method processes the changes to the selected set of clusters.
 */
void PerformanceDataManager::handleSelectedClustersChanged(const QString &criteriaName, const QSet<QString> &selected)
{
    {
        QMutexLocker guard( &m_mutex );

        m_selectedClusters[ criteriaName ] = selected;
    }

    emit signalRequestMetricTableViewUpdate( true );
}

/**
 * @brief PerformanceDataManager::loadCudaMetricViews
 * @param synchronizer - the QFutureSynchronizer to add futures
 * @param futures - the future map used when invoking QtConcurrent::run
 * @param clusteringCriteriaName - the clustering criteria name
 * @param metricList - the list of metrics to process and add to metric view
 * @param viewList - the list of views to process and add to the metric view
 * @param collector - the collector object
 * @param experiment - the experiment object
 * @param interval - the interval to process for the metric view
 *
 * Process the specified metric views.
 */
void PerformanceDataManager::loadCudaMetricViews(
        QFutureSynchronizer<void>& synchronizer,
        QMap< QString, QFuture<void> >& futures,
        const QString& clusteringCriteriaName,
        const QStringList& metricList,
        const QStringList& viewList,
        const CollectorGroup& collectors,
        const ThreadGroup& all_threads,
        const TimeInterval& interval)
{
    foreach ( QString metricName, metricList ) {
        foreach ( QString viewName, viewList ) {
            const QString futuresKey = metricName + QStringLiteral("-") + viewName;
            if ( viewName == QStringLiteral("Functions") ) {
                futures[ futuresKey ] = QtConcurrent::run(
                            boost::bind( &PerformanceDataManager::processMetricView<Function>, this,
                                         boost::cref(collectors), boost::cref(all_threads), boost::cref(interval), boost::cref(clusteringCriteriaName), metricName ) );
                synchronizer.addFuture( futures[ futuresKey ] );
            }

            else if ( viewName == QStringLiteral("Statements") ) {
                futures[ futuresKey ] = QtConcurrent::run(
                            boost::bind( &PerformanceDataManager::processMetricView<Statement>, this,
                                         boost::cref(collectors), boost::cref(all_threads), boost::cref(interval), boost::cref(clusteringCriteriaName), metricName ) );
                synchronizer.addFuture( futures[ futuresKey ] );
            }

            else if ( viewName == QStringLiteral("LinkedObjects") ) {
                futures[ futuresKey ] = QtConcurrent::run(
                            boost::bind( &PerformanceDataManager::processMetricView<LinkedObject>, this,
                                         boost::cref(collectors), boost::cref(all_threads), boost::cref(interval), boost::cref(clusteringCriteriaName), metricName ) );
                synchronizer.addFuture( futures[ futuresKey ] );
            }

            else if ( viewName == QStringLiteral("Loops") ) {
                futures[ futuresKey ] = QtConcurrent::run(
                            boost::bind( &PerformanceDataManager::processMetricView<Loop>, this,
                                         boost::cref(collectors), boost::cref(all_threads), boost::cref(interval), boost::cref(clusteringCriteriaName), metricName ) );
                synchronizer.addFuture( futures[ futuresKey ] );
            }

            else if ( viewName == QStringLiteral("CallTree") && collectors.size() > 0 ) {
                const Collector collector = *collectors.begin();
                futures[ futuresKey ] = QtConcurrent::run( this, &PerformanceDataManager::processCalltreeView, collector, all_threads, interval, boost::cref(clusteringCriteriaName) );
                synchronizer.addFuture( futures[ futuresKey ] );
            }
        }
    }
}

/**
 * @brief PerformanceDataManager::unloadCudaViews
 * @param clusteringCriteriaName - the clustering criteria name
 * @param clusterNames - the list of associated clusters
 *
 * Unload all CUDA views for the specified clusters in the clustering criteria.
 */
void PerformanceDataManager::unloadCudaViews(const QString &clusteringCriteriaName, const QStringList &clusterNames)
{
    if ( m_renderer ) {
        m_renderer->unloadCudaViews( clusteringCriteriaName, clusterNames );
    }

    if ( m_tableViewInfo.contains( clusteringCriteriaName ) ) {
        m_tableViewInfo.remove( clusteringCriteriaName );
    }

    if ( m_experiments.contains( clusteringCriteriaName ) ) {
        m_experiments.remove( clusteringCriteriaName );
    }

    foreach( const QString& clusterName, clusterNames ) {
        emit removeCluster( clusteringCriteriaName, clusterName );
    }

    ArgoNavis::CUDA::resetThreadMap();
}

/**
 * @brief PerformanceDataManager::getPerformanceData
 * @param collector - the collector object
 * @param all_threads - all threads known by the cuda collector
 * @param threadSet - indicates whether a given thread name should be considered (=true) or not (=false)
 * @param threads - the set of threads desired
 * @param data - the CUDA performance data object built from the the set of threads desired
 * @return - whether the collector is a CUDA collector
 *
 * This routine loads the specified set of thread performance data into the CUDA performance data object.
 */
bool PerformanceDataManager::getPerformanceData(const Collector& collector,
                                                const ThreadGroup& all_threads,
                                                const QMap< Base::ThreadName, bool >& threadSet,
                                                QMap< Base::ThreadName, Thread>& threads,
                                                CUDA::PerformanceData& data)
{
    bool hasCudaCollector( "cuda" == collector.getMetadata().getUniqueId() );

    for (ThreadGroup::const_iterator i = all_threads.begin(); i != all_threads.end(); ++i) {
        Base::ThreadName thread = ConvertToArgoNavis(*i);
        if ( threadSet.value( thread, false ) ) {
            if ( hasCudaCollector ) {
                GetCUDAPerformanceData( collector, *i, data );
            }
            threads.insert( thread, *i );
        }
    }

    return hasCudaCollector;
}

/**
 * @brief PerformanceDataManager::getThreadGroupFromSelectedClusters
 * @param clusteringCriteriaName - the clustering criteria name
 * @param group - the superset of threads
 * @param threadGroup - the subset of threads currently selected
 *
 * This method returns the subset of threads currently selected.
 */
void PerformanceDataManager::getThreadGroupFromSelectedClusters(const QString &clusteringCriteriaName, const ThreadGroup &group, ThreadGroup &threadGroup)
{
    QMutexLocker guard( &m_mutex );

    if ( m_selectedClusters.contains( clusteringCriteriaName ) ) {
        const QSet< QString >& selected = m_selectedClusters[ clusteringCriteriaName ];

        for ( ThreadGroup::iterator iter = group.begin(); iter != group.end(); ++iter ) {
            const Thread thread( *iter );
            if ( selected.contains( ArgoNavis::CUDA::getUniqueClusterName( thread ) ) ) {
                threadGroup.insert( thread );
            }
        }
    }
}

/**
 * @brief PerformanceDataManager::getRankSetFromSelectedClusters
 * @param clusteringCriteriaName - the clustering criteria name
 * @param ranks - list of ranks within selected clusters
 *
 * This method returns a list of ranks within the set of selected clusters.
 */
void PerformanceDataManager::getRankSetFromSelectedClusters(const QString &clusteringCriteriaName, QSet< int >& ranks)
{
    QMutexLocker guard( &m_mutex );

    if ( m_selectedClusters.contains( clusteringCriteriaName ) ) {
        const QSet< QString >& selected = m_selectedClusters[ clusteringCriteriaName ];
        foreach( const QString& name, selected ) {
            const QString section = name.section( '-', 2, 2 );
            if ( section.startsWith('r') ) {
                ranks.insert( section.mid(1).toInt() );
            }
        }
    }
}

/**
 * @brief PerformanceDataManager::getHostSetFromSelectedClusters
 * @param clusteringCriteriaName - the clustering criteria name
 * @param hosts - list of hosts within selected clusters
 *
 * This method returns a list of hosts within the set of selected clusters.
 */
void PerformanceDataManager::getHostSetFromSelectedClusters(const QString &clusteringCriteriaName, QSet< QString >& hosts)
{
    QMutexLocker guard( &m_mutex );

    if ( m_selectedClusters.contains( clusteringCriteriaName ) ) {
        const QSet< QString >& selected = m_selectedClusters[ clusteringCriteriaName ];
        foreach( const QString& name, selected ) {
            hosts.insert( name.section( '-', 0, 0 ) );
        }
    }
}

/**
 * @brief PerformanceDataManager::getProcessIdSetFromSelectedClusters
 * @param clusteringCriteriaName - the clustering criteria name
 * @param pids - list of process ids within selected clusters
 *
 * This method returns a list of process ids within the set of selected clusters.
 */
void PerformanceDataManager::getProcessIdSetFromSelectedClusters(const QString &clusteringCriteriaName, QSet< pid_t >& pids)
{
    QMutexLocker guard( &m_mutex );

    if ( m_selectedClusters.contains( clusteringCriteriaName ) ) {
        const QSet< QString >& selected = m_selectedClusters[ clusteringCriteriaName ];
        foreach( const QString& name, selected ) {
            const QString section = name.section( '-', 1, 1 );
            if ( section.startsWith('p') ) {
                pids.insert( section.mid(1).toInt() );
            }
        }
    }
}

/**
 * @brief PerformanceDataManager::getThreadGroupListFromSelectedClusters
 * @param clusteringCriteriaName - the clustering criteria name
 * @param compareMode - the specific compare mode
 * @param group - the set of all threads
 * @param threadGroup - the list of ThreadGroups appropriate for the specified compare mode
 *
 * This method returns a list of ThreadGroups appropriate for the specified compare mode.
 */
void PerformanceDataManager::getListOfThreadGroupsFromSelectedClusters(const QString &clusteringCriteriaName, const QString &compareMode, const ThreadGroup &group, QList<ThreadGroup> &threadGroupList)
{
    if ( compareMode == QStringLiteral("Compare") ) {
        ThreadGroup threadGroup;
        getThreadGroupFromSelectedClusters( clusteringCriteriaName, group, threadGroup );
        foreach( Thread thread, threadGroup ) {
            ThreadGroup tempGroup;
            tempGroup.insert( thread );
            threadGroupList.append( tempGroup );
        }
    }
    else if ( compareMode == QStringLiteral("Compare By Rank") ) {
        // get set of selected ranks from individual experiment components current selected
        QSet< int > selectedRanks;
        getRankSetFromSelectedClusters( clusteringCriteriaName, selectedRanks );
        // for each selected rank in the set of selected ranks
        foreach( int rank, selectedRanks ) {
            ThreadGroup tempGroup;  // accumulated matching Threads
            // examine each Thread in the specified ThreadGroup object looking for ones matching the rank
            foreach( Thread thread, group ) {
                bool hasRank;
                int threadRank;
                std::tie( hasRank, threadRank ) = thread.getMPIRank();
                if ( hasRank && threadRank == rank ) {
                    tempGroup.insert( thread );  // insert match into the temporary ThreadGroup object
                }
            }
            // insert temporary ThreadGroup object into the return list
            threadGroupList.append( tempGroup );
        }
    }
    else if ( compareMode == QStringLiteral("Compare By Host") ) {
        // get set of selected hosts from individual experiment components current selected
        QSet< QString > selectedHosts;
        getHostSetFromSelectedClusters( clusteringCriteriaName, selectedHosts );
        // for each selected rank in the set of selected ranks
        foreach( const QString& hostname, selectedHosts ) {
            ThreadGroup tempGroup;  // accumulated matching Threads
            // examine each Thread in the specified ThreadGroup object looking for ones matching the rank
            foreach( Thread thread, group ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
                QString clusterName = QString::fromStdString( thread.getHost() );
#else
                QString clusterName = QString( thread.getHost().c_str() );
#endif
#ifdef HAS_STRIP_DOMAIN_NAME
                int index = clusterName.indexOf( '.' );
                if ( index > 0 )
                    clusterName = clusterName.left( index );
#endif
                if ( clusterName == hostname ) {
                    tempGroup.insert( thread );  // insert match into the temporary ThreadGroup object
                }
            }
            // insert temporary ThreadGroup object into the return list
            threadGroupList.append( tempGroup );
        }
    }
    else if ( compareMode == QStringLiteral("Compare By Process") ) {
        // get set of selected process ids from individual experiment components current selected
        QSet< pid_t > selectedPids;
        getProcessIdSetFromSelectedClusters( clusteringCriteriaName, selectedPids );
        // for each selected process in the set of selected process ids
        foreach( pid_t pid, selectedPids ) {
            ThreadGroup tempGroup;  // accumulated matching Threads
            // examine each Thread in the specified ThreadGroup object looking for ones matching the rank
            foreach( Thread thread, group ) {
                pid_t tpid = thread.getProcessId();
                if ( tpid == pid ) {
                    tempGroup.insert( thread );  // insert match into the temporary ThreadGroup object
                }
            }
            // insert temporary ThreadGroup object into the return list
            threadGroupList.append( tempGroup );
        }
    }
}

/**
 * @brief PerformanceDataManager::getColumnNameForCompareView
 * @param compareMode - the specific compare mode
 * @param thread - thread instance used to get column name information
 * @return - the column name
 *
 * This method constructs the column name for the corresponding ThreadGroup (using representative Thread).
 */
QString PerformanceDataManager::getColumnNameForCompareView(const QString& compareMode, const Thread& thread)
{
    QString columnName;

    if ( QStringLiteral("Compare") == compareMode ) {
        // build column name from unique cluster name value
        columnName = ArgoNavis::CUDA::getUniqueClusterName( thread );
    }
    else if ( QStringLiteral("Compare By Rank") == compareMode ) {
        bool found;
        int rank;
        std::tie( found, rank ) = thread.getMPIRank();
        columnName = QString("%1 %2").arg( found ? QStringLiteral("-r") : tr("Group") ).arg( rank );
    }
    else if ( QStringLiteral("Compare By Host") == compareMode ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString clusterName = QString::fromStdString( thread.getHost() );
#else
        QString clusterName = QString( thread.getHost().c_str() );
#endif
#ifdef HAS_STRIP_DOMAIN_NAME
        int index = clusterName.indexOf( '.' );
        if ( index > 0 )
            clusterName = clusterName.left( index );
#endif
        columnName = QString("-h %1").arg( clusterName );
    }
    else if ( QStringLiteral("Compare By Process") == compareMode ) {
        pid_t pid = thread.getProcessId();
        columnName = QString("-p %1").arg( pid );
    }

    return columnName;
}

/**
 * @brief PerformanceDataManager::loadCudaView
 * @param experimentName - experiment database filename
 * @param clusteringCriteriaName - the clustering criteria name
 * @param collector - a reference to the cuda collector
 * @param all_threads - all threads known by the cuda collector
 *
 * Parse the requested CUDA performance data to metric model data.
 */
void PerformanceDataManager::loadCudaView(const QString& experimentName, const QString& clusteringCriteriaName, const Collector& collector, const ThreadGroup& all_threads)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaView STARTED!!";
#endif

    if ( ! m_tableViewInfo.contains( clusteringCriteriaName ) )
        return;

    QMap< Base::ThreadName, bool > flags;

    // initialize all thread flags to true in order to add all threads to PerformanceData object instance
    for (ThreadGroup::const_iterator i = all_threads.begin(); i != all_threads.end(); ++i) {
        flags.insert( ConvertToArgoNavis(*i), true );
    }

    CUDA::PerformanceData data;
    QMap< Base::ThreadName, Thread> threads;

    bool hasCudaCollector = getPerformanceData( collector, all_threads, flags, threads, data );

    // set default metric view
    emit signalSetDefaultMetricView( hasCudaCollector ? TIMELINE_VIEW : CALLTREE_VIEW, true, false );

    // reset all thread flags to false
    QMutableMapIterator< Base::ThreadName, bool > mapiter( flags );
    while( mapiter.hasNext() ) {
        mapiter.next();
        mapiter.value() = false;
    }

    MetricTableViewInfo& info = m_tableViewInfo[ clusteringCriteriaName ];

    // Determine full time interval extent of this experiment
    Extent extent = info.getExtent();
    // Get full experiment time interval
    TimeInterval experiment_interval = extent.getTimeInterval();
    // Compute time origin
    const double time_origin = experiment_interval.getBegin().getValue();
#ifdef HAS_TEST_DATA_RANGE_CONSTRAINT
    // Get experiment start time
    const Time stime = experiment_interval.getBegin();
    const Time etime = experiment_interval.getEnd();
    // Get part of experiment time interval to actual process (may be have user constraint)
    //TimeInterval interval = TimeInterval( stime + experiment_interval.getWidth() / 32, stime + experiment_interval.getWidth() / 16 );
    //TimeInterval interval = TimeInterval( etime - experiment_interval.getWidth() / 32, etime );
    TimeInterval interval = TimeInterval( etime - experiment_interval.getWidth() / 2, etime );
#else
    TimeInterval interval = info.getInterval();
#endif

    const double lower = ( interval.getBegin().getValue() - time_origin ) / 1000000.0;
    const double upper = ( interval.getEnd().getValue() - time_origin ) / 1000000.0;

    QVector< QString > sampleCounterNames;
    QSet< int > gpuCounterIndexes;

    for ( std::vector<std::string>::size_type i = 0; i < data.counters().size(); ++i ) {
#if defined(HAS_REAL_SAMPLE_COUNTER_NAME)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString sampleCounterName = QString::fromStdString( data.counters()[i] );
#else
        QString sampleCounterName( data.counters()[i].c_str() );
#endif
        sampleCounterNames << sampleCounterName;
#else
#ifdef HAS_METRIC_TYPES
        std::string displayNameStr = ArgoNavis::CUDA::stringify<>( ArgoNavis::CUDA::CounterName( data.counters()[i].name ) );
#else
        std::string displayNameStr = ArgoNavis::CUDA::stringify<>( ArgoNavis::CUDA::CounterName( data.counters()[i] ) );
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString displayName = QString::fromStdString( displayNameStr );
#else
        QString displayName( displayNameStr.c_str() );
#endif
#endif
        sampleCounterNames << displayName;
        if ( displayName.contains( QStringLiteral("GPU") ) )
            gpuCounterIndexes.insert( i );
    }

    data.visitThreads( boost::bind( &PerformanceDataManager::hasCudaEvents, instance(), boost::cref(data), boost::cref(gpuCounterIndexes), _1, boost::ref(flags) ) );

#if 0
    if ( sampleCounterNames.isEmpty() ) {
        sampleCounterNames << "Default";
    }
#endif

    QVector< QString > clusterNames;

#if 0
    for( int i=0; i<threads.size(); ++i ) {
        clusterNames << tr("Group of %1 Thread").arg(1);
    }
#else
    QMap< Base::ThreadName, Thread>::iterator iter( threads.begin() );
    QVector< bool > isGpuSampleCounters;
    QMap< QString, bool > isGpuSampleCounterPercentage;
    while ( iter != threads.end() ) {
        Thread thread( *iter );
        QString hostName = ArgoNavis::CUDA::getUniqueClusterName( thread );
        // Due to 4 Oct 2016 change by Bill to move CUPTI metrics and events collection to a separate thread, there are two threads having the same hostname.
        // Determine the GPU thread from the thread set generated earlier
        clusterNames << hostName;
        Base::ThreadName threadName( iter.key() );
        std::vector< uint64_t> counterValues( data.counts( threadName, data.interval() ) );
        bool hasGpuCounters( false );
        bool hasGpuPercentageCounter( false );
        for ( std::size_t i=0; i<counterValues.size() && ! hasGpuCounters; i++ ) {
            hasGpuCounters |= ( ( counterValues[i] != 0 ) && gpuCounterIndexes.contains(i) );
#ifdef HAS_METRIC_TYPES
            if ( hasGpuCounters && gpuCounterIndexes.contains(i) ) {
                hasGpuPercentageCounter |= ( data.counters()[i].kind == CUDA::kPercentage );
            }
#endif
        }
        isGpuSampleCounters << hasGpuCounters;
        isGpuSampleCounterPercentage[ hostName ] = hasGpuPercentageCounter;
        ++iter;
    }
#endif

    emit addExperiment( experimentName, clusteringCriteriaName, clusterNames, isGpuSampleCounters, sampleCounterNames );

    if ( hasCudaCollector ) {
        m_renderer->setPerformanceData( clusteringCriteriaName, clusterNames, data );

        foreach( const QString& clusterName, clusterNames ) {
            bool hasGpuPercentageCounter( isGpuSampleCounterPercentage.contains(clusterName) && isGpuSampleCounterPercentage[clusterName] );
            emit addCluster( clusteringCriteriaName, clusterName, lower, upper, false, 0.0, hasGpuPercentageCounter ? 100.0 : -1.0 );
        }

        data.visitThreads( boost::bind(
                               &PerformanceDataManager::processPerformanceData, instance(),
                               boost::cref(data), _1, boost::cref(gpuCounterIndexes), boost::cref(clusteringCriteriaName) ) );

        foreach( const QString& clusterName, clusterNames ) {
            emit setMetricDuration( clusteringCriteriaName, clusterName, lower, upper );
        }

        std::vector<CUDA::Device> devices;

        for ( std::vector<CUDA::Device>::size_type i = 0; i < data.devices().size(); ++i ) {
            const CUDA::Device& device = data.devices()[i];

            int definedDevice = -1;

            for (std::size_t d=0; d<devices.size(); d++ ) {
                if ( devices[d] == device ) {
                    definedDevice = d;
                    break;
                }
            }

            NameValueList attributes;
            NameValueList maximumLimits;

            if ( definedDevice == -1 ) {
                definedDevice = devices.size();
                devices.push_back( device );

                // build attribute name/values list
                attributes << qMakePair( QStringLiteral("Name"), QString(device.name.c_str()) );
                attributes << qMakePair( QStringLiteral("ComputeCapability"),
                                         QString("%1.%2").arg(device.compute_capability.get<0>())
                                         .arg(device.compute_capability.get<1>()) );
                attributes << qMakePair( QStringLiteral("Global Memory Bandwidth"),
                                         QString("%1/sec").arg(ArgoNavis::CUDA::stringify<ArgoNavis::CUDA::ByteCount>(1024ULL * device.global_memory_bandwidth).c_str()) );
                attributes << qMakePair( QStringLiteral("Global Memory Size"),
                                         QString(ArgoNavis::CUDA::stringify<ArgoNavis::CUDA::ByteCount>(device.global_memory_size).c_str()) );
                attributes << qMakePair( QStringLiteral("Constant Memory Size"),
                                         QString(ArgoNavis::CUDA::stringify<ArgoNavis::CUDA::ByteCount>(device.constant_memory_size).c_str()) );
                attributes << qMakePair( QStringLiteral("L2 Cache Size"),
                                         QString(ArgoNavis::CUDA::stringify<ArgoNavis::CUDA::ByteCount>(device.l2_cache_size).c_str()) );
                attributes << qMakePair( QStringLiteral("Threads Per Warp"),
                                         QString(ArgoNavis::CUDA::stringify<>(device.threads_per_warp).c_str()) );
                attributes << qMakePair( QStringLiteral("Core Clock Rate"),
                                         QString(ArgoNavis::CUDA::stringify<ArgoNavis::CUDA::ClockRate>(024ULL * device.core_clock_rate).c_str()) );
                attributes << qMakePair( QStringLiteral("Number of Async Engines"),
                                         QString(ArgoNavis::CUDA::stringify<>(device.memcpy_engines).c_str()) );
                attributes << qMakePair( QStringLiteral("Number of Multiprocessors"),
                                         QString(ArgoNavis::CUDA::stringify<>(device.multiprocessors).c_str()) );

                // build maximum limits name/values list
                maximumLimits << qMakePair( QStringLiteral("Max Grid Dimensions"),
                                            QString("%1x%2x%3").arg(device.max_grid.get<0>())
                                            .arg(device.max_grid.get<1>())
                                            .arg(device.max_grid.get<2>()) );
                maximumLimits << qMakePair( QStringLiteral("Max Block Dimensions"),
                                            QString("%1, %2, %3").arg(device.max_block.get<0>())
                                            .arg(device.max_block.get<1>())
                                            .arg(device.max_block.get<2>()) );
                maximumLimits << qMakePair( QStringLiteral("Max IPC"),
                                            QString(ArgoNavis::CUDA::stringify<>(device.max_ipc).c_str()) );
                maximumLimits << qMakePair( QStringLiteral("Max Warps Per Multiprocessor"),
                                            QString(ArgoNavis::CUDA::stringify<>(device.max_warps_per_multiprocessor).c_str()) );
                maximumLimits << qMakePair( QStringLiteral("Max Blocks Per Multiprocessor"),
                                            QString(ArgoNavis::CUDA::stringify<>(device.max_blocks_per_multiprocessor).c_str()) );
                maximumLimits << qMakePair( QStringLiteral("Max Registers Per Block"),
                                            QString(ArgoNavis::CUDA::stringify<>(device.max_registers_per_block).c_str()) );
                maximumLimits << qMakePair( QStringLiteral("Max Shared Memory Per Block"),
                                            QString(ArgoNavis::CUDA::stringify<>(device.max_shared_memory_per_block).c_str()) );
                maximumLimits << qMakePair( QStringLiteral("Max Threads Per Block"),
                                            QString(ArgoNavis::CUDA::stringify<>(device.max_threads_per_block).c_str()) );
            }

            emit addDevice( i, definedDevice, attributes, maximumLimits );
        }

        // clear temporary data structures used during thread visitation
        m_sampleKeys.clear();
        m_sampleValues.clear();
        m_rawValues.clear();
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaView ENDED!!";
#endif
}

/**
 * @brief sortByFixedComponent
 * @param first - iterator to start sorting from
 * @param last - iterator one past last item to sort
 *
 * Sorts the 'first' to 'last' items in a container.  The item type is a std:tuple.  The Nth value in the std::tuple is used as the key for sorting.
 * The container Iterator must follow the ForwardIterator concept.  The BinaryPredicate concept provides a function used during the sorting.
 */
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
template <int N, typename BinaryPredicate, typename ForwardIterator>
void PerformanceDataManager::sortByFixedComponent(ForwardIterator first, ForwardIterator last) {
    std::sort (first, last, [](const typename ForwardIterator::value_type& x, const typename ForwardIterator::value_type& y)->bool {
        return BinaryPredicate()(std::get<N>(x), std::get<N>(y));
    });
}
#else
template <int N, typename BinaryPredicate, typename ForwardIterator>
bool PerformanceDataManager::ComponentBinaryPredicate(const typename ForwardIterator::value_type& x, const typename ForwardIterator::value_type& y)
{
    return BinaryPredicate()(boost::get<N>(x), boost::get<N>(y));
}
template <int N, typename BinaryPredicate, typename ForwardIterator>
void PerformanceDataManager::sortByFixedComponent(ForwardIterator first, ForwardIterator last)
{
    std::sort( first, last, boost::bind(&PerformanceDataManager::ComponentBinaryPredicate<N, BinaryPredicate, ForwardIterator>, this, _1, _2) );
}
#endif

/**
 * @brief PerformanceDataManager::partition_sort
 * @param function - a reference to the called function
 * @param callingFunctionSet- a reference to the calling function (as an element of a QSet since the calling function may be empty)
 * @param callingLinkedObjectName - name of linked object where calling function resides
 * @param d - entry in stack frame
 * @return - Same caller -> callee relationship? true or false
 *
 * This is an UnaryPredicate operation for std::partition with additional parameters bound using boost::bind.
 * If the stack entry represented by 'd' matches the caller->callee relationship this method returns true;
 * otherwise it returns false.  Thus, std::partition will move all the currently processed caller->callee
 * related stack frames to the top of the container.
 */
bool PerformanceDataManager::partition_sort(const Function& function,
                                            const std::set< Function >& callingFunctionSet,
                                            const all_details_data_t& d)
{
    const Function& func( get<2>(d) );
    const std::set< Function >& callingFuncSet( get<3>(d) );

    if ( ! callingFunctionSet.empty() ) {
        const Function& callingFunction( *callingFunctionSet.begin() );
        if ( ! callingFuncSet.empty() ) {
            const Function& callingFunc( *callingFuncSet.begin() );
            // Same caller -> callee relationship?  This is determined by comparing the caller function and
            // linked object name and callee function and linked object name to the current function and linked
            // object name of the current item in the caller function list being processed.
            return ( ( func == function ) && ( callingFunction == callingFunc ) );
        }
        return false;
    }

    return callingFuncSet.empty();
}

/**
 * @brief PerformanceDataManager::detail_reduction
 * @param caller_function_list - the set of all direct calls (caller -> function) involved in the view
 * @param call_depth_map - a map of the calltree depth from "_start" to a particular function
 * @param all_details - an array of raw detail data
 * @param callPairToWeightMap - weights for all call pairs
 * @param reduced_details - an array of reduced detail data
 *
 * This method combines individual detail records for each function call pairs into one detail record.
 * The std::partition algorithm is used to move all records associated with the current function call pair
 * being processed to the front of the container and returns an iterator to the end of the group of items
 * moved to the front.  The front is initially the beginning of the raw detail array and after completing
 * the reduction for each function call pairs the front becomes the iterator returned by std::partition.
 * The UnaryPredicate function used by std::partition determines whether an evaluated item should be moved
 * to the front by comparing the caller function and linked object name and called function and linked object
 * name to the current function and linked object name in the caller function list being processed.
 * The reduced details container is sorted in descending order by time (the second item in the std::tuple).
 */
void PerformanceDataManager::detail_reduction(
        const ArgoNavis::GUI::PerformanceDataManager::FunctionSet &caller_function_list,
        std::map< Function, uint32_t >& call_depth_map,
        TALLDETAILS& all_details,
        CallPairToWeightMap& callPairToWeightMap,
        TDETAILS& reduced_details)
{
    TALLDETAILS::iterator siter = all_details.begin();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    using namespace std;
#else
    using namespace boost;
#endif
    for ( std::set< tuple< std::set< Function >, Function > >::iterator fit = caller_function_list.begin(); fit != caller_function_list.end(); fit++) {
        const tuple< std::set< Function >, Function > elem( *fit );

        // getting called function name and linked object name
        Function function = get<1>(elem);

        // get call depth
        uint32_t depth = ( call_depth_map.find( function ) != call_depth_map.end() ) ? call_depth_map[ function ] : 0;

        // getting caller function name and linked object name
        std::set< Function > caller = get<0>(elem);

        // partition remaining raw details per caller->callee relationships for current caller being processed in the caller function list
        TALLDETAILS::iterator eiter = std::partition(
                    siter, all_details.end(),
                    boost::bind( &PerformanceDataManager::partition_sort, this,
                                 boost::cref(function), boost::cref(caller), _1 ) );

        // exit when reached end of raw details
        if ( siter == all_details.end() )
            break;

        // sum the count and time values
        int sum_count(0);
        double sum_time(0.0);
        for ( TALLDETAILS::iterator it = siter; it != eiter; it++ ) {
            all_details_data_t d( *it );
            sum_count += get<0>(d);
            sum_time += get<1>(d);
        }

        // save the reduced details data
        reduced_details.push_back( make_tuple( sum_count, sum_time, function, depth ) );

        // add entry to the call pair to weight map
        if ( ! caller.empty() ) {
            const Function callingFunction( *(caller.begin()) );
            callPairToWeightMap[ std::make_pair( callingFunction, function ) ] = sum_time;
        }

        // set start iterator to the end iterator
        siter = eiter;
    }

    // Sort the reduced details by time (the second item in the std::tuple - index value of 1).
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    sortByFixedComponent<1, std::greater<std::tuple_element<1,details_data_t>::type>>( reduced_details.begin(), reduced_details.end() );
#else
    sortByFixedComponent<1, std::greater< boost::tuples::element<1,details_data_t>::type > >( reduced_details.begin(), reduced_details.end() );
#endif
}

/**
 * @brief PerformanceDataManager::generate_calltree_graph
 * @param graphManager - the calltree graph manager instance in which to store the vertices (functions) and edges representing the call pairs
 * @param functions - the set of functions involved in the calltree view
 * @param caller_function_list - the set of all direct calls (caller -> function) involved in the calltree view
 * @param call_depth_map - a map of the calltree depth from "_start" to a particular function
 * @param callPairToEdgeMap - a map of the function call pairs to the edge handle of the edge linking them in the calltree graph
 *
 * This method constructs a graph using a CalltreeGraphManager class instannce.  Each function is added as a vertex to the graph.  Each calling -> callee
 * function pair results in the associated edge to be added to the graph.  The calltree depth map and DOT formatted output is returned by this method.
 */
void PerformanceDataManager::generate_calltree_graph(
        CalltreeGraphManager& graphManager,
        const std::set< Framework::Function >& functions,
        const ArgoNavis::GUI::PerformanceDataManager::FunctionSet &caller_function_list,
        std::map< Function, uint32_t >& call_depth_map,
        CallPairToEdgeMap& callPairToEdgeMap)
{
    // Define fast lookup data structures from Function type to CalltreeGraphManager::handle_t type and vice-versa.
    std::map< Function, CalltreeGraphManager::handle_t > mapFunctionToHandle;
    std::vector< std::set< Function > > mapHandleToFunction( functions.size() );

    // Create a vertex in the graph for each function
    foreach ( const Framework::Function& function, functions ) {
        CalltreeGraphManager::handle_t handle = graphManager.addFunctionNode( function.getName(), std::string(), 0, function.getLinkedObject().getPath().getBaseName() );
        // Add items to the lookup data structures for the function/handle pair
        mapFunctionToHandle[ function ] = handle;
        mapHandleToFunction[ handle ].insert( function );
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    using namespace std;
#else
    using namespace boost;
#endif

    // Create an edge for each caller->callee function pair
    // NOTE: Initial edge weight will be one so that the call depths can be computed using the boost::johnson_all_pairs_shortest_paths algorithm.
    for ( std::set< tuple< std::set< Function >, Function > >::iterator fit = caller_function_list.begin(); fit != caller_function_list.end(); fit++ ) {
        const tuple< std::set< Function >, Function > elem( *fit );

        std::set< Function > caller = get<0>(elem);
        if ( caller.empty() )
            continue;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        const Function callingFunction( *(caller.cbegin()) );
#else
        const Function callingFunction( *(caller.begin()) );
#endif
        Function function = get<1>(elem);

        if ( mapFunctionToHandle.find( callingFunction ) != mapFunctionToHandle.end() && mapFunctionToHandle.find( function ) != mapFunctionToHandle.end() ) {
            const CalltreeGraphManager::handle_t& caller_handle = mapFunctionToHandle.at( callingFunction );
            const CalltreeGraphManager::handle_t& handle = mapFunctionToHandle.at( function );
            callPairToEdgeMap[ std::make_pair( callingFunction, function ) ] = graphManager.addCallEdge( caller_handle, handle );
        }
    }

    // Find the calltree depths for each function
    std::map< std::pair< CalltreeGraphManager::handle_t, CalltreeGraphManager::handle_t>, uint32_t > depth_map;  // maps pair (caller_handle. handle) to calltree depth

    graphManager.generate_call_depths( depth_map );

    // Convert the handle pairs to Function pairs
    const std::string startFunction = "_start";

    for ( std::map< std::pair< CalltreeGraphManager::handle_t, CalltreeGraphManager::handle_t>, uint32_t >::iterator iter = depth_map.begin(); iter != depth_map.end(); iter++ ) {
        std::pair< CalltreeGraphManager::handle_t, CalltreeGraphManager::handle_t > callpath( iter->first );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        const Function caller( *mapHandleToFunction[ callpath.first ].cbegin() );
#else
        const Function caller( *mapHandleToFunction[ callpath.first ].begin() );
#endif
        if ( caller.getName() != startFunction || callpath.second >= mapHandleToFunction.size() )
            continue;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        const Function function( *mapHandleToFunction[ callpath.second ].cbegin() );
#else
        const Function function( *mapHandleToFunction[ callpath.second ].begin() );
#endif
        call_depth_map[ function ] = iter->second;
    }
}

/**
 * @brief PerformanceDataManager::getDetailTotals
 * @param detail - the OpenSpeedShop::Framework::CUDAExecDetail instance
 * @param factor - the time factor
 * @return - a std::pair formed from the 'factor' parameter and the time from the 'detail' instance scaled by the 'factor'
 *
 * This is a template specialization for PerformanceDataManager::getDetailTotal which usually works on a type required to have the two public member variables 'dm_count'
 * and 'dm_time'.  This template specialization works with the OpenSpeedShop::Framework::CUDAExecDetail class which doesn't satisfy this requirement as it doesn't have the
 * 'dm_count' member variable and 'dm_time' isn't public but is accessed via a public getter method 'getTime()'.  Thus, this template specialization works for the special case
 * of the OpenSpeedShop::Framework::CUDAExecDetail implementation.
 */
template <>
std::pair< uint64_t, double > PerformanceDataManager::getDetailTotals(const std::vector< Framework::CUDAExecDetail >& detail, const double factor)
{
    double sum = std::accumulate( detail.begin(), detail.end(), 0.0, [](double sum, const Framework::CUDAExecDetail& d) {
        return sum + d.getTime();
    } );

    return std::make_pair( factor, sum / factor * 1000.0 );
}

/**
 * @brief PerformanceDataManager::getDetailTotals
 * @param detail - the OpenSpeedShop::Framework::MPIDetail instance
 * @param factor - the time factor
 * @return - a std::pair formed from the 'factor' parameter and the time from the 'detail' instance scaled by the 'factor'
 *
 * This is a template specialization for PerformanceDataManager::getDetailTotal which usually works on a type required to have the two public member variables 'dm_count'
 * and 'dm_time'.  This template specialization works with the OpenSpeedShop::Framework::MPIDetail class which doesn't satisfy this requirement as it doesn't have the
 * 'dm_count' member variable although it does has a pubic 'dm_time' member variable.  Thus, this template specialization works for the special case
 * of the OpenSpeedShop::Framework::MPIDetail implementation.
 */
template <>
std::pair< uint64_t, double > PerformanceDataManager::getDetailTotals(const std::vector< Framework::MPIDetail >& detail, const double factor)
{
    double sum = std::accumulate( detail.begin(), detail.end(), 0.0, [](double sum, const Framework::MPIDetail& d) {
        return sum + d.dm_time;
    } );

    return std::make_pair( factor, sum / factor * 1000.0 );
}

/**
 * @brief PerformanceDataManager::getDetailTotals
 * @param detail - the OpenSpeedShop::Framework::MPIDetail instance
 * @param factor - the time factor
 * @return - a std::pair formed from the 'factor' parameter and the time from the 'detail' instance scaled by the 'factor'
 *
 * This is a template specialization for PerformanceDataManager::getDetailTotal which usually works on a type required to have the two public member variables 'dm_count'
 * and 'dm_time'.  This template specialization works with the OpenSpeedShop::Framework::PthreadsDetail class which doesn't satisfy this requirement as it doesn't have the
 * 'dm_count' member variable although it does has a pubic 'dm_time' member variable.  Thus, this template specialization works for the special case
 * of the OpenSpeedShop::Framework::PthreadsDetail implementation.
 */
template <>
std::pair< uint64_t, double > PerformanceDataManager::getDetailTotals(const std::vector< Framework::PthreadsDetail >& detail, const double factor)
{
    double sum = std::accumulate( detail.begin(), detail.end(), 0.0, [](double sum, const Framework::PthreadsDetail& d) {
        return sum + d.dm_time;
    } );

    return std::make_pair( factor, sum / factor * 1000.0 );
}

/**
 * @brief PerformanceDataManager::getDetailTotals
 * @param detail - the OpenSpeedShop::Framework::MPIDetail instance
 * @param factor - the time factor
 * @return - a std::pair formed from the 'factor' parameter and the time from the 'detail' instance scaled by the 'factor'
 *
 * This is a template specialization for PerformanceDataManager::getDetailTotal which usually works on a type required to have the two public member variables 'dm_count'
 * and 'dm_time'.  This template specialization works with the OpenSpeedShop::Framework::MPIDetail class which doesn't satisfy this requirement as it doesn't have the
 * 'dm_count' member variable although it does has a pubic 'dm_time' member variable.  Thus, this template specialization works for the special case
 * of the OpenSpeedShop::Framework::MPITDetail implementation.
 */
template <>
std::pair< uint64_t, double > PerformanceDataManager::getDetailTotals(const std::vector<Framework::MPITDetail>& detail, const double factor)
{
    double sum = std::accumulate( detail.begin(), detail.end(), 0.0, [](double sum, const Framework::MPITDetail& d) {
        return sum + d.dm_time;
    } );

    return std::make_pair( factor, sum / factor * 1000.0 );
}

/**
 * @brief PerformanceDataManager::getDetailTotals
 * @param detail - the OpenSpeedShop::Framework::IODetail instance
 * @param factor - the time factor
 * @return - a std::pair formed from the 'factor' parameter and the time from the 'detail' instance scaled by the 'factor'
 *
 * This is a template specialization for PerformanceDataManager::getDetailTotal which usually works on a type required to have the two public member variables 'dm_count'
 * and 'dm_time'.  This template specialization works with the OpenSpeedShop::Framework::IODetail class which doesn't satisfy this requirement as it doesn't have the
 * 'dm_count' member variable although it does has a pubic 'dm_time' member variable.  Thus, this template specialization works for the special case
 * of the OpenSpeedShop::Framework::IODetail implementation.
 */
template <>
std::pair< uint64_t, double > PerformanceDataManager::getDetailTotals(const std::vector<Framework::IODetail>& detail, const double factor)
{
    double sum = std::accumulate( detail.begin(), detail.end(), 0.0, [](double sum, const Framework::IODetail& d) {
        return sum + d.dm_time;
    } );

    return std::make_pair( factor, sum / factor * 1000.0 );
}

/**
 * @brief PerformanceDataManager::getDetailTotals
 * @param detail - the OpenSpeedShop::Framework::IOTDetail instance
 * @param factor - the time factor
 * @return - a std::pair formed from the 'factor' parameter and the time from the 'detail' instance scaled by the 'factor'
 *
 * This is a template specialization for PerformanceDataManager::getDetailTotal which usually works on a type required to have the two public member variables 'dm_count'
 * and 'dm_time'.  This template specialization works with the OpenSpeedShop::Framework::IOTDetail class which doesn't satisfy this requirement as it doesn't have the
 * 'dm_count' member variable although it does has a pubic 'dm_time' member variable.  Thus, this template specialization works for the special case
 * of the OpenSpeedShop::Framework::IOTDetail implementation.
 */
template <>
std::pair< uint64_t, double > PerformanceDataManager::getDetailTotals(const std::vector<Framework::IOTDetail>& detail, const double factor)
{
    double sum = std::accumulate( detail.begin(), detail.end(), 0.0, [](double sum, const Framework::IOTDetail& d) {
        return sum + d.dm_time;
    } );

    return std::make_pair( factor, sum / factor * 1000.0 );
}

/**
 * @brief PerformanceDataManager::ShowCalltreeDetail
 * @param collector - the experiment collector used for the calltree view
 * @param threadGroup - the set of threads applicable to the calltree view
 * @param interval - the time interval for the calltree view
 * @param functions - the set of functions involved in the calltree view
 * @param metric - the metric computed in the calltree view
 * @param metricDesc - the metric table column headers for the calltree view
 * @param clusteringCriteriaName - the clustering criteria name
 *
 * This method computes the data for the calltree view in accordance with the various contraints for the view -
 * set of threads, set of functions, time interval and the metric name.
 */
template <typename DETAIL_t>
void PerformanceDataManager::ShowCalltreeDetail(const Framework::Collector& collector,
        const Framework::ThreadGroup& threadGroup,
        const Framework::TimeInterval& interval,
        const std::set<Function> functions,
        const QString metric,
        const QStringList metricDesc,
        const QString &clusteringCriteriaName)
{
    // bet view name
    const QString viewName = getViewName<DETAIL_t>();

    emit addMetricView( clusteringCriteriaName, viewName, viewName, metricDesc );

    SmartPtr< std::map< Function,
                std::map< Framework::Thread,
                    std::map< Framework::StackTrace, DETAIL_t > > > > raw_items;

    Queries::GetMetricValues( collector, metric.toStdString(), interval, threadGroup, functions,  // input - metric search criteria
                              raw_items );                                                        // output - raw metric values

    SmartPtr< std::map<Function, std::map<Framework::StackTrace, DETAIL_t > > > data =
            Queries::Reduction::Apply( raw_items, Queries::Reduction::Summation );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    typedef std::tuple< std::set< Function >, Function > CallerCallee_t;
#else
    typedef boost::tuple< std::set< Function >, Function > CallerCallee_t;
#endif

    TALLDETAILS all_details;
    std::set< CallerCallee_t > caller_function_list;

    for ( typename std::map< Function, std::map< Framework::StackTrace, DETAIL_t > >::iterator iter = data->begin(); iter != data->end(); iter++ ) {
        const Framework::Function& function( iter->first );

        const std::map< Framework::StackTrace, DETAIL_t >& tracemap( iter->second );

        std::map< Framework::Thread, Framework::ExtentGroup > subextents_map;
        Get_Subextents_To_Object_Map( threadGroup, function, subextents_map );

        std::set< Framework::StackTrace, ltST > StackTraces_Processed;

        for ( typename std::map< Framework::StackTrace, DETAIL_t >::const_iterator siter = tracemap.begin(); siter != tracemap.end(); siter++ ) {
            const Framework::StackTrace& stacktrace( siter->first );

            std::pair< std::set< Framework::StackTrace >::iterator, bool > ret = StackTraces_Processed.insert( stacktrace );
            if ( ! ret.second )
                continue;

            // Find the extents associated with the stack trace's thread.
            std::map< Framework::Thread, Framework::ExtentGroup >::iterator tei = subextents_map.find( stacktrace.getThread() );
            Framework::ExtentGroup subExtents;
            if ( tei != subextents_map.end() ) {
                subExtents = (*tei).second;
            }

            const int64_t num_calls = ( subExtents.begin() == subExtents.end() ) ? 1 : stack_contains_N_calls( stacktrace, subExtents );

            std::size_t index;
            for ( index=0; index<stacktrace.size(); index++ ) {
                std::pair< bool, Function > result = stacktrace.getFunctionAt( index );
                if ( result.first && result.second == function )
                    break;
            }

            std::set< Function > caller;

            if ( index < stacktrace.size()-1 ) {
                std::pair< bool, Function > result = stacktrace.getFunctionAt( index+1 );
                if ( result.first )
                    caller.insert( result.second );
            }

            const DETAIL_t& detail( siter->second );

            // compute the 'count' and 'time' metric for this 'detail' instance
            std::pair< uint64_t, double > results = getDetailTotals( detail, num_calls );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            all_details_data_t detail_tuple = std::make_tuple( results.first, results.second, function, caller );
            CallerCallee_t callerCalleeInfo = std::make_tuple( caller, function ) ;
#else
            all_details_data_t detail_tuple = boost::make_tuple( results.first, results.second, function, caller );
            CallerCallee_t callerCalleeInfo = boost::make_tuple( caller, function ) ;
#endif
            all_details.push_back( detail_tuple );

            caller_function_list.insert( callerCalleeInfo );
        }
    }

    // Define map for Function to calltree depth from "_start" invocation to the Function
    std::map< Function, uint32_t > call_depth_map;

    // Create calltree graph manager instance
    CalltreeGraphManager graphManager;

    // A map of function call-pairs to edge handles
    CallPairToEdgeMap callPairToEdgeMap;

    // A map of function call-pairs to edge weights
    CallPairToWeightMap callPairToWeightMap;

    generate_calltree_graph( graphManager, functions, caller_function_list, call_depth_map, callPairToEdgeMap );

    TDETAILS reduced_details;

    detail_reduction( caller_function_list, call_depth_map, all_details, callPairToWeightMap, reduced_details );

    std::sort( reduced_details.begin(), reduced_details.end(), details_compare );

    CalltreeGraphManager::EdgeWeightMap edgeWeightMap;
    for ( CallPairToEdgeMap::iterator iter = callPairToEdgeMap.begin(); iter != callPairToEdgeMap.end(); iter++ ) {
        edgeWeightMap[ iter->second ] = callPairToWeightMap[ iter->first ];
    }

    graphManager.setEdgeWeights( edgeWeightMap );

    // Generate the DOT formatted data from the graph
    std::ostringstream oss;
    graphManager.write_graphviz( oss );
    emit signalDisplayCalltreeGraph( QString::fromStdString( oss.str() ) );

    for ( TDETAILS::const_reverse_iterator i = reduced_details.rbegin(); i != reduced_details.rend(); ++i ) {
        const details_data_t& d( *i );
        QVariantList metricData;
        std::ostringstream oss;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        metricData << std::get<1>(d) << QVariant::fromValue((long)std::get<0>(d));
        for ( uint32_t i=0; i<std::get<3>(d); ++i ) oss << '>';
        const Function func( std::get<2>(d) );
#else
        metricData << boost::get<1>(d) << QVariant::fromValue((long)boost::get<0>(d));
        for ( uint32_t i=0; i<boost::get<3>(d); ++i ) oss << '>';
        const Function func( boost::get<2>(d) );
#endif
        oss << func.getName() << " (" << func.getLinkedObject().getPath().getBaseName() << ")";
        metricData << QString::fromStdString( oss.str() );
        emit addMetricViewData( clusteringCriteriaName, viewName, viewName, metricData );
    }
}

/**
 * @brief PerformanceDataManager::getTraceMetricValues
 * @param functionName - the function name
 * @param time_origin - the experiment start time
 * @param details - the vector of metric details
 * @param metricData - a vector of trace metrics generated from the metric details
 *
 * This is a template specialization of the getTraceMetricValues template for the OpenSpeedShop::Framework::MPITDetail typename.
 * This method generates a vector of trace metrics from the metric details.
 */
template <>
void PerformanceDataManager::getTraceMetricValues(const QString& functionName, const double time_origin, const std::vector<MPITDetail>& details, QVector< QVariantList >& metricData)
{
    for ( std::vector<MPITDetail>::const_iterator iter = details.begin(); iter != details.end(); iter++ ) {
        const MPITDetail& detail( *iter );

        const double lower = ( detail.dm_interval.getBegin().getValue() / 1000000.0 ) - time_origin;
        const double upper = ( detail.dm_interval.getEnd().getValue() / 1000000.0 ) - time_origin;
        const double time_in_call = detail.dm_time * 1000.0;

        metricData.push_back( QVariantList() << functionName << lower << upper << time_in_call << detail.dm_source << detail.dm_destination << QVariant::fromValue<long>(detail.dm_size) << detail.dm_retval << detail.dm_id.first );
    }
}

/**
 * @brief PerformanceDataManager::getTraceMetrics<std::vector<MPITDetail>>
 * @return - the trace metrics (names of columns for the MPI trace view).
 *
 * This is a template specialization of the getTraceMetrics template for the OpenSpeedShop::Framework::MPITDetail typename.
 * The function returns the names of columns for the MPI trace view.
 */
template <>
QStringList PerformanceDataManager::getTraceMetrics<std::vector<MPITDetail>>() const
{
    QStringList metrics;

    metrics << s_functionTitle << tr("Start Time") << tr("End Time") << tr("Duration (ms)")
            << tr("From Rank") << tr("To Rank")
            << tr("Message Size") << tr("Return Value") << tr("Rank");

    return metrics;
}

/*
 * @brief PerformanceDataManager::ShowTraceDetail
 * @param clusteringCriteriaName - the clustering criteria name
 * @param collector - the experiment collector used for the trace view
 * @param threadGroup - the set of threads applicable to the trace view
 * @param time_origin - the start time of the experiment
 * @param lower - the start time of the trace view
 * @param upper - the end time of the trace view
 * @param interval - the time interval for the trace view
 * @param functions - the set of functions involved in the trace view
 * @param metric - the metric computed in the trace view
 *
 * This method computes the data for the trace view in accordance with the various contraints for the view -
 * set of threads, set of functions, time interval and the metric name.
 */
template <typename DETAIL_t>
void PerformanceDataManager::ShowTraceDetail(
        const QString& clusteringCriteriaName,
        const Framework::Collector& collector,
        const Framework::ThreadGroup& threadGroup,
        const double time_origin,
        const double lower,
        const double upper,
        const Framework::TimeInterval& interval,
        const std::set<Function> functions,
        const QString metric)
{
    // get view name
    const QString traceViewName = TRACE_EVENT_DETAILS_METRIC;

    const QStringList metricDesc = getTraceMetrics<DETAIL_t>();

    // for details view emit signal to create just the model
    emit addMetricView( clusteringCriteriaName, traceViewName, ALL_EVENTS_DETAILS_VIEW, metricDesc );

    SmartPtr< std::map< Function,
                std::map< Framework::Thread,
                    std::map< Framework::StackTrace, DETAIL_t > > > > raw_items;

    Queries::GetMetricValues( collector, metric.toStdString(), interval, threadGroup, functions,  // input - metric search criteria
                              raw_items );                                                        // output - raw metric values

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    std::vector< std::tuple< std::string, double, double, double, double, int, int, long, int > > metrics;
#else
    std::vector< boost::tuple< std::string, double, double, double, double, int, int, long, int > > metrics;
#endif

    const QString metricViewName = traceViewName + "-" + ALL_EVENTS_DETAILS_VIEW;

    // build the proxy views and tree views for the various trace views: "All Events"
    // NOTE: functions[0] .. functions[N-10] will be added below
    emit addAssociatedMetricView( clusteringCriteriaName, traceViewName, ALL_EVENTS_DETAILS_VIEW, metricViewName, metricDesc );

    for ( typename std::map< Function, std::map< Framework::Thread, std::map< Framework::StackTrace, DETAIL_t > > >::iterator iter = raw_items->begin(); iter != raw_items->end(); iter++ ) {
        const Framework::Function& function( iter->first );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        const QString functionName = QString::fromStdString( function.getDemangledName() );
#else
        const QString functionName = QString( function.getDemangledName().c_str() );
#endif

        emit addAssociatedMetricView( clusteringCriteriaName, traceViewName, functionName, metricViewName, metricDesc );

        typename std::map< Framework::Thread, std::map< Framework::StackTrace, DETAIL_t > >& thread( iter->second );

        std::map< Framework::Thread, Framework::ExtentGroup > subextents_map;
        Get_Subextents_To_Object_Map( threadGroup, function, subextents_map );

        std::set< Framework::StackTrace, ltST > StackTraces_Processed;

        for ( typename std::map< Framework::Thread, std::map< Framework::StackTrace, DETAIL_t > >::iterator titer = thread.begin(); titer != thread.end(); titer++ ) {
            const typename std::map< Framework::StackTrace, DETAIL_t >& tracemap( titer->second );

            for ( typename std::map< Framework::StackTrace, DETAIL_t >::const_iterator siter = tracemap.begin(); siter != tracemap.end(); siter++ ) {
                const Framework::StackTrace& stacktrace( siter->first );
                const DETAIL_t& details( siter->second );

                std::pair< std::set< Framework::StackTrace >::iterator, bool > ret = StackTraces_Processed.insert( stacktrace );
                if ( ! ret.second )
                    continue;

                QVector< QVariantList > traceList;
                getTraceMetricValues( functionName, time_origin, details, traceList );

                foreach( const QVariantList& list, traceList ) {
                    if ( list.size() == metricDesc.size() ) {
                        emit addTraceItem( clusteringCriteriaName, clusteringCriteriaName, list[0].toString(), list[1].toDouble(), list[2].toDouble(), list[8].toInt() );
                    }
                }

                foreach( const QVariantList& metricData, traceList ) {
                    emit addMetricViewData( clusteringCriteriaName, traceViewName, ALL_EVENTS_DETAILS_VIEW, metricData );
                }
            }
        }

        emit requestMetricViewComplete( clusteringCriteriaName, traceViewName, ALL_EVENTS_DETAILS_VIEW, lower, upper );

        emit requestMetricViewComplete( clusteringCriteriaName, traceViewName, functionName, lower, upper );
    }
}

/*
 * @brief PerformanceDataManager::ShowSampleCountersDetail
 * @param clusteringCriteriaName - the clustering criteria name
 * @param collector - the experiment collector used for the metric view
 * @param threadGroup - the set of threads applicable to the metric view
 * @param lower - the start time of the metric view
 * @param upper - the end time of the metric view
 * @param interval - the time interval for the metric view
 * @param metricName - the metric computed in the metric view
 * @param viewName - the metric view requested
 *
 * This method computes the data for the metric view for sampling experiments in accordance with the
 * various contraints for the view - set of threads, set of functions, time interval and the metric name.
 */
template <typename TS, typename DETAIL_t>
void PerformanceDataManager::ShowSampleCountersDetail(const QString& clusteringCriteriaName,
        const Framework::Collector& collector,
        const Framework::ThreadGroup& threadGroup,
        const double lower,
        const double upper,
        const Framework::TimeInterval& interval,
        const QString metricName,
        const QString viewName)
{
    // get name of sample counter
    std::string name;
    collector.getParameterValue( "event", name );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    const QString sampleCounterName = QString::fromStdString( name );
#else
    const QString sampleCounterName = QString( name.c_str() );
#endif

    // define metric view column headers
    QStringList metricDesc;
    metricDesc << sampleCounterName << s_functionTitle;

    // for details view emit signal to create just the model
    emit addMetricView( clusteringCriteriaName, metricName, viewName, metricDesc );

    SmartPtr< std::map< TS,
                std::map< Framework::Thread,
                    std::map< Framework::StackTrace, DETAIL_t > > > > raw_items;

    Queries::GetMetricValues( collector, metricName.toStdString(), interval, threadGroup, getThreadSet<TS>( threadGroup ),  // input - metric search criteria
                              raw_items );                                                                                  // output - raw metric values

    for ( typename std::map< TS, std::map< Framework::Thread, std::map< Framework::StackTrace, DETAIL_t > > >::iterator iter = raw_items->begin(); iter != raw_items->end(); iter++ ) {

        const QString locationName = getLocationInfo( iter->first );

        typename std::map< Framework::Thread, std::map< Framework::StackTrace, DETAIL_t > >& thread( iter->second );

        std::map< Framework::Thread, Framework::ExtentGroup > subextents_map;
        Get_Subextents_To_Object_Map( threadGroup, iter->first, subextents_map );

        std::set< Framework::StackTrace, ltST > StackTraces_Processed;

        qulonglong totalSampleCount = 0;

        for ( typename std::map< Framework::Thread, std::map< Framework::StackTrace, DETAIL_t > >::iterator titer = thread.begin(); titer != thread.end(); titer++ ) {
            const typename std::map< Framework::StackTrace, DETAIL_t >& tracemap( titer->second );

            for ( typename std::map< Framework::StackTrace, DETAIL_t >::const_iterator siter = tracemap.begin(); siter != tracemap.end(); siter++ ) {
                const Framework::StackTrace& stacktrace( siter->first );
                const DETAIL_t& details( siter->second );

                std::pair< std::set< Framework::StackTrace >::iterator, bool > ret = StackTraces_Processed.insert( stacktrace );
                if ( ! ret.second )
                    continue;

                totalSampleCount += details.dm_events;
            }
        }

        emit addMetricViewData( clusteringCriteriaName, metricName, viewName, QVariantList() << totalSampleCount << locationName );
    }

    emit requestMetricViewComplete( clusteringCriteriaName, metricName, viewName, lower, upper );
}


} // GUI
} // ArgoNavis

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


QString PerformanceDataManager::s_functionTitle( tr("Function (defining location)") );
QString PerformanceDataManager::s_minimumTitle( tr("Minimum (Thread)") );
QString PerformanceDataManager::s_maximumTitle( tr("Maximum (Thread)") );
QString PerformanceDataManager::s_meanTitle( tr("Average (Thread)") );

const QString CUDA_EVENT_DETAILS_METRIC = QStringLiteral("Details");
const QString ALL_EVENTS_DETAILS_VIEW = QStringLiteral("All Events");
const QString KERNEL_EXECUTION_DETAILS_VIEW = QStringLiteral("Kernel Executions");
const QString DATA_TRANSFER_DETAILS_VIEW = QStringLiteral("Data Transfers");

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
    , m_renderer( Q_NULLPTR )
{
    qRegisterMetaType< Base::Time >("Base::Time");
    qRegisterMetaType< CUDA::DataTransfer >("CUDA::DataTransfer");
    qRegisterMetaType< CUDA::KernelExecution >("CUDA::KernelExecution");
    qRegisterMetaType< QVector< QString > >("QVector< QString >");
    qRegisterMetaType< QVector< bool > >("QVector< bool >");

    m_renderer = new BackgroundGraphRenderer;
#if defined(HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE)
    m_thread.start();
    m_renderer->moveToThread( &m_thread );
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( this, &PerformanceDataManager::loadComplete, m_renderer, &BackgroundGraphRenderer::signalProcessCudaEventView );
    connect( this, &PerformanceDataManager::graphRangeChanged, m_renderer, &BackgroundGraphRenderer::handleGraphRangeChanged );
    connect( this, &PerformanceDataManager::graphRangeChanged, this, &PerformanceDataManager::handleLoadCudaMetricViews );
    connect( m_renderer, &BackgroundGraphRenderer::signalCudaEventSnapshot, this, &PerformanceDataManager::addCudaEventSnapshot );
    connect( &m_userChangeMgr, &UserGraphRangeChangeManager::timeout, this, &PerformanceDataManager::handleLoadCudaMetricViewsTimeout );
#else
    connect( this, SIGNAL(loadComplete()), m_renderer, SIGNAL(signalProcessCudaEventView()) );
    connect( this, SIGNAL(graphRangeChanged(QString,double,double,QSize)), m_renderer, SLOT(handleGraphRangeChanged(QString,double,double,QSize)) );
    connect( this, SIGNAL(graphRangeChanged(QString,double,double,QSize)), this, SLOT(handleLoadCudaMetricViews(QString,double,double)) );
    connect( m_renderer, SIGNAL(signalCudaEventSnapshot(QString,QString,double,double,QImage)), this, SIGNAL(addCudaEventSnapshot(QString,QString,double,double,QImage)) );
    connect( &m_userChangeMgr, SIGNAL(timeout(QString,double,double,QSize)), this, SLOT(handleLoadCudaMetricViewsTimeout(QString,double,double)) );
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
 * @param clusterName - the name of the cluster associated with the metric view
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 *
 * Handler for external request to produce metric view data for specified metric view.
 */
void PerformanceDataManager::handleRequestMetricView(const QString& clusterName, const QString& metricName, const QString& viewName)
{
    if ( ! m_tableViewInfo.contains( clusterName ) || metricName.isEmpty() || viewName.isEmpty() )
        return;

#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleRequestMetricView: clusterName=" << clusterName << "metric=" << metricName << "view=" << viewName;
#endif

    MetricTableViewInfo& info = m_tableViewInfo[ clusterName ];

    const QString metricViewName = metricName + "-" + viewName;

    if ( info.metricViewList.contains( metricViewName ) )
        return;

    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QString("generate-%1").arg(metricViewName) );
    }

    info.metricViewList << metricViewName;

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
    QFutureSynchronizer<void> synchronizer;

    QMap< QString, QFuture<void> > futures;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Experiment experiment( info.experimentFilename.toStdString() );
#else
    Experiment experiment( std::string( info.experimentFilename.toLatin1().data() ) );
#endif

    CollectorGroup collectors = experiment.getCollectors();

    if ( collectors.size() > 0 ) {
        const Collector collector( *collectors.begin() );

        QStringList tableColumnHeaders = info.tableColumnHeaders;
        TimeInterval interval = info.interval;

        if ( ! info.viewList.contains( viewName ) )
            info.viewList << viewName;

        QStringList columnTitles;
        columnTitles << tableColumnHeaders.takeFirst();
        columnTitles << tableColumnHeaders.takeFirst();
        columnTitles << s_functionTitle << s_minimumTitle << s_maximumTitle << s_meanTitle;

        loadCudaMetricViews(
            #if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                    synchronizer, futures,
            #endif
                    QStringList() << metricName,
                    QStringList() << viewName,
                    columnTitles,
                    collector,
                    experiment,
                    interval );

        if ( futures.size() > 0 ) {
            // Determine full time interval extent of this experiment
            Extent extent = experiment.getPerformanceDataExtent();
            Base::TimeInterval experimentInterval = ConvertToArgoNavis( extent.getTimeInterval() );

            Base::TimeInterval graphInterval = ConvertToArgoNavis( interval );

            double lower = ( graphInterval.begin() - experimentInterval.begin() ) / 1000000.0;
            double upper = ( graphInterval.end() - experimentInterval.begin() ) / 1000000.0;

            synchronizer.waitForFinished();

            emit requestMetricViewComplete( clusterName, metricName, viewName, lower, upper );
        }
    }

    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QString("generate-%1").arg(metricViewName) );
    }
}

/**
 * @brief PerformanceDataManager::handleProcessDetailViews
 * @param clusterName - the cluster group name
 *
 * This method handles a request for a new detail view.  After building the ArgoNavis::CUDA::PerformanceData object for threads of interest,
 * it will begin processing of the all CUDA event types by executing the visitor methods in a separate thread (via QtConcurrent::run) over
 * the entire duration of the experiment and waits for the thread to complete.
 */
void PerformanceDataManager::handleProcessDetailViews(const QString &clusterName)
{ 
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleRequestDetailView: clusterName=" << clusterName;
#endif

    if ( ! m_tableViewInfo.contains( clusterName ) )
        return;

    MetricTableViewInfo& info = m_tableViewInfo[ clusterName ];

    const QString metricViewName = CUDA_EVENT_DETAILS_METRIC + "-" + ALL_EVENTS_DETAILS_VIEW;

    // the details view should only be setup once
    if ( info.metricViewList.contains( metricViewName ) )
        return;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    for ( auto metricName : { ALL_EVENTS_DETAILS_VIEW, KERNEL_EXECUTION_DETAILS_VIEW, DATA_TRANSFER_DETAILS_VIEW } ) {
#else
    foreach ( const QString& metricName, QStringList() << ALL_EVENTS_DETAILS_VIEW << KERNEL_EXECUTION_DETAILS_VIEW << DATA_TRANSFER_DETAILS_VIEW ) {
#endif
        info.metricViewList << ( CUDA_EVENT_DETAILS_METRIC + "-" + metricName );
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Experiment experiment( info.experimentFilename.toStdString() );
#else
    Experiment experiment( std::string( info.experimentFilename.toLatin1().data() ) );
#endif

    ThreadGroup all_threads = experiment.getThreads();
    CollectorGroup collectors = experiment.getCollectors();

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
    emit addMetricView( clusterName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, tableColumnList );

    // build the proxy views and tree views for the three details views: "All Events", "Kernel Executions" and "Data Transfers"
    emit addAssociatedMetricView( clusterName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, metricViewName, commonColumnList );
    emit addAssociatedMetricView( clusterName, CUDA_EVENT_DETAILS_METRIC, KERNEL_EXECUTION_DETAILS_VIEW, metricViewName, ArgoNavis::CUDA::getKernelExecutionDetailsHeaderList() );
    emit addAssociatedMetricView( clusterName, CUDA_EVENT_DETAILS_METRIC, DATA_TRANSFER_DETAILS_VIEW, metricViewName, ArgoNavis::CUDA::getDataTransferDetailsHeaderList() );

    QFutureSynchronizer<void> synchronizer;

    Base::TimeInterval interval = data.interval();

    foreach( const ArgoNavis::Base::ThreadName thread, threads.keys() ) {
        synchronizer.addFuture( QtConcurrent::run( &data, &CUDA::PerformanceData::visitDataTransfers, thread, interval,
                                                   boost::bind( &PerformanceDataManager::processDataTransferDetails, this,
                                                                boost::cref(clusterName), boost::cref(data.interval().begin()), _1 ) ) );

        synchronizer.addFuture( QtConcurrent::run( &data, &CUDA::PerformanceData::visitKernelExecutions, thread, interval,
                                                   boost::bind( &PerformanceDataManager::processKernelExecutionDetails, this,
                                                                boost::cref(clusterName), boost::cref(data.interval().begin()), _1 ) ) );
    }

    double durationMs( 0.0 );
    if ( ! interval.empty() ) {
        uint64_t duration = static_cast<uint64_t>(interval.end() - interval.begin());
        durationMs = qCeil( duration / 1000000.0 );
    }

    const double lower = 0.0;
    const double upper = durationMs;

    synchronizer.waitForFinished();

    emit requestMetricViewComplete( clusterName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, lower, upper );
    emit requestMetricViewComplete( clusterName, CUDA_EVENT_DETAILS_METRIC, KERNEL_EXECUTION_DETAILS_VIEW, lower, upper );
    emit requestMetricViewComplete( clusterName, CUDA_EVENT_DETAILS_METRIC, DATA_TRANSFER_DETAILS_VIEW, lower, upper );
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
 * @param clusterName - the cluster group name
 * @param time_origin - the time origin of the experiment
 * @param details - the CUDA data transfer details
 * @return - indicates whether visitation should continue (always true)
 *
 * Visitation method to process each CUDA data transfer event and provide to CUDA event details view.
 */
bool PerformanceDataManager::processDataTransferDetails(const QString &clusterName, const Base::Time &time_origin, const CUDA::DataTransfer &details)
{
    QVariantList detailsData = ArgoNavis::CUDA::getDataTransferDetailsDataList( time_origin, details );

    emit addMetricViewData( clusterName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, detailsData, ArgoNavis::CUDA::getDataTransferDetailsHeaderList() );

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::processKernelExecutionDetails
 * @param clusterName - the cluster group name
 * @param time_origin - the time origin of the experiment
 * @param details - the CUDA kernel execution details
 * @return - indicates whether visitation should continue (always true)
 *
 * Visitation method to process each CUDA kernel executiopn event and provide to CUDA event details view.
 */
bool PerformanceDataManager::processKernelExecutionDetails(const QString &clusterName, const Base::Time &time_origin, const CUDA::KernelExecution &details)
{
    QVariantList detailsData = ArgoNavis::CUDA::getKernelExecutionDetailsDataList( time_origin, details );

    emit addMetricViewData( clusterName, CUDA_EVENT_DETAILS_METRIC, ALL_EVENTS_DETAILS_VIEW, detailsData, ArgoNavis::CUDA::getKernelExecutionDetailsHeaderList() );

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
    Q_UNUSED( details );

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
    Q_UNUSED( details );

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
 * @brief PerformanceDataManager::processMetricView
 * @param collector - a copy of the cuda collector
 * @param threads - all threads known by the cuda collector
 * @param interval - the time interval of interest
 * @param metric - the metric to generate data for
 * @param metricDesc - the metrics to generate data for
 *
 * Build function/statement view output for the specified metrics for all threads over the entire experiment time period.
 * NOTE: must be metrics providing time information.
 */
template <typename TS>
void PerformanceDataManager::processMetricView(const Collector collector, const ThreadGroup& threads, const TimeInterval& interval, const QString &metric, const QStringList &metricDesc)
{
    const QString viewName = getViewName<TS>();

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processMetricView STARTED" << metric;
#endif
#if defined(HAS_CONCURRENT_PROCESSING_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processMetricView: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif

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
                              threads,
                              getThreadSet<TS>( threads ),
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

    // get cluster name
    Thread thread = *(threads.begin());

    const QString clusterName = ArgoNavis::CUDA::getUniqueClusterName( thread );

    emit addMetricView( clusterName, metric, viewName, metricDesc );

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

        emit addMetricViewData( clusterName, metric, viewName, metricData );
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processMetricView FINISHED" << metric;
#endif
}

/**
 * @brief PerformanceDataManager::processCalltreeView
 * @param collector - a copy of the cuda collector
 * @param threads - all threads known by the cuda collector
 * @param interval - the time interval of interest
 *
 * Build calltree view output for the specified group of threads and time interval.
 */
void PerformanceDataManager::processCalltreeView(const Collector collector, const ThreadGroup& threads, const TimeInterval& interval)
{
    const std::set< Function > functions( threads.getFunctions() );

    QStringList metricDesc;
    metricDesc << QStringLiteral("Inclusive Time") << QStringLiteral("Inclusive Counts") << s_functionTitle;

    const std::string collectorId = collector.getMetadata().getUniqueId();

    if ( collectorId == "usertime" ) {
        ShowCalltreeDetail< Framework::UserTimeDetail >( collector, threads, interval, functions, "inclusive_detail", metricDesc );
    }
    else if ( collectorId == "cuda" ) {
        ShowCalltreeDetail< std::vector<Framework::CUDAExecDetail> >( collector, threads, interval, functions, "exec_inclusive_details", metricDesc );
    }
    else if ( collectorId == "mpi" ) {
        ShowCalltreeDetail< std::vector<Framework::MPIDetail> >( collector, threads, interval, functions, "inclusive_details", metricDesc );
    }
    else if ( collectorId == "pthreads" ) {
        ShowCalltreeDetail< std::vector<Framework::PthreadsDetail> >( collector, threads, interval, functions, "inclusive_details", metricDesc );
    }
    else if ( collectorId == "omptp" ) {
        ShowCalltreeDetail< Framework::OmptPDetail >( collector, threads, interval, functions, "inclusive_detail", metricDesc );
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Experiment experiment( filePath.toStdString() );
#else
    Experiment experiment( std::string( filePath.toLatin1().data() ) );
#endif

    // Determine full time interval extent of this experiment
    Extent extent = experiment.getPerformanceDataExtent();
    TimeInterval interval = extent.getTimeInterval();

    const QString timeMetric( "time" );
    QStringList metricList;
    QStringList metricDescList;

    CollectorGroup collectors = experiment.getCollectors();
    boost::optional<Collector> collector;
    bool foundOne( false );

    for ( CollectorGroup::const_iterator i = collectors.begin(); i != collectors.end() && ! foundOne; ++i ) {
        std::set<Metadata> metrics = i->getMetrics();
        std::set<Metadata>::iterator iter( metrics.begin() );
        while ( iter != metrics.end() ) {
            Metadata metadata( *iter );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            QString metricName = QString::fromStdString( metadata.getUniqueId() );
            QString metricDesc = QString::fromStdString( metadata.getShortName() );
#else
            QString metricName( metadata.getUniqueId().c_str() );
            QString metricDesc( metadata.getShortName().c_str() );
#endif
            if ( metricName.contains( timeMetric ) && metadata.isType( typeid(double) ) ) {
                metricList << metricName;
                metricDescList <<  tr("% of Time");
                metricDescList <<  tr("Time (msec)");
                foundOne = true;
            }
            iter++;
        }
        collector = *i;
    }

    if ( collector ) {
        bool hasCudaCollector( "cuda" == collector.get().getMetadata().getUniqueId() );

        QFutureSynchronizer<void> synchronizer;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString experimentFilename = QString::fromStdString( experiment.getName() );
#else
        QString experimentFilename( QString( experiment.getName().c_str() ) );
#endif

        QFileInfo fileInfo( experimentFilename );
        QString experimentName( fileInfo.fileName() );
        experimentName.replace( QString(".openss"), QString("") );

        Thread thread = *(experiment.getThreads().begin());

        const QString clusterName = ArgoNavis::CUDA::getUniqueClusterName( thread );

        MetricTableViewInfo info;
        info.metricList = metricList;
        info.tableColumnHeaders = metricDescList;
        info.experimentFilename = experimentFilename;
        info.interval = interval;

        m_tableViewInfo[ clusterName ] = info;
        
        QFuture<void> future1 = QtConcurrent::run( this, &PerformanceDataManager::loadCudaView, experimentName, collector.get(), experiment.getThreads() );
        synchronizer.addFuture( future1 );

        if ( hasCudaCollector ) {
            QFuture<void> future2 = QtConcurrent::run( this, &PerformanceDataManager::handleProcessDetailViews, clusterName );
            synchronizer.addFuture( future2 );
        }

        const QString functionView = QStringLiteral("Functions");

        foreach ( const QString& metricName, metricList ) {
            handleRequestMetricView( clusterName, metricName, functionView );
        }

        synchronizer.waitForFinished();
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaViews: ENDED";
#endif

    emit loadComplete();
}

/**
 * @brief PerformanceDataManager::handleLoadCudaMetricViews
 * @param clusterName - the cluster name
 * @param lower - the lower value of the interval to process
 * @param upper - the upper value of the interval to process
 *
 * Handles graph range change - initiates a timed monitor to actually process the graph range change if user hasn't
 * continued to update the graph range within a threshold period.
 */
void PerformanceDataManager::handleLoadCudaMetricViews(const QString& clusterName, double lower, double upper)
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleLoadCudaMetricViews: clusterName=" << clusterName << "lower=" << lower << "upper=" << upper;
#endif

    if ( ! m_tableViewInfo.contains( clusterName ) || lower >= upper )
        return;

    // cancel any active processing for this cluster
    m_userChangeMgr.cancel( clusterName );

    // initiate a new timed monitor for this cluster
    m_userChangeMgr.create( clusterName, lower, upper );
}

/**
 * @brief PerformanceDataManager::handleLoadCudaMetricViewsTimeout
 * @param clusterName - the cluster name
 * @param lower - the lower value of the interval to process
 * @param upper - the upper value of the interval to process
 *
 * This handler in invoked when the waiting period has benn reached and actual processing of the CUDA metric view can proceed.
 */
void PerformanceDataManager::handleLoadCudaMetricViewsTimeout(const QString& clusterName, double lower, double upper)
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleLoadCudaMetricViewsTimeout: clusterName=" << clusterName << "lower=" << lower << "upper=" << upper;
#endif

    if ( ! m_tableViewInfo.contains( clusterName ) )
        return;

    MetricTableViewInfo& info = m_tableViewInfo[ clusterName ];

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QStringLiteral("metric-view" ) );
    }

    QFutureSynchronizer<void> synchronizer;

    QMap< QString, QFuture<void> > futures;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Experiment experiment( info.experimentFilename.toStdString() );
#else
    Experiment experiment( std::string( info.experimentFilename.toLatin1().data() ) );
#endif

    CollectorGroup collectors = experiment.getCollectors();

    if ( collectors.size() > 0 ) {
        const Collector collector( *collectors.begin() );

        // Determine time origin from extent of this experiment
        Extent extent = experiment.getPerformanceDataExtent();
        Time timeOrigin = extent.getTimeInterval().getBegin();

        // Calculate new interval from currently selected graph range
        Time lowerTime = timeOrigin + lower * 1000000;
        Time upperTime = timeOrigin + upper * 1000000;

        TimeInterval interval( lowerTime, upperTime );

        info.interval = interval;

        // Update metric view scorresponding to timeline in graph view
        loadCudaMetricViews(
            #if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                    synchronizer, futures,
            #endif
                    info.metricList, info.viewList, info.tableColumnHeaders, collector, experiment, interval );

        // Emit signal to update detail views corresponding to timeline in graph view
        foreach ( const QString& metricViewName, info.metricViewList ) {
            QStringList tokens = metricViewName.split('-');
            if ( 2 == tokens.size() ) {
                if ( QStringLiteral("Details") == tokens[0] ) {
                    emit metricViewRangeChanged( clusterName, tokens[0], tokens[1], lower, upper );
                }
                else {
                    emit requestMetricViewComplete( clusterName, tokens[0], tokens[1], lower, upper );
                }
            }
        }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
        synchronizer.waitForFinished();
#endif
    }

    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QStringLiteral("metric-view" ) );
    }
}

/**
 * @brief PerformanceDataManager::loadCudaMetricViews
 * @param synchronizer - the QFutureSynchronizer to add futures
 * @param futures - the future map used when invoking QtConcurrent::run
 * @param metricList - the list of metrics to process and add to metric view
 * @param viewList - the list of views to process and add to the metric view
 * @param metricDescList - the column headers for the metric view
 * @param collector - the collector object
 * @param experiment - the experiment object
 * @param interval - the interval to process for the metric view
 *
 * Process the specified metric views.
 */
void PerformanceDataManager::loadCudaMetricViews(
        #if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
        QFutureSynchronizer<void>& synchronizer,
        QMap< QString, QFuture<void> >& futures,
        #endif
        const QStringList& metricList,
        const QStringList& viewList,
        QStringList metricDescList,
        const Collector& collector,
        const Experiment& experiment,
        const TimeInterval& interval)
{
    const ThreadGroup threadGroup( experiment.getThreads() );

    foreach ( const QString& metricName, metricList ) {
        QStringList metricDesc;

        metricDesc << metricDescList.takeFirst() << metricDescList.takeFirst() << s_functionTitle << s_minimumTitle << s_maximumTitle << s_meanTitle;

        foreach ( const QString& viewName, viewList ) {
            const QString futuresKey = metricName + QStringLiteral("-") + viewName;

            if ( viewName == QStringLiteral("Functions") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                futures[ futuresKey ] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView<Function>, collector, threadGroup, interval, metricName, metricDesc );
                synchronizer.addFuture( futures[ futuresKey ] );
#else
                processMetricView<Function>( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
            }

            else if ( viewName == QStringLiteral("Statements") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                futures[ futuresKey ] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView<Statement>, collector, threadGroup, interval, metricName, metricDesc );
                synchronizer.addFuture( futures[ futuresKey ] );
#else
                processMetricView<Statement>( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
            }

            else if ( viewName == QStringLiteral("LinkedObjects") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                futures[ futuresKey ] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView<LinkedObject>, collector, threadGroup, interval, metricName, metricDesc );
                synchronizer.addFuture( futures[ futuresKey ] );
#else
                processMetricView<LinkedObject>( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
            }

            else if ( viewName == QStringLiteral("Loops") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                futures[ futuresKey ] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView<Loop>, collector, threadGroup, interval, metricName, metricDesc );
                synchronizer.addFuture( futures[ futuresKey ] );
#else
                processMetricView<Loop>( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
            }

            else if ( viewName == QStringLiteral("CallTree") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                futures[ futuresKey ] = QtConcurrent::run( this, &PerformanceDataManager::processCalltreeView, collector, threadGroup, interval );
                synchronizer.addFuture( futures[ futuresKey ] );
#else
                processCalltreeView( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
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

    foreach( const QString& clusterName, clusterNames ) {
        m_tableViewInfo.remove( clusterName );
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
 * @brief PerformanceDataManager::loadCudaView
 * @param experimentName - experiment database filename
 * @param collector - a reference to the cuda collector
 * @param all_threads - all threads known by the cuda collector
 *
 * Parse the requested CUDA performance data to metric model data.
 */
void PerformanceDataManager::loadCudaView(const QString& experimentName, const Collector& collector, const ThreadGroup& all_threads)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaView STARTED!!";
#endif

    QMap< Base::ThreadName, bool > flags;

    // initialize all thread flags to true in order to add all threads to PerformanceData object instance
    for (ThreadGroup::const_iterator i = all_threads.begin(); i != all_threads.end(); ++i) {
        flags.insert( ConvertToArgoNavis(*i), true );
    }

    CUDA::PerformanceData data;
    QMap< Base::ThreadName, Thread> threads;

    bool hasCudaCollector = getPerformanceData( collector, all_threads, flags, threads, data );

    // set default metric view
    emit signalSetDefaultMetricView( hasCudaCollector ? CUDA_VIEW : CALLTREE_VIEW );

    // reset all thread flags to false
    QMutableMapIterator< Base::ThreadName, bool > mapiter( flags );
    while( mapiter.hasNext() ) {
        mapiter.next();
        mapiter.value() = false;
    }

    double durationMs( 0.0 );
    if ( ! data.interval().empty() ) {
        uint64_t duration = static_cast<uint64_t>(data.interval().end() - data.interval().begin());
        durationMs = qCeil( duration / 1000000.0 );
    }

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

    QString clusteringCriteriaName;
    if ( hasCudaCollector )
        clusteringCriteriaName = QStringLiteral( "GPU Compute / Data Transfer Ratio" );
    else
        clusteringCriteriaName = QStringLiteral( "Thread Groups" );

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
            emit addCluster( clusteringCriteriaName, clusterName );
        }

        data.visitThreads( boost::bind(
                               &PerformanceDataManager::processPerformanceData, instance(),
                               boost::cref(data), _1, boost::cref(gpuCounterIndexes), boost::cref(clusteringCriteriaName) ) );

        foreach( const QString& clusterName, clusterNames ) {
            bool hasGpuPercentageCounter( isGpuSampleCounterPercentage.contains(clusterName) && isGpuSampleCounterPercentage[clusterName] );
            emit setMetricDuration( clusteringCriteriaName, clusterName, durationMs, hasGpuPercentageCounter);
            qDebug() << "CLUSTER NAME: " << clusterName << " PERCENTAGE SAMPLE COUNTER? " << hasGpuPercentageCounter;
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
 * @param functionName - name of called function
 * @param linkedObjectName - name of linked object where called function resides
 * @param callingFunctionName - name of calling function
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
 * @brief PerformanceDataManager::ShowCalltreeDetail
 * @param collector - the experiment collector used for the calltree view
 * @param threadGroup - the set of threads applicable to the calltree view
 * @param interval - the time interval for the calltree view
 * @param functions - the set of functions involved in the calltree view
 * @param metric - the metric computed in the calltree view
 * @param reduced_details - the data computed for the calltree view
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
        const QStringList metricDesc)
{
    // bet view name
    const QString viewName = getViewName<DETAIL_t>();

    // get cluster name
    Thread thread = *(threadGroup.begin());

    const QString clusterName = ArgoNavis::CUDA::getUniqueClusterName( thread );

    emit addMetricView( clusterName, viewName, viewName, metricDesc );

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
        const Framework::Function function( iter->first );
        std::map< Framework::StackTrace, DETAIL_t > tracemap( iter->second );

        std::map< Framework::Thread, Framework::ExtentGroup > subextents_map;
        Get_Subextents_To_Object_Map( threadGroup, function, subextents_map );

        std::set< Framework::StackTrace, ltST > StackTraces_Processed;

        for ( typename std::map< Framework::StackTrace, DETAIL_t >::iterator siter = tracemap.begin(); siter != tracemap.end(); siter++ ) {
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

            int64_t num_calls = ( subExtents.begin() == subExtents.end() ) ? 1 : stack_contains_N_calls( stacktrace, subExtents );

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

    for ( TDETAILS::reverse_iterator i = reduced_details.rbegin(); i != reduced_details.rend(); ++i ) {
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
        emit addMetricViewData( clusterName, viewName, viewName, metricData );
    }

}

} // GUI
} // ArgoNavis

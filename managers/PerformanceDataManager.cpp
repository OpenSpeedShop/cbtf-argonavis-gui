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
#include "managers/CalltreeGraphManager.h"
#include "CBTF-ArgoNavis-Ext/DataTransferDetails.h"
#include "CBTF-ArgoNavis-Ext/KernelExecutionDetails.h"
#include "CBTF-ArgoNavis-Ext/ClusterNameBuilder.h"

#include <algorithm>
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

#include "collectors/usertime/UserTimeDetail.hxx"

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

    m_renderer = new BackgroundGraphRenderer;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)) && defined(HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE)
    m_thread.start();
    m_renderer->moveToThread( &m_thread );
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::PerformanceDataManager: &m_thread=" << QString::number((long long)&m_thread, 16);
#endif
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( this, &PerformanceDataManager::replotCompleted, m_renderer, &BackgroundGraphRenderer::signalProcessCudaEventView );
    connect( this, &PerformanceDataManager::graphRangeChanged, m_renderer, &BackgroundGraphRenderer::handleGraphRangeChanged );
    connect( this, &PerformanceDataManager::graphRangeChanged, this, &PerformanceDataManager::handleLoadCudaMetricViews );
    connect( m_renderer, &BackgroundGraphRenderer::signalCudaEventSnapshot, this, &PerformanceDataManager::addCudaEventSnapshot );
    connect( &m_userChangeMgr, &UserGraphRangeChangeManager::timeout, this, &PerformanceDataManager::handleLoadCudaMetricViewsTimeout );
#else
    connect( this, SIGNAL(replotCompleted()), m_renderer, SIGNAL(signalProcessCudaEventView()) );
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)) && defined(HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE)
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

    info.metricViewList << metricViewName;

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
    QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

    QFutureSynchronizer<void> synchronizer;

    QMap< QString, QFuture<void> > futures;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Experiment experiment( info.experimentFilename.toStdString() );
#else
    Experiment experiment( std::string( info.experimentFilename.toLatin1().data() ) );
#endif

    CollectorGroup collectors = experiment.getCollectors();
    boost::optional<Collector> collector;

    for ( CollectorGroup::const_iterator i = collectors.begin(); i != collectors.end(); ++i ) {
        collector = *i;
        break;
    }

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

    QApplication::restoreOverrideCursor();
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
            if ( gpuCounterIndexes.contains( i ) ) {
                // add periodic sample bin to corresponding GPU cluster
                emit addPeriodicSample( clusteringCriteriaName, clusterName+" (GPU)", lastTimeStamp, timeStamp, value );
            }
            else {
                // add periodic sample bin to corresponding CPU cluster
                emit addPeriodicSample( clusteringCriteriaName, clusterName, lastTimeStamp, timeStamp, value );
            }
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

    data.visitPeriodicSamples(
                thread, data.interval(),
                boost::bind( &PerformanceDataManager::processPeriodicSample, instance(),
                             boost::cref(data.interval().begin()), _1, _2,
                             boost::cref(gpuCounterIndexes),
                             boost::cref(clusterName), boost::cref(clusteringCriteriaName) )
                );

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
 * @brief PerformanceDataManager::hasCudaEvents
 * @param data - the performance data structure to be parsed
 * @param thread - the thread of interest
 * @param flag - a mapping of thread names to boolean to indicate whether threads have any CUDA events
 * @return - continue (=true) or not continue (=false) the visitation
 */
bool PerformanceDataManager::hasCudaEvents(const CUDA::PerformanceData &data,
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

        double value( i->first * 1000.0 );
        double min( dataMin->at(i->second) * 1000.0 );
        double max( dataMax->at(i->second) * 1000.0 );
        double mean( dataMean->at(i->second) * 1000.0 );
        double percentage( i->first / total * 100.0 );
        QString valueStr( QString::number( value, 'f', 6 ) );
        QString minStr( QString::number( min, 'f', 6 ) );
        QString maxStr( QString::number( max, 'f', 6 ) );
        QString meanStr( QString::number( mean, 'f', 6 ) );
        QString percentageStr( QString::number( percentage, 'f', 6 ) );

        metricData << QVariant::fromValue< double >( valueStr.toDouble() );
        metricData << QVariant::fromValue< double >( percentageStr.toDouble() );
        metricData << getLocationInfo<TS>( i->second );
        metricData << QVariant::fromValue< double >( minStr.toDouble() );
        metricData << QVariant::fromValue< double >( maxStr.toDouble() );
        metricData << QVariant::fromValue< double >( meanStr.toDouble() );

        emit addMetricViewData( clusterName, metric, viewName, metricData );
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processMetricView FINISHED" << metric;
#endif
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

    Experiment experiment( filePath.toStdString() );
#if 0
    ShowUserTimeDetailOrig( experiment, "exclusive_detail" );
#else
    const Collector collector = *experiment.getCollectors().begin();
    const TimeInterval interval( Time::TheBeginning(), Time::TheEnd() );
    const ThreadGroup threadGroup( experiment.getThreads() );
    const std::set< Function > functions( threadGroup.getFunctions() );

    TDETAILS exclusive_details;

    ShowDetail< Framework::UserTimeDetail >( collector, threadGroup, interval, functions, "exclusive_detail", exclusive_details );

#if 1
    std::cout << "Reduced exclusive details (by function name):" << std::endl;
    for (const details_data_t& d : exclusive_details) {
        std::cout << "\t" << std::left << std::setw(20) << std::fixed << std::setprecision(6) << std::get<1>(d) << std::setw(20) << std::get<0>(d) << std::setw(20) << std::get<2>(d).getName() << std::endl;
    }
#endif

    TDETAILS inclusive_details;

    ShowDetail< Framework::UserTimeDetail >( collector, threadGroup, interval, functions, "inclusive_detail", inclusive_details );

#if 1
    std::cout << "Reduced inclusive details (by function name):" << std::endl;
    for (const details_data_t& d : inclusive_details) {
        std::cout << "\t" << std::left << std::setw(20) << std::fixed << std::setprecision(6) << std::get<1>(d) << std::setw(20) << std::get<0>(d) << std::setw(20) << std::get<2>(d).getName() << std::endl;
    }
#endif
#endif
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
                metricDescList <<  "% of " + metricDesc;
                metricDescList <<  metricDesc + " (msec)";
                foundOne = true;
            }
            iter++;
        }
        collector = *i;
    }

    if ( collector ) {
        QFutureSynchronizer<void> synchronizer;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString experimentFilename = QString::fromStdString( experiment.getName() );
#else
        QString experimentFilename( QString( experiment.getName().c_str() ) );
#endif

        QFileInfo fileInfo( experimentFilename );
        QString experimentName( fileInfo.fileName() );
        experimentName.replace( QString(".openss"), QString("") );

        QFuture<void> future = QtConcurrent::run( this, &PerformanceDataManager::loadCudaView, experimentName, collector.get(), experiment.getThreads() );
        synchronizer.addFuture( future );

        Thread thread = *(experiment.getThreads().begin());

        const QString clusterName = ArgoNavis::CUDA::getUniqueClusterName( thread );

        MetricTableViewInfo info;
        info.metricList = metricList;
        info.tableColumnHeaders = metricDescList;
        info.experimentFilename = experimentFilename;
        info.interval = interval;

        m_tableViewInfo[ clusterName ] = info;

        handleProcessDetailViews( clusterName );

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
    if ( ! m_tableViewInfo.contains( clusterName ) || lower >= upper )
        return;

#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "PerformanceDataManager::handleLoadCudaMetricViews: clusterName=" << clusterName << "lower=" << lower << "upper=" << upper;
#endif

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
    QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

    QFutureSynchronizer<void> synchronizer;

    QMap< QString, QFuture<void> > futures;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Experiment experiment( info.experimentFilename.toStdString() );
#else
    Experiment experiment( std::string( info.experimentFilename.toLatin1().data() ) );
#endif

    CollectorGroup collectors = experiment.getCollectors();
    boost::optional<Collector> collector;

    for ( CollectorGroup::const_iterator i = collectors.begin(); i != collectors.end(); ++i ) {
        collector = *i;
        break;
    }

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

    QApplication::restoreOverrideCursor();
#endif
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
                                                 boost::optional<Collector>& collector,
                                                 const Experiment& experiment,
                                                 const TimeInterval& interval)
{
    foreach ( const QString& metricName, metricList ) {
        QStringList metricDesc;

        metricDesc << metricDescList.takeFirst() << metricDescList.takeFirst() << s_functionTitle << s_minimumTitle << s_maximumTitle << s_meanTitle;

        foreach ( const QString& viewName, viewList ) {
            if ( viewName == QStringLiteral("Functions") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                QString functionView = metricName + QStringLiteral("-Functions");
                futures[ functionView ] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView<Function>, collector.get(), experiment.getThreads(), interval, metricName, metricDesc );
                synchronizer.addFuture( futures[ functionView ] );
#else
                processMetricView<Function>( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
            }

            else if ( viewName == QStringLiteral("Statements") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                QString statementView = metricName + QStringLiteral("-Statements");
                futures[ statementView ] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView<Statement>, collector.get(), experiment.getThreads(), interval, metricName, metricDesc );
                synchronizer.addFuture( futures[ statementView ] );
#else
                processMetricView<Statement>( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
            }

            else if ( viewName == QStringLiteral("LinkedObjects") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                QString linkedObjectView = metricName + QStringLiteral("-LinkedObjects");
                futures[ linkedObjectView ] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView<LinkedObject>, collector.get(), experiment.getThreads(), interval, metricName, metricDesc );
                synchronizer.addFuture( futures[ linkedObjectView ] );
#else
                processMetricView<LinkedObject>( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
            }

            else if ( viewName == QStringLiteral("Loops") ) {
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                QString loopViewView = metricName + QStringLiteral("-Loops");
                futures[ loopViewView ] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView<Loop>, collector.get(), experiment.getThreads(), interval, metricName, metricDesc );
                synchronizer.addFuture( futures[ loopViewView ] );
#else
                processMetricView<Loop>( collector.get(), experiment.getThreads(), metric, metricDesc );
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
    bool hasCudaCollector( false );

    hasCudaCollector = getPerformanceData( collector, all_threads, flags, threads, data );

    // reset all thread flags to false
    QMutableMapIterator< Base::ThreadName, bool > mapiter( flags );
    while( mapiter.hasNext() ) {
        mapiter.next();
        mapiter.value() = false;
    }

    data.visitThreads( boost::bind( &PerformanceDataManager::hasCudaEvents, instance(), boost::cref(data), _1, boost::ref(flags) ) );

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
        std::string displayNameStr = ArgoNavis::CUDA::stringify<>( ArgoNavis::CUDA::CounterName( data.counters()[i] ) );
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
    while ( iter != threads.end() ) {
        Thread thread( *iter );
        QString hostName = ArgoNavis::CUDA::getUniqueClusterName( thread );
        // Due to 4 Oct 2016 change by Bill to move CUPTI metrics and events collection to a separate thread, there are two threads having the same hostname.
        // Determine the GPU thread from the thread set generated earlier
        if ( flags.value( iter.key(), false ) )
            clusterNames << ( hostName + " (GPU)" );
        else
            clusterNames << hostName;
        ++iter;
    }
#endif

    emit addExperiment( experimentName, clusteringCriteriaName, clusterNames, sampleCounterNames );

    if ( hasCudaCollector ) {
        m_renderer->setPerformanceData( clusteringCriteriaName, clusterNames, data );

        foreach( const QString& clusterName, clusterNames ) {
            emit addCluster( clusteringCriteriaName, clusterName );
        }

        data.visitThreads( boost::bind(
                               &PerformanceDataManager::processPerformanceData, instance(),
                               boost::cref(data), _1, boost::cref(gpuCounterIndexes), boost::cref(clusteringCriteriaName) ) );

        foreach( const QString& clusterName, clusterNames ) {
            emit setMetricDuration( clusteringCriteriaName, clusterName, durationMs );
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

//SmartPtr<std::map<Function,
//                  std::map<Framework::StackTrace,
//                           TDETAIL > > > raw_items;

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

#if 1
typedef std::map< Function, double > detail_time_t;
typedef std::map< Function, uint64_t > detail_count_t;

void sum_detail_values(const UserTimeDetail& primary, int64_t num_calls, uint64_t& detail_count, double& detail_time)
{
    detail_time += ( primary.dm_time / num_calls );
    detail_count += primary.dm_count;
}

void PerformanceDataManager::ShowUserTimeDetailOrig(const Experiment& experiment, const QString& metric)
{
    Collector collector = *experiment.getCollectors().begin();
    std::cout << "metric = " << metric.toStdString() << " Collector unique id =" << collector.getMetadata().getUniqueId() << std::endl;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    using UserTimeDetail = OpenSpeedShop::Framework::UserTimeDetail;
#else
    typedef OpenSpeedShop::Framework::UserTimeDetail UserTimeDetail;
#endif

    SmartPtr< std::map< Function,
                std::map< Framework::Thread,
                    std::map< Framework::StackTrace, UserTimeDetail > > > > raw_items;

    Framework::ThreadGroup threadGroup = experiment.getThreads();

    Queries::GetMetricValues( collector, metric.toStdString(),
                              TimeInterval( Time::TheBeginning(), Time::TheEnd() ),
                              threadGroup,
                              threadGroup.getFunctions(),
                              raw_items );

    SmartPtr< std::map<Function, std::map<Framework::StackTrace, UserTimeDetail > > > data =
            Queries::Reduction::Apply( raw_items, Queries::Reduction::Summation );

    detail_count_t inclusive_count_detail;
    detail_time_t  inclusive_time_detail;
    detail_count_t exclusive_count_detail;
    detail_time_t  exclusive_time_detail;

    std::map< Function, std::map< Framework::StackTrace, UserTimeDetail> >::iterator iter( data->begin() );

    while ( iter != data->end() ) {
        const Framework::Function& function( iter->first );
        const std::string functionName = function.getMangledName();
        std::cout << "Function: " << functionName << std::endl;
        Framework::ExtentGroup subextents;
        Get_Subextents_To_Object( threadGroup, function, subextents );
        std::map< Framework::StackTrace, UserTimeDetail >& tracemap( iter->second );
        std::map< Framework::StackTrace, UserTimeDetail >::iterator siter( tracemap.begin() );
        std::set< Framework::StackTrace > StackTraces_Processed;
        while ( siter != tracemap.end() ) {
            const Framework::StackTrace& stacktrace( siter->first );
            std::pair< std::set< Framework::StackTrace >::iterator, bool > ret = StackTraces_Processed.insert( stacktrace );
            if ( ! ret.second ) {
                std::cout << "STACK TRACE ALREADY PROCESSED - SKIP THIS ONE!!" << std::endl;
                continue;
            }
            for ( std::size_t idx=0; idx<stacktrace.size(); idx++ ) {
                Framework::Address address( stacktrace[idx] );
                std::pair< bool, Framework::Function > this_function = stacktrace.getFunctionAt(idx);
                if ( this_function.first ) {
                    std::cout << '\t' << idx << ": function: " << this_function.second.getDemangledName() << " address: " << address;
                    std::set< Framework::Statement > this_statement = stacktrace.getStatementsAt(idx);
                    Q_ASSERT( this_statement.size() <= 1 );
                    std::set< Framework::Statement >::iterator sit( this_statement.begin() );
                    if ( sit != this_statement.end() ) {
                        std::cout << " statement: " << (*sit).getPath() << "," << (*sit).getLine();
                    }
                    std::cout << std::endl;
                }
            }
            const UserTimeDetail& detail( siter->second );
            std::cout << "\tcount: " << detail.dm_count << " time: " << detail.dm_time << std::endl;
            int64_t num_calls = stack_contains_N_calls( stacktrace, subextents );
            sum_detail_values( detail, num_calls, inclusive_count_detail[function], inclusive_time_detail[function] );
            sum_detail_values( detail, num_calls, exclusive_count_detail[function], exclusive_time_detail[function] );
            siter++;
        }
        iter++;
    }
}
#endif

template <int N, typename BinaryPredicate, typename ForwardIterator>
void sortByFixedComponent (ForwardIterator first, ForwardIterator last) {
    std::sort (first, last, [](const typename ForwardIterator::value_type& x, const typename ForwardIterator::value_type& y)->bool {
        return BinaryPredicate()(std::get<N>(x), std::get<N>(y));
    });
}

struct ltST {
    bool operator() (Framework::StackTrace stl, Framework::StackTrace str) {
        if (stl.getTime() < str.getTime()) { return true; }
        if (stl.getTime() > str.getTime()) { return false; }

        if (stl.getThread() < str.getThread()) { return true; }
        if (stl.getThread() > str.getThread()) { return false; }

        return stl < str;
    }
};

#if 0
template <typename TDETAIL>
void PerformanceDataManager::ShowDetail(
        const Framework::Collector& collector,
        const Framework::ThreadGroup& threadGroup,
        const Framework::TimeInterval& interval,
        const std::set< Framework::Function >& functions,
        const QString& metric,
        TDETAILS& reduced_details)
{
    SmartPtr< std::map< Function,
                std::map< Framework::Thread,
                    std::map< Framework::StackTrace, TDETAIL > > > > raw_items;

    Queries::GetMetricValues( collector, metric.toStdString(), interval, threadGroup, functions,  // input - metric search criteria
                              raw_items );                                                        // output - raw metric values

    SmartPtr< std::map<Function, std::map<Framework::StackTrace, TDETAIL > > > data =
            Queries::Reduction::Apply( raw_items, Queries::Reduction::Summation );

    for ( typename std::map< Function, std::map< Framework::StackTrace, TDETAIL > >::iterator iter = data->begin(); iter != data->end(); iter++ ) {
        const Framework::Function& function( iter->first );
        std::map< Framework::StackTrace, TDETAIL >& tracemap( iter->second );

        std::map< Framework::Thread, Framework::ExtentGroup > subextents_map;
        Get_Subextents_To_Object_Map( threadGroup, function, subextents_map );

        std::set< Framework::StackTrace, ltST > StackTraces_Processed;

        int sum_count(0);
        double sum_time(0.0);

        for ( typename std::map< Framework::StackTrace, TDETAIL >::iterator siter = tracemap.begin(); siter != tracemap.end(); siter++ ) {
            const Framework::StackTrace& stacktrace( siter->first );

            std::pair< std::set< Framework::StackTrace >::iterator, bool > ret = StackTraces_Processed.insert( stacktrace );
            if ( ! ret.second )
                continue;

            const TDETAIL& detail( siter->second );

            /* Find the extents associated with the stack trace's thread. */
            std::map< Framework::Thread, Framework::ExtentGroup >::iterator tei = subextents_map.find( stacktrace.getThread() );
            Framework::ExtentGroup subExtents;
            if (tei != subextents_map.end()) {
                subExtents = (*tei).second;
            }

            int64_t num_calls = ( subExtents.begin() == subExtents.end() ) ? 1 : stack_contains_N_calls( stacktrace, subExtents );

            sum_count += detail.dm_count;
            sum_time += detail.dm_time / num_calls;
        }

        reduced_details.push_back( std::make_tuple( sum_count, sum_time, function ) );
    }

    sortByFixedComponent<1, std::greater<std::tuple_element<1,details_data_t>::type>> (reduced_details.begin(), reduced_details.end());
}
#else
void PerformanceDataManager::detail_reduction(
        const std::set< std::tuple< std::set< Function >, Function > >& caller_function_list,
        TALLDETAILS& all_details,
        TDETAILS& reduced_details)
{
    TALLDETAILS::iterator siter = all_details.begin();

    for ( std::set< std::tuple< std::set< Function >, Function > >::iterator fit = caller_function_list.begin(); fit != caller_function_list.end(); fit++) {
        const std::tuple< std::set< Function >, Function > elem( *fit );
        Function function = std::get<1>(elem);
        std::string functionName( function.getName() );
        std::string linkedObjectName( function.getLinkedObject().getPath() );
        std::string callingFunctionName;
        std::string callingLinkedObjectName;
        std::set< Function > caller = std::get<0>(elem);
        if ( ! caller.empty() ) {
            const Function callingFunction( *(caller.cbegin()) );
            callingFunctionName = callingFunction.getName();
            callingLinkedObjectName = callingFunction.getLinkedObject().getPath();
        }
        std::cout << callingFunctionName << " --> " << functionName << std::endl;
        TALLDETAILS::iterator eiter = std::partition( siter, all_details.end(),
                                                      [&](const all_details_data_t& d) {
                                                          const Function& func( std::get<2>(d) );
                                                          const std::set< Function >& callingFunction( std::get<3>(d) );
                                                          std::string cfuncName;
                                                          std::string cfuncLinkedObjectName;
                                                          if ( ! callingFunction.empty() ) {
                                                              const Function cfunc( *(callingFunction.cbegin()) );
                                                              cfuncName = cfunc.getName();
                                                              cfuncLinkedObjectName = cfunc.getLinkedObject().getPath();
                                                          }
                                                          return func.getName() == functionName && func.getLinkedObject().getPath() == linkedObjectName &&
                                                                 callingFunctionName == cfuncName && callingLinkedObjectName == cfuncLinkedObjectName;
                                                      } );
        if ( siter == all_details.end() )
            break;
        int sum_count(0);
        double sum_time(0.0);
        for ( TALLDETAILS::iterator it = siter; it != eiter; it++ ) {
            all_details_data_t d( *it );
            sum_count += std::get<0>(d);
            sum_time += std::get<1>(d);
        }
        reduced_details.push_back( std::make_tuple( sum_count, sum_time, function ) );
        siter = eiter;
    }

    sortByFixedComponent<1, std::greater<std::tuple_element<1,details_data_t>::type>> (reduced_details.begin(), reduced_details.end());
}

void PerformanceDataManager::generate_calltree_graph(
        const std::set< Framework::Function >& functions,
        const std::set< std::tuple< std::set< Function >, Function > >& caller_function_list,
        std::vector<double>& call_depths,
        std::ostream& os)
{
    CalltreeGraphManager graphManager;

    std::map< Function, CalltreeGraphManager::handle_t > handleMap;

    foreach ( const Framework::Function& function, functions ) {
        std::string functionName( function.getName() );
        std::string sourceFilename;
        uint32_t lineNumber(0);
        std::string linkedObjectName( function.getLinkedObject().getPath() );
        CalltreeGraphManager::handle_t handle = graphManager.addFunctionNode( functionName, sourceFilename, lineNumber, linkedObjectName );

        handleMap[ function ] = handle;
    }

    for ( std::set< std::tuple< std::set< Function >, Function > >::iterator fit = caller_function_list.begin(); fit != caller_function_list.end(); fit++) {
        const std::tuple< std::set< Function >, Function > elem( *fit );
        std::set< Function > caller = std::get<0>(elem);
        if ( caller.empty() ) continue;
        const Function callingFunction( *(caller.cbegin()) );
        Function function = std::get<1>(elem);
        if ( handleMap.find( callingFunction ) != handleMap.end() && handleMap.find( function ) != handleMap.end() ) {
            const CalltreeGraphManager::handle_t& caller_handle = handleMap.at( callingFunction );
            const CalltreeGraphManager::handle_t& handle = handleMap.at( function );
            graphManager.addCallEdge( caller_handle, handle );
        }
    }

    graphManager.write_graphviz( os );

    graphManager.iterate_over_edges();

    graphManager.determine_call_depths( call_depths );
}

template <typename TDETAIL>
void PerformanceDataManager::ShowDetail(
        const Framework::Collector& collector,
        const Framework::ThreadGroup& threadGroup,
        const Framework::TimeInterval& interval,
        const std::set< Framework::Function >& functions,
        const QString& metric,
        TDETAILS& reduced_details)
{
    SmartPtr< std::map< Function,
                std::map< Framework::Thread,
                    std::map< Framework::StackTrace, TDETAIL > > > > raw_items;

    Queries::GetMetricValues( collector, metric.toStdString(), interval, threadGroup, functions,  // input - metric search criteria
                              raw_items );                                                        // output - raw metric values

    SmartPtr< std::map<Function, std::map<Framework::StackTrace, TDETAIL > > > data =
            Queries::Reduction::Apply( raw_items, Queries::Reduction::Summation );

    TALLDETAILS all_details;
    std::set< std::tuple< std::set< Function >, Function > > caller_function_list;

    for ( typename std::map< Function, std::map< Framework::StackTrace, TDETAIL > >::iterator iter = data->begin(); iter != data->end(); iter++ ) {
        const Framework::Function& function( iter->first );
        std::map< Framework::StackTrace, TDETAIL >& tracemap( iter->second );

        std::map< Framework::Thread, Framework::ExtentGroup > subextents_map;
        Get_Subextents_To_Object_Map( threadGroup, function, subextents_map );

        std::set< Framework::StackTrace, ltST > StackTraces_Processed;

        for ( typename std::map< Framework::StackTrace, TDETAIL >::iterator siter = tracemap.begin(); siter != tracemap.end(); siter++ ) {
            const Framework::StackTrace& stacktrace( siter->first );

            std::pair< std::set< Framework::StackTrace >::iterator, bool > ret = StackTraces_Processed.insert( stacktrace );
            if ( ! ret.second )
                continue;

            /* Find the extents associated with the stack trace's thread. */
            std::map< Framework::Thread, Framework::ExtentGroup >::iterator tei = subextents_map.find( stacktrace.getThread() );
            Framework::ExtentGroup subExtents;
            if (tei != subextents_map.end()) {
                subExtents = (*tei).second;
            }

            int64_t num_calls = ( subExtents.begin() == subExtents.end() ) ? 1 : stack_contains_N_calls( stacktrace, subExtents );

            const TDETAIL& detail( siter->second );

            int index;
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

            all_details.push_back( std::make_tuple( detail.dm_count, detail.dm_time / num_calls, function, caller ) );

            caller_function_list.insert( std::make_tuple( caller, function ) );
        }
    }

    std::vector<double> call_depths;

    detail_reduction( caller_function_list, all_details, reduced_details );

    std::ostringstream oss;
    generate_calltree_graph( functions, caller_function_list, call_depths, oss );
    std::cout << oss.str() << std::endl;

    for(int i=0; i<call_depths.size(); i++)
        std::cout << "    " << call_depths[i];
    std::cout << std::endl;
}
#endif

} // GUI
} // ArgoNavis

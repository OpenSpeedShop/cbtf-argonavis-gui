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

#include "managers/PerformanceDataManager.h"

#include "common/openss-gui-config.h"

#include <ArgoNavis/Base/StackTrace.hpp>
#include <ArgoNavis/Base/Time.hpp>

#include <ArgoNavis/CUDA/PerformanceData.hpp>
#include <ArgoNavis/CUDA/DataTransfer.hpp>
#include <ArgoNavis/CUDA/Device.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>
#include <ArgoNavis/CUDA/stringify.hpp>
#include <ArgoNavis/CUDA/Vector.hpp>

#include "ToolAPI.hxx"
#include "Queries.hxx"
#include "CUDAQueries.hxx"

#include "collectors/cuda/CUDACountsDetail.hxx"
#include "collectors/cuda/CUDAExecDetail.hxx"
#include "collectors/cuda/CUDAXferDetail.hxx"
#include "collectors/cuda/CUDACollector.hxx"

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <QtConcurrentRun>
#include <QFutureSynchronizer>
#include <QFileInfo>

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


QAtomicPointer< PerformanceDataManager > PerformanceDataManager::s_instance;


/**
 * @brief PerformanceDataManager::PerformanceDataManager
 * @param parent - the parent QObject (in any - ie non-NULL)
 *
 * Private constructor to construct a singleton PerformanceDataManager instance.
 */
PerformanceDataManager::PerformanceDataManager(QObject *parent)
    : QObject( parent )
{
    qRegisterMetaType< Base::Time >("Base::Time");
    qRegisterMetaType< CUDA::DataTransfer >("CUDA::DataTransfer");
    qRegisterMetaType< CUDA::KernelExecution >("CUDA::KernelExecution");
    qRegisterMetaType< QVector< QString > >("QVector< QString >");
}

/**
 * @brief PerformanceDataManager::~PerformanceDataManager
 *
 * Destroys the PerformanceDataManager instance.
 */
PerformanceDataManager::~PerformanceDataManager()
{
    qDebug() << "PerformanceDataManager::~PerformanceDataManager: TERMINATED";
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
 * @brief PerformanceDataManager::processDataTransferEvent
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the data transfer event
 * @param clusterName - the cluster group name
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
 * @param clusterName - the cluster group name
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * Emit signal for each periodic sample collected at the indicated experiment time.
 */
bool PerformanceDataManager::processPeriodicSample(const Base::Time& time_origin,
                                                   const Base::Time& time,
                                                   const std::vector<uint64_t>& counts,
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

        if ( 0 == i ) {
            emit addPeriodicSample( clusteringCriteriaName, clusterName, lastTimeStamp, timeStamp, value );
        }
    }

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::convert_performance_data
 * @param data - the performance data structure to be parsed
 * @param thread - the thread of interest
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @return - continue (=true) or not continue (=false) the visitation
 *
 * Initiate visitations for data transfer and kernel execution events and period sample data.
 * Emit signals for each of these to be handled by the performance data view to build graph items for plotting.
 */
bool PerformanceDataManager::processPerformanceData(const CUDA::PerformanceData& data,
                                                    const Base::ThreadName& thread,
                                                    const QString& clusteringCriteriaName)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QString clusterName = QString::fromStdString( thread.host() );
#else
    QString clusterName = QString( thread.host().c_str() );
#endif
    qDebug() << "PerformanceDataManager::processPerformanceData: cluster name: " << clusterName;

    data.visitDataTransfers(
                thread, data.interval(),
                boost::bind( &PerformanceDataManager::processDataTransferEvent, instance(),
                             boost::cref(data.interval().begin()), _1,
                             boost::cref(clusterName), boost::cref(clusteringCriteriaName) )
                );

    data.visitKernelExecutions(
                thread, data.interval(),
                boost::bind( &PerformanceDataManager::processKernelExecutionEvent, instance(),
                             boost::cref(data.interval().begin()), _1,
                             boost::cref(clusterName), boost::cref(clusteringCriteriaName) )
                );

    data.visitPeriodicSamples(
                thread, data.interval(),
                boost::bind( &PerformanceDataManager::processPeriodicSample, instance(),
                             boost::cref(data.interval().begin()), _1, _2,
                             boost::cref(clusterName), boost::cref(clusteringCriteriaName) )
                );

    return true; // continue the visitation
}

/**
 * @brief PerformanceDataManager::processMetricView
 * @param experiment - a reference to the experiment database
 * @param metric - the metric to generate data for
 * @param metricDesc - the metrics to generate data for
 *
 * Build function/statement view output for the specified metrics for all threads over the entire experiment time period.
 * NOTE: must be metrics providing time information.
 */
void PerformanceDataManager::processMetricView(const Experiment* experiment, const QString &metric, const QStringList &metricDesc)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::processMetricView STARTED" << metric;
#endif
    // Evaluate the first collector's time metric for all functions
    SmartPtr<std::map<Function, std::map<Thread, double> > > individual;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    std::string metricStr = metric.toStdString();
#else
    std::string metricStr = std::string( metric.toLatin1().data() );
#endif
    Queries::GetMetricValues(*experiment->getCollectors().begin(), metricStr,
                             TimeInterval(Time::TheBeginning(), Time::TheEnd()),
                             experiment->getThreads(),
                             experiment->getThreads().getFunctions(),
                             individual);
    SmartPtr<std::map<Function, double> > data =
            Queries::Reduction::Apply(individual, Queries::Reduction::Summation);
    individual = SmartPtr<std::map<Function, std::map<Thread, double> > >();

    // Sort the results
    std::multimap<double, Function> sorted;
    for(std::map<Function, double>::const_iterator
        i = data->begin(); i != data->end(); ++i)
        sorted.insert(std::make_pair(i->second, i->first));

    // Display the results

#ifdef HAS_PROCESS_METRIC_VIEW_DEBUG
    std::cout << std::endl << std::endl
              << std::setw(10) << metricDesc[0].toStdString()
              << "    "
              << metricDesc[1].toStdString() << std::endl
              << std::endl;
#endif

    emit addMetricView( metric, metricDesc );

    for(std::multimap<double, Function>::reverse_iterator
        i = sorted.rbegin(); i != sorted.rend(); ++i) {

        QVariantList metricData;

        double value( i->first * 1000.0 );

        metricData << QVariant::fromValue< double >( value );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        metricData << QString::fromStdString( i->second.getDemangledName() );
#else
        metricData << QString( i->second.getDemangledName().c_str() );
#endif

        emit addMetricViewData( metric, metricData );

#ifdef HAS_PROCESS_METRIC_VIEW_DEBUG
        std::cout << std::setw(10) << std::fixed << std::setprecision(3)
                  << value
                  << "    "
                  << i->second.getDemangledName();

        std::set<Statement> definitions = i->second.getDefinitions();
        for(std::set<Statement>::const_iterator
            i = definitions.begin(); i != definitions.end(); ++i)
            std::cout << " (" << i->getLinkedObject().getPath().getDirName() << i->getLinkedObject().getPath().getBaseName() << ": " << i->getPath().getDirName() << i->getPath().getBaseName()
                      << ", " << i->getLine() << ")";

        std::cout << std::endl;
#endif
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

    QFutureSynchronizer<void> synchronizer;

    QFuture<void> future = QtConcurrent::run( this, &PerformanceDataManager::loadCudaView, &experiment );
    synchronizer.addFuture( future );

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
    // Generate the default CUDA metric view
    const QString execTimeMetric = QStringLiteral( "exec_time" );
    const QString xferTimeMetric = QStringLiteral( "xfer_time" );
    QStringList metricDesc = QStringList() << "Exclusive Time (msec)" << "Function (defining location)";
    QFuture<void> future1 = QtConcurrent::run( this, &PerformanceDataManager::processMetricView, &experiment, xferTimeMetric, metricDesc );
    synchronizer.addFuture( future1 );
    QFuture<void> future2 = QtConcurrent::run( this, &PerformanceDataManager::processMetricView, &experiment, execTimeMetric, metricDesc );
    synchronizer.addFuture( future2 );
#endif

    synchronizer.waitForFinished();
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaViews: ENDED";
#endif

    emit loadComplete();
}

/**
 * @brief PerformanceDataManager::loadCudaView
 * @param experiment - experiment database object instance
 *
 * Parse the requested CUDA performance data to metric model data.
 */
void PerformanceDataManager::loadCudaView(const Experiment *experiment)
{
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaView STARTED!!";
#endif
    boost::optional<Collector> collector;
    CollectorGroup collectors = experiment->getCollectors();
    for ( CollectorGroup::const_iterator i = collectors.begin(); i != collectors.end(); ++i ) {
        if ( i->getMetadata().getUniqueId() == "cuda" ) {
            collector = *i;
            break;
        }
    }

    if ( ! collector )
        return;

    std::set<int> ranks;
    CUDA::PerformanceData data;
    QMap< Base::ThreadName, Thread> threads;

    ThreadGroup all_threads = experiment->getThreads();
    for (ThreadGroup::const_iterator i = all_threads.begin(); i != all_threads.end(); ++i) {
        std::pair<bool, int> rank = i->getMPIRank();

        if ( ranks.empty() || ( rank.first && (ranks.find(rank.second) != ranks.end() )) ) {
            GetCUDAPerformanceData( *collector, *i, data );
            threads.insert( ConvertToArgoNavis(*i), *i );
        }
    }

    double durationMs( 0.0 );
    if ( data.interval().empty() ) {
        return;
    }
    else {
        uint64_t duration = static_cast<uint64_t>(data.interval().end() - data.interval().begin());
        durationMs = qCeil( duration / 1000000.0 );
    }

    QVector< QString > sampleCounterNames;

    for ( std::vector<std::string>::size_type i = 0; i < data.counters().size(); ++i ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        sampleCounterNames << QString::fromStdString( data.counters()[i] );
#else
        sampleCounterNames << QString( data.counters()[i].c_str() );
#endif
    }

#if 0
    if ( sampleCounterNames.isEmpty() ) {
        sampleCounterNames << "Default";
    }
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QFileInfo fileInfo( QString::fromStdString( experiment->getName() ) );
#else
    QFileInfo fileInfo( QString( experiment->getName().c_str() ) );
#endif
    QString expName( fileInfo.fileName() );
    expName.replace( QString(".openss"), QString("") );

    const QString clusteringCriteriaName = QStringLiteral( "GPU Compute / Data Transfer Ratio" );

    QVector< QString > clusterNames;

#if 0
    for( int i=0; i<threads.size(); ++i ) {
        clusterNames << tr("Group of %1 Thread").arg(1);
    }
#else
    QMap< Base::ThreadName, Thread>::iterator iter( threads.begin() );
    while ( iter != threads.end() ) {
        Thread thread( *iter );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        clusterNames << QString::fromStdString( thread.getHost() );
#else
        clusterNames << QString( thread.getHost().c_str() );
#endif
        ++iter;
    }
#endif

    emit addExperiment( expName, clusteringCriteriaName, clusterNames, sampleCounterNames );

    foreach( const QString& clusterName, clusterNames ) {
        emit addCluster( clusteringCriteriaName, clusterName );
    }

    data.visitThreads( boost::bind(
                           &PerformanceDataManager::processPerformanceData, instance(),
                           boost::cref(data), _1, boost::cref(clusteringCriteriaName) ) );

    foreach( const QString& clusterName, clusterNames ) {
        emit setMetricDuration( clusteringCriteriaName, clusterName, durationMs );
    }

    // clear temporary data structures used during thread visitation
    m_sampleKeys.clear();
    m_sampleValues.clear();
    m_rawValues.clear();

#if !defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
    // Generate the default CUDA metric view
    const QString execTimeMetric = QStringLiteral( "exec_time" );
    const QString xferTimeMetric = QStringLiteral( "xfer_time" );
    QStringList metricDesc = QStringList() << "Exclusive Time (msec)" << "Function (defining location)";
    processMetricView( experiment, xferTimeMetric, metricDesc );
    processMetricView( experiment, execTimeMetric, metricDesc );
#endif
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


} // GUI
} // ArgoNavis

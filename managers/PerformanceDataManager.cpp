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

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <QtConcurrentRun>
#include <QFutureSynchronizer>
#include <QFileInfo>

#include <ArgoNavis/CUDA/PerformanceData.hpp>
#include <ArgoNavis/CUDA/DataTransfer.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>

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


QAtomicPointer< PerformanceDataManager > PerformanceDataManager::s_instance;


/**
 * @brief PerformanceDataManager::PerformanceDataManager
 * @param parent - the parent QObject (in any - ie non-NULL)
 *
 * Private constructor to construct a singleton PerformanceDataManager instance.
 */
PerformanceDataManager::PerformanceDataManager(QObject *parent)
    : QObject( parent )
    , m_processEvents( false )
    , m_renderer( Q_NULLPTR )
{
    qRegisterMetaType< Base::Time >("Base::Time");
    qRegisterMetaType< CUDA::DataTransfer >("CUDA::DataTransfer");
    qRegisterMetaType< CUDA::KernelExecution >("CUDA::KernelExecution");
    qRegisterMetaType< QVector< QString > >("QVector< QString >");

    if ( ! m_processEvents ) {
        m_renderer = new BackgroundGraphRenderer;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( this, &PerformanceDataManager::replotCompleted, m_renderer, &BackgroundGraphRenderer::signalProcessCudaEventView );
        connect( this, &PerformanceDataManager::graphRangeChanged, m_renderer, &BackgroundGraphRenderer::handleGraphRangeChanged );
        connect( m_renderer, &BackgroundGraphRenderer::signalCudaEventSnapshot, this, &PerformanceDataManager::addCudaEventSnapshot );
#else
        connect( this, SIGNAL(replotCompleted()), m_renderer, SIGNAL(signalProcessCudaEventView()) );
        connect( this, SIGNAL(graphRangeChanged(QString,double,double,QSize)), m_renderer, SLOT(handleGraphRangeChanged(QString,double,double,QSize)) );
        connect( m_renderer, SIGNAL(signalCudaEventSnapshot(QString,QString,double,double,QImage)), this, SIGNAL(addCudaEventSnapshot(QString,QString,double,double,QImage)) );
#endif
    }
}

/**
 * @brief PerformanceDataManager::~PerformanceDataManager
 *
 * Destroys the PerformanceDataManager instance.
 */
PerformanceDataManager::~PerformanceDataManager()
{
    qDebug() << "PerformanceDataManager FINISHED";
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
        else if ( 1 == i ) {
            emit addPeriodicSample( clusteringCriteriaName, clusterName+" (GPU)", lastTimeStamp, timeStamp, value );
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
#ifdef HAS_STRIP_DOMAIN_NAME
    int index = clusterName.indexOf( '.' );
    if ( index > 0 )
        clusterName = clusterName.left( index );
#endif
    qDebug() << "PerformanceDataManager::processPerformanceData: cluster name: " << clusterName;

    if ( m_processEvents ) {
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
    }

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
 * @param collector - a copy of the cuda collector
 * @param threads - all threads known by the cuda collector
 * @param metric - the metric to generate data for
 * @param metricDesc - the metrics to generate data for
 *
 * Build function/statement view output for the specified metrics for all threads over the entire experiment time period.
 * NOTE: must be metrics providing time information.
 */
void PerformanceDataManager::processMetricView(const Collector collector, const ThreadGroup& threads, const QString &metric, const QStringList &metricDesc)
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
    Queries::GetMetricValues( collector,
                              metricStr,
                              TimeInterval(Time::TheBeginning(), Time::TheEnd()),
                              threads,
                              threads.getFunctions(),
                              individual );
    SmartPtr<std::map<Function, double> > data =
            Queries::Reduction::Apply( individual, Queries::Reduction::Summation );
    individual = SmartPtr<std::map<Function, std::map<Thread, double> > >();

    // Sort the results
    std::multimap<double, Function> sorted;
    double total( 0.0 );
    for( std::map<Function, double>::const_iterator i = data->begin(); i != data->end(); ++i ) {
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

    emit addMetricView( metric, metricDesc );

    for ( std::multimap<double, Function>::reverse_iterator i = sorted.rbegin(); i != sorted.rend(); ++i ) {
        QVariantList metricData;

        double value( i->first * 1000.0 );
        double percentage( i->first / total * 100.0 );

        metricData << QVariant::fromValue< double >( value );
        metricData << QVariant::fromValue< double >( percentage );
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

    const QString timeMetric( "time" );
    QStringList metricList;
    QStringList metricDescList;
    const QString functionTitle( tr("Function (defining location)") );

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
        QString experimentName = QString::fromStdString( experiment.getName() );
#else
        QString experimentName( QString( experiment.getName().c_str() ) );
#endif

        QFuture<void> future = QtConcurrent::run( this, &PerformanceDataManager::loadCudaView, experimentName, collector.get(), experiment.getThreads() );
        synchronizer.addFuture( future );

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
        QMap< QString, QFuture<void> > futures;
#endif
        foreach ( const QString& metric, metricList ) {
            QStringList metricDesc;
            metricDesc << metricDescList.takeFirst() << metricDescList.takeFirst() << functionTitle;
#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
            futures[metric] = QtConcurrent::run( this, &PerformanceDataManager::processMetricView, collector.get(), experiment.getThreads(), metric, metricDesc );
            synchronizer.addFuture( futures[ metric ] );
#else
            processMetricView( collector.get(), experiment.getThreads(), metric, metricDesc );
#endif
        }

#if defined(HAS_OSSCUDA2XML)
        PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
        if ( dataMgr )
            dataMgr->xmlDump( filePath );
#endif

        synchronizer.waitForFinished();
    }

#if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG)
    qDebug() << "PerformanceDataManager::loadCudaViews: ENDED";
#endif

    emit loadComplete();
}

/**
 * @brief PerformanceDataManager::unloadCudaViews
 * @param clusteringCriteriaName
 * @param clusterNames
 */
void PerformanceDataManager::unloadCudaViews(const QString &clusteringCriteriaName, const QStringList &clusterNames)
{
    if ( m_renderer ) {
        m_renderer->unloadCudaViews( clusteringCriteriaName, clusterNames );
    }

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

    std::set<int> ranks;
    CUDA::PerformanceData data;
    QMap< Base::ThreadName, Thread> threads;
    bool hasCudaCollector( false );
    bool hasGpuCounts( false );

    for (ThreadGroup::const_iterator i = all_threads.begin(); i != all_threads.end(); ++i) {
        std::pair<bool, int> rank = i->getMPIRank();

        if ( ranks.empty() || ( rank.first && (ranks.find(rank.second) != ranks.end() )) ) {
            if ( "cuda" == collector.getMetadata().getUniqueId() ) {
                GetCUDAPerformanceData( collector, *i, data );
                hasCudaCollector = true;
            }
            threads.insert( ConvertToArgoNavis(*i), *i );
        }
    }

    double durationMs( 0.0 );
    if ( ! data.interval().empty() ) {
        uint64_t duration = static_cast<uint64_t>(data.interval().end() - data.interval().begin());
        durationMs = qCeil( duration / 1000000.0 );
    }

    QVector< QString > sampleCounterNames;

    for ( std::vector<std::string>::size_type i = 0; i < data.counters().size(); ++i ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString sampleCounterName = QString::fromStdString( data.counters()[i] );
#else
        QString sampleCounterName( data.counters()[i].c_str() );
#endif
        sampleCounterNames << sampleCounterName;
        if ( sampleCounterName.contains( QStringLiteral("inst_executed") ) )
            hasGpuCounts = true;
    }

#if 0
    if ( sampleCounterNames.isEmpty() ) {
        sampleCounterNames << "Default";
    }
#endif

    QFileInfo fileInfo( experimentName );
    QString expName( fileInfo.fileName() );
    expName.replace( QString(".openss"), QString("") );

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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString hostName = QString::fromStdString( thread.getHost() );
#else
        QString hostName = QString( thread.getHost().c_str() );
#endif
#ifdef HAS_STRIP_DOMAIN_NAME
        int index = hostName.indexOf( '.' );
        if ( index > 0 )
            hostName = hostName.left( index );
#endif
        clusterNames << hostName;
        if ( hasGpuCounts )
            clusterNames << ( hostName + " (GPU)" );
        ++iter;
    }
#endif

    emit addExperiment( expName, clusteringCriteriaName, clusterNames, sampleCounterNames );

    if ( hasCudaCollector ) {
        if ( ! m_processEvents ) {
            m_renderer->setPerformanceData( clusteringCriteriaName, clusterNames, data );
        }

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


} // GUI
} // ArgoNavis

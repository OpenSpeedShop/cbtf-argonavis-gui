/*!
   \file PerformanceDataManager.h
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

#ifndef PERFORMANCEDATAMANAGER_H
#define PERFORMANCEDATAMANAGER_H

#include <QObject>

#include <QMap>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QAtomicPointer>
#include <QFutureSynchronizer>
#include <QMutex>

#include <vector>
// use either std::tuple or boost::tuple
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <tuple>
using namespace std;
#else
#include <QUuid>
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
using namespace boost;
#endif

#include "common/openss-gui-config.h"

#include "boost/optional.hpp"

#include "TimeInterval.hxx"
#include "AddressRange.hxx"
#include "Function.hxx"
#include "LinkedObject.hxx"
#include "Loop.hxx"
#include "Statement.hxx"
#include "Collector.hxx"

#include "UserGraphRangeChangeManager.h"
#include "CBTF-ArgoNavis-Ext/NameValueDefines.h"
#include "widgets/MetricViewManager.h"
#include "widgets/ShowDeviceDetailsDialog.h"
#include "managers/CalltreeGraphManager.h"

namespace OpenSpeedShop {
namespace Framework {
class Experiment;
class ThreadGroup;
}
}

class QTimer;
class QCPAxisRect;
class QTextStream;


namespace ArgoNavis {


namespace Base {
class Time;
class ThreadName;
}

namespace CUDA {
class PerformanceData;
class DataTransfer;
class KernelExecution;
}


namespace GUI {


class BackgroundGraphRenderer;


class PerformanceDataManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PerformanceDataManager)

public:

    static PerformanceDataManager* instance();

    static void destroy();

    void loadCudaViews(const QString& filePath);
    void unloadCudaViews(const QString& clusteringCriteriaName, const QStringList& clusterNames);

#if defined(HAS_OSSCUDA2XML)
    void xmlDump(const QString& filePath);
#endif

public slots:

    void asyncLoadCudaViews(const QString& filePath);

    void handleRequestMetricView(const QString& clusterName, const QString& metric, const QString& view);
    void handleProcessDetailViews(const QString& clusterName);

signals:

    void signalSetDefaultMetricView(const MetricViewTypes viewType);

    void addExperiment(const QString& name,
                       const QString& clusteringCriteriaName,
                       const QVector< QString >& clusterNames,
                       const QVector< bool >& clusterHasGpuSampleCounters,
                       const QVector< QString >& sampleCounterNames);

    void addDevice(const int deviceNumber, const int definedDeviceNumber, const NameValueList& attributes, const NameValueList& maximumLimits);

    void addDataTransfer(const QString &clusteringCriteriaName,
                         const QString &clusterName,
                         const Base::Time &time_origin,
                         const CUDA::DataTransfer &details);

    void addKernelExecution(const QString& clusteringCriteriaName,
                            const QString& clusterName,
                            const Base::Time& time_origin,
                            const CUDA::KernelExecution& details);

    void addPeriodicSample(const QString& clusteringCriteriaName,
                           const QString& clusterName,
                           const double& time_begin,
                           const double& time_end,
                           const double& count);

    void addCudaEventSnapshot(const QString& clusteringCriteriaName, const QString& clusteringName, double lower, double upper, const QImage& image);

    void addMetricView(const QString& clusterName, const QString& metricName, const QString& viewName, const QStringList& metrics);
    void addAssociatedMetricView(const QString& clusterName, const QString& metricName, const QString& viewName, const QString& attachedMetricViewName, const QStringList& metrics);

    void addMetricViewData(const QString& clusterName, const QString& metricName, const QString& viewName, const QVariantList& data, const QStringList& columnHeaders = QStringList());

    void addCluster(const QString& clusteringCriteriaName, const QString& clusterName);
    void removeCluster(const QString& clusteringCriteriaName, const QString& clusterName);

    void setMetricDuration(const QString& clusteringCriteriaName, const QString& clusterName, double duration, bool yAxisPercentage);

    void graphRangeChanged(const QString& clusterName, double lower, double upper, const QSize& size);

    void metricViewRangeChanged(const QString& clusterName, const QString& metricName, const QString& viewName, double lower, double upper);

    void loadComplete();

    void requestMetricViewComplete(const QString& clusterName, const QString& metricName, const QString& viewName, double lower, double upper);

    void signalDisplayCalltreeGraph(const QString& graph);

private slots:

    void handleLoadCudaMetricViews(const QString& clusterName, double lower, double upper);

    void handleLoadCudaMetricViewsTimeout(const QString& clusterName, double lower, double upper);

private:

    explicit PerformanceDataManager(QObject* parent = 0);
    virtual ~PerformanceDataManager();

    bool getPerformanceData(const OpenSpeedShop::Framework::Collector& collector,
                            const OpenSpeedShop::Framework::ThreadGroup& all_threads,
                            const QMap< Base::ThreadName, bool >& threadSet,
                            QMap< Base::ThreadName, OpenSpeedShop::Framework::Thread>& threads,
                            CUDA::PerformanceData& data);

    void loadCudaView(const QString& experimentName,
                      const OpenSpeedShop::Framework::Collector& collector,
                      const OpenSpeedShop::Framework::ThreadGroup& all_threads);

    void loadCudaMetricViews(
                             #if defined(HAS_PARALLEL_PROCESS_METRIC_VIEW)
                             QFutureSynchronizer<void>& synchronizer,
                             QMap< QString, QFuture<void> >& futures,
                             #endif
                             const QStringList& metricList,
                             const QStringList& viewList,
                             QStringList metricDescList,
                             const OpenSpeedShop::Framework::Collector& collector,
                             const OpenSpeedShop::Framework::Experiment& experiment,
                             const OpenSpeedShop::Framework::TimeInterval& interval);

    template <typename TS>
    void processMetricView(const OpenSpeedShop::Framework::Collector collector,
                           const OpenSpeedShop::Framework::ThreadGroup& threads,
                           const OpenSpeedShop::Framework::TimeInterval& interval,
                           const QString &metric,
                           const QStringList &metricDesc);

    template <typename TS>
    std::set<TS> getThreadSet(const OpenSpeedShop::Framework::ThreadGroup& threads) { }

    template <typename TS>
    QString getLocationInfo(const TS& metric) { Q_UNUSED(metric) return QString(); }

    template <typename TS>
    QString getViewName() const { return QString("CallTree"); }

    typedef tuple< int64_t, double, OpenSpeedShop::Framework::Function, std::set< OpenSpeedShop::Framework::Function > > all_details_data_t;
    typedef std::vector< all_details_data_t > TALLDETAILS;

    typedef tuple< int64_t, double, OpenSpeedShop::Framework::Function, uint32_t > details_data_t;  // count, time, Function, calltree depth
    typedef std::vector< details_data_t > TDETAILS;
    typedef std::set< tuple< std::set< OpenSpeedShop::Framework::Function >, OpenSpeedShop::Framework::Function > > FunctionSet;

    typedef std::pair< OpenSpeedShop::Framework::Function, OpenSpeedShop::Framework::Function > FunctionCallPair;
    typedef std::map< FunctionCallPair, CalltreeGraphManager::handle_t > CallPairToEdgeMap;
    typedef std::map< FunctionCallPair, double > CallPairToWeightMap;

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    template <int N, typename BinaryPredicate, typename ForwardIterator>
    bool ComponentBinaryPredicate(const typename ForwardIterator::value_type& x, const typename ForwardIterator::value_type& y);
#endif

    template <int N, typename BinaryPredicate, typename ForwardIterator>
    void sortByFixedComponent (ForwardIterator first, ForwardIterator last);

    bool partition_sort(const OpenSpeedShop::Framework::Function &function,
                        const std::set<OpenSpeedShop::Framework::Function> &callingFunctionSet,
                        const all_details_data_t& d);

    void print_details(const std::string& details_name, const TDETAILS &details) const;

    void detail_reduction(const FunctionSet& caller_function_list,
                          std::map< OpenSpeedShop::Framework::Function, uint32_t >& call_depth_map,
                          TALLDETAILS &all_details,
                          CallPairToWeightMap& callPairToWeightMap,
                          TDETAILS& reduced_details);

    void generate_calltree_graph(
            CalltreeGraphManager& graphManager,
            const std::set< OpenSpeedShop::Framework::Function >& functions,
            const FunctionSet& caller_function_list,
            std::map< OpenSpeedShop::Framework::Function, uint32_t>& function_call_depth_map,
            CallPairToEdgeMap& callPairToEdgeMap);

    template <typename DETAIL_t>
    std::pair< uint64_t, double > getDetailTotals(const DETAIL_t& detail, const double factor) { return std::make_pair( detail.dm_count, detail.dm_time / factor ); }

    template <typename DETAIL_t>
    void ShowCalltreeDetail(const OpenSpeedShop::Framework::Collector& collector,
                            const OpenSpeedShop::Framework::ThreadGroup& threadGroup,
                            const OpenSpeedShop::Framework::TimeInterval& interval,
                            const std::set< OpenSpeedShop::Framework::Function > functions,
                            const QString metric,
                            const QStringList metricDesc);

    void processCalltreeView(const OpenSpeedShop::Framework::Collector collector,
                             const OpenSpeedShop::Framework::ThreadGroup& threads,
                             const OpenSpeedShop::Framework::TimeInterval& interval);

    bool processDataTransferEvent(const ArgoNavis::Base::Time& time_origin,
                                  const ArgoNavis::CUDA::DataTransfer& details,
                                  const QString& clusterName,
                                  const QString& clusteringCriteriaName);

    bool processKernelExecutionEvent(const ArgoNavis::Base::Time& time_origin,
                                     const ArgoNavis::CUDA::KernelExecution& details,
                                     const QString& clusterName,
                                     const QString& clusteringCriteriaName);

    bool processPeriodicSample(const ArgoNavis::Base::Time& time_origin,
                               const ArgoNavis::Base::Time& time,
                               const std::vector<uint64_t>& counts,
                               const QSet< int >& gpuCounterIndexes,
                               const QString& clusterName,
                               const QString& clusteringCriteriaName);

    bool processPerformanceData(const ArgoNavis::CUDA::PerformanceData& data,
                                const ArgoNavis::Base::ThreadName& thread,
                                const QSet< int >& gpuCounterIndexes,
                                const QString& clusteringCriteriaName);

    // visitor functions to process CUDA details view

    bool processDataTransferDetails(const QString& clusterName, const Base::Time &time_origin, const CUDA::DataTransfer &details);

    bool processKernelExecutionDetails(const QString& clusterName, const Base::Time &time_origin, const CUDA::KernelExecution &details);

    // visitor functions to determine whether thread has CUDA events

    bool hasDataTransferEvents(const CUDA::DataTransfer& details,
                               bool& flag);

    bool hasKernelExecutionEvents(const CUDA::KernelExecution& details,
                                  bool& flag);

    bool hasCudaPeriodicSamples(const QSet< int >& gpuCounterIndexes,
                                const std::vector<uint64_t>& counts,
                                bool& flag);

    bool hasCudaEvents(const ArgoNavis::CUDA::PerformanceData& data,
                       const QSet< int >& gpuCounterIndexes,
                       const ArgoNavis::Base::ThreadName& thread,
                       QMap< Base::ThreadName, bool >& flags);

private:

    static QAtomicPointer< PerformanceDataManager > s_instance;

    static QString s_functionTitle;
    static QString s_minimumTitle;
    static QString s_maximumTitle;
    static QString s_meanTitle;

    QVector<double> m_sampleKeys;
    QMap< int, QVector<double> > m_sampleValues;
    QMap< int, QVector<double> > m_rawValues;

    BackgroundGraphRenderer* m_renderer;

#if defined(HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE)
    // thread for BackgroundGraphRenderer instance
    QThread m_thread;
#endif

    typedef struct {
        QStringList metricList;
        QStringList viewList;                 // processed metric views
        QStringList tableColumnHeaders;       // table column headers for metric views
        QStringList metricViewList;           // [ <metric name> | "Details" ] - [ <View Name> ]
        QString experimentFilename;
        OpenSpeedShop::Framework::TimeInterval interval;
    } MetricTableViewInfo;

    QMap< QString, MetricTableViewInfo > m_tableViewInfo;

    UserGraphRangeChangeManager m_userChangeMgr;

    struct {
        bool operator() (const details_data_t& lhs, const details_data_t& rhs) {
            return ( get<3>(lhs) < get<3>(rhs) ) || ( get<3>(lhs) == get<3>(rhs) && get<1>(lhs) > get<1>(rhs) );
        }
    } details_compare;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAMANAGER_H

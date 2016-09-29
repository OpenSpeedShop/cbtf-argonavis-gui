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
#if (QT_VERSION < QT_VERSION_CHECK(4, 8, 0))
#include <QUuid>
#endif

#include <vector>

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

signals:

    void addExperiment(const QString& name,
                       const QString& clusteringCriteriaName,
                       const QVector< QString >& clusterNames,
                       const QVector< QString >& sampleCounterNames);

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

    void addMetricViewData(const QString& clusterName, const QString& metricName, const QString& viewName, const QVariantList& data);

    void addCluster(const QString& clusteringCriteriaName, const QString& clusterName);

    void setMetricDuration(const QString& clusteringCriteriaName, const QString& clusterName, double duration);

    void graphRangeChanged(const QString& clusterName, double lower, double upper, const QSize& size);

    void replotCompleted();

    void loadComplete();

    void requestMetricViewComplete(const QString& clusterName, const QString& metricName, const QString& viewName);

private slots:

    void handleLoadCudaMetricViews(const QString& clusterName, double lower, double upper);

    void handleLoadCudaMetricViewsTimeout(const QString& clusterName, double lower, double upper);

private:

    explicit PerformanceDataManager(QObject* parent = 0);
    virtual ~PerformanceDataManager();

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
                             boost::optional<OpenSpeedShop::Framework::Collector>& collector,
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
    QString getViewName() const { return QString(); }

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

private:

    static QAtomicPointer< PerformanceDataManager > s_instance;

    QVector<double> m_sampleKeys;
    QMap< int, QVector<double> > m_sampleValues;
    QMap< int, QVector<double> > m_rawValues;
    QStringList m_gpuCounterNames;

    bool m_processEvents;

    BackgroundGraphRenderer* m_renderer;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)) && defined(HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE)
    // thread for BackgroundGraphRenderer instance
    QThread m_thread;
#endif

    typedef struct {
        QStringList metricList;
        QStringList viewList;
        QStringList tableColumnHeaders;
        QString experimentFilename;
        OpenSpeedShop::Framework::TimeInterval interval;
    } MetricTableViewInfo;

    QMap< QString, MetricTableViewInfo > m_tableViewInfo;

    UserGraphRangeChangeManager m_userChangeMgr;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAMANAGER_H

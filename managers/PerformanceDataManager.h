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

#include "boost/optional.hpp"

#include "ToolAPI.hxx"
/*
namespace OpenSpeedShop {
namespace Framework {
class Experiment;
class Collector;
class ThreadGroup;
class TimeInterval;
}
}
*/
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

    void addMetricView(const QString& metricView, const QStringList& metrics);

    void addMetricViewData(const QString& metricView, const QVariantList& data);

    void addCluster(const QString& clusteringCriteriaName, const QString& clusterName);

    void setMetricDuration(const QString& clusteringCriteriaName, const QString& clusterName, double duration);

    void graphRangeChanged(const QString& clusterName, double lower, double upper, const QSize& size);

    void replotCompleted();

    void loadComplete();

private slots:

    void handleLoadCudaMetricViews(const QString& clusterName, double lower, double upper);

    void handleLoadCudaMetricViewsTimeout();

    void threadDestroyed();
    void timerDestroyed();

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
                             QStringList metricDescList,
                             boost::optional<OpenSpeedShop::Framework::Collector>& collector,
                             const OpenSpeedShop::Framework::Experiment& experiment,
                             const OpenSpeedShop::Framework::TimeInterval& interval);

    void processMetricView(const OpenSpeedShop::Framework::Collector collector,
                           const OpenSpeedShop::Framework::ThreadGroup& threads,
                           const OpenSpeedShop::Framework::TimeInterval& interval,
                           const QString &metric,
                           const QStringList &metricDesc);

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
                               const QString& clusterName,
                               const QString& clusteringCriteriaName);

    bool processPerformanceData(const ArgoNavis::CUDA::PerformanceData& data,
                                const ArgoNavis::Base::ThreadName& thread,
                                const QString& clusteringCriteriaName);

private:

    static QAtomicPointer< PerformanceDataManager > s_instance;

    QVector<double> m_sampleKeys;
    QMap< int, QVector<double> > m_sampleValues;
    QMap< int, QVector<double> > m_rawValues;

    bool m_processEvents;

    BackgroundGraphRenderer* m_renderer;

    typedef struct {
        QStringList metricList;
        QStringList tableColumnHeaders;
        QString experimentFilename;
    } MetricTableViewInfo;

    QMap< QString, MetricTableViewInfo > m_tableViewInfo;

    QMutex m_mutex;
    QMap< QString, QThread* > m_timerThreads;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAMANAGER_H

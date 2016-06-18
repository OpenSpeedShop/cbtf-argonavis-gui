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

#include "ToolAPI.hxx"

namespace OpenSpeedShop {
namespace Framework {
class Experiment;
}
}

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


class PerformanceDataManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PerformanceDataManager)

public:

    static PerformanceDataManager* instance();

    static void destroy();

    void loadCudaViews(const QString& filePath);

#if defined(HAS_OSSCUDA2XML)
    void xmlDump(const QString& filePath);
#endif

public slots:

    void asyncLoadCudaViews(const QString& filePath);

signals:

    void addExperiment(const QString& name,
                       const QString& groupName,
                       const QVector< QString >& clusterNames,
                       const QVector< QString >& sampleCounterNames);

    void addDataTransfer(const QString &metricGroupName,
                         const QString &metricName,
                         const Base::Time &time_origin,
                         const CUDA::DataTransfer &details);

    void addKernelExecution(const QString& metricGroupName,
                            const QString& metricName,
                            const Base::Time& time_origin,
                            const CUDA::KernelExecution& details);

    void addPeriodicSample(const QString& metricGroupName,
                           int counterIndex,
                           const double& time_begin,
                           const double& time_end,
                           const double& count);

    void addMetricView(const QString& metricView, const QStringList& metrics);

    void addMetricViewData(const QString& metricView, const QVariantList& data);

    void addMetric(const QString& metricGroupName, const QString& metricName);

    void setMetricDuration(const QString& metricGroupName, const QString& metricName, double duration);

    void loadComplete();

private:

    explicit PerformanceDataManager(QObject* parent = 0);
    virtual ~PerformanceDataManager();

    void loadCudaView(const OpenSpeedShop::Framework::Experiment* experiment);

    void processMetricView(const OpenSpeedShop::Framework::Experiment* experiment,
                           const QString &metric,
                           const QStringList &metricDesc);

    bool processDataTransferEvent(const ArgoNavis::Base::Time& time_origin,
                                  const ArgoNavis::CUDA::DataTransfer& details,
                                  const QVector< QString >& metricNames,
                                  const QString& metricGroupName);

    bool processKernelExecutionEvent(const ArgoNavis::Base::Time& time_origin,
                                     const ArgoNavis::CUDA::KernelExecution& details,
                                     const QVector< QString >& metricNames,
                                     const QString& metricGroupName);

    bool processPeriodicSample(const ArgoNavis::Base::Time& time_origin,
                               const ArgoNavis::Base::Time& time,
                               const std::vector<uint64_t>& counts,
                               const QString& metricGroupName);

    bool convert_performance_data(const ArgoNavis::CUDA::PerformanceData& data,
                                  const ArgoNavis::Base::ThreadName& thread,
                                  const QVector< QString >& metricNames,
                                  const QString& metricGroupName);

private:

    static QAtomicPointer< PerformanceDataManager > s_instance;

    QVector<double> m_sampleKeys;
    QMap< int, QVector<double> > m_sampleValues;
    QMap< int, QVector<double> > m_rawValues;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAMANAGER_H

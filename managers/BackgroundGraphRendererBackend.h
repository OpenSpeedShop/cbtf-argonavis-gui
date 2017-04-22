/*!
   \file BackgroundGraphRenderer.h
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

#ifndef BACKGROUNDGRAPHRENDERERBACKEND_H
#define BACKGROUNDGRAPHRENDERERBACKEND_H

#include <QObject>
#include <QFutureWatcher>

#include <ArgoNavis/CUDA/PerformanceData.hpp>
#include <ArgoNavis/CUDA/DataTransfer.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>

class QCustomPlot;


namespace ArgoNavis { namespace GUI {


class BackgroundGraphRendererBackend : public QObject
{
    Q_OBJECT

public:

    explicit BackgroundGraphRendererBackend(const QString& clusteringCriteriaName, const CUDA::PerformanceData& data, QObject *parent = 0);
    virtual ~BackgroundGraphRendererBackend();

signals:

    void signalProcessCudaEventViewStart();
    void signalProcessCudaEventViewDone();

    void addDataTransfer(const QString& clusteringName,
                         const Base::Time &time_origin,
                         const CUDA::DataTransfer &details,
                         bool last = false);

    void addKernelExecution(const QString& clusteringName,
                            const Base::Time& time_origin,
                            const CUDA::KernelExecution& details,
                            bool last = false);

private slots:

    void handleProcessCudaEventView();
    void handleProcessCudaEventViewDone();

private:

    bool processThreadCudaEvents(const Base::ThreadName& thread);
    bool processDataTransferEvent(const QString& clusteringName, const Base::Time &time_origin, const CUDA::DataTransfer &details);
    bool processKernelExecutionEvent(const QString& clusteringName, const Base::Time &time_origin, const CUDA::KernelExecution &details);

private:

    CUDA::PerformanceData m_data;

};


} // GUI
} // ArgoNavis

#endif // BACKGROUNDGRAPHRENDERERBACKEND_H

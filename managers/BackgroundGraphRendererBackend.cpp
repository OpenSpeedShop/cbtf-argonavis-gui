/*!
   \file BackgroundGraphRenderer.cpp
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

#include "BackgroundGraphRendererBackend.h"

#include <QtConcurrentRun>
#include <QFutureSynchronizer>
#include <QDebug>

#include "common/openss-gui-config.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>


namespace ArgoNavis { namespace GUI {


/**
 * @brief BackgroundGraphRendererBackend::BackgroundGraphRendererBackend
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @param data - the CUDA performance data object for the clustering criteria
 * @param parent - the parent QWidget instance
 *
 * Constructs a BackgroundGraphRendererBackend instance
 */
BackgroundGraphRendererBackend::BackgroundGraphRendererBackend(const QString& clusteringCriteriaName, const CUDA::PerformanceData& data, QObject *parent)
    : QObject( parent )
    , m_data( data )
{
    connect( this, &BackgroundGraphRendererBackend::signalProcessCudaEventViewStart, this, &BackgroundGraphRendererBackend::handleProcessCudaEventView );
}

/**
 * @brief BackgroundGraphRendererBackend::~BackgroundGraphRendererBackend
 *
 * Destroys the BackgroundGraphRendererBackend instance.
 */
BackgroundGraphRendererBackend::~BackgroundGraphRendererBackend()
{

}

/**
 * @brief BackgroundGraphRenderer::processDataTransferEvent
 * @param clusterName - the cluster group name
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the data transfer event
 *
 * Emit a signal so a handler in the GUI thread can generate the data transfer graph item from the details.
 */
bool BackgroundGraphRendererBackend::processDataTransferEvent(const QString& clusteringName,
                                                              const Base::Time &time_origin,
                                                              const CUDA::DataTransfer &details)
{
    emit addDataTransfer( clusteringName, time_origin, details );

    return true; // continue the visitation
}

/**
 * @brief BackgroundGraphRenderer::processKernelExecutionEvent
 * @param clusteringName - the cluster group name
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the kernel execution event
 *
 * Emit a signal so a handler in the GUI thread can generate the kernel execution graph item from the details.
 */
bool BackgroundGraphRendererBackend::processKernelExecutionEvent(const QString& clusteringName,
                                                                 const Base::Time &time_origin,
                                                                 const CUDA::KernelExecution &details)
{
    emit addKernelExecution( clusteringName, time_origin, details );

    return true; // continue the visitation
}

/**
 * @brief BackgroundGraphRenderer::handleProcessCudaEventView
 *
 * Handler to initiate processing of the CUDA events for all threads in the performance data object maintained by this class.
 * A signal is emitted when the processing completes (signalProcessCudaEventViewDone).
 */
void BackgroundGraphRendererBackend::handleProcessCudaEventView()
{
    qDebug() << "BackgroundGraphRendererBackend::handleProcessCudaEventView: STARTED!!";

    m_watcher = new QFutureWatcher<void>();

    if ( m_watcher ) {
        connect( m_watcher, &QFutureWatcher<void>::finished, this, &BackgroundGraphRendererBackend::signalProcessCudaEventViewDone );
        connect( m_watcher, &QFutureWatcher<void>::finished, m_watcher, &QFutureWatcher<void>::deleteLater );

        m_watcher->setFuture( QtConcurrent::run( &m_data, &CUDA::PerformanceData::visitThreads,
                                                 boost::bind( &BackgroundGraphRendererBackend::processThreadCudaEvents, this, _1 ) ) );
    }
}

/**
 * @brief BackgroundGraphRenderer::processThreadCudaEvents
 * @param thread - the current thread to be processed
 *
 * This method provides a "visitor" implementation for the visitor design pattern used to process each thread in the performance
 * data object.
 */
bool BackgroundGraphRendererBackend::processThreadCudaEvents(const Base::ThreadName& thread)
{
    QString clusterName = QString::fromStdString( thread.host() );
#ifdef HAS_STRIP_DOMAIN_NAME
    int index = clusterName.indexOf( '.' );
    if ( index > 0 )
        clusterName = clusterName.left( index );
#endif
    qDebug() << "BackgroundGraphRendererBackend::processThreadCudaEvents: STARTED: thread=" << clusterName;

    // concurrently initiate visitations of the CUDA data transfer and kernel execution events
    QFutureSynchronizer<void> synchronizer;
    QFuture<void> future1 = QtConcurrent::run( &m_data, &CUDA::PerformanceData::visitDataTransfers, thread, m_data.interval(),
                                               boost::bind( &BackgroundGraphRendererBackend::processDataTransferEvent, this,
                                                            boost::cref(clusterName), boost::cref(m_data.interval().begin()), _1 ) );
    synchronizer.addFuture( future1 );

    QFuture<void> future2 = QtConcurrent::run( &m_data, &CUDA::PerformanceData::visitKernelExecutions, thread, m_data.interval(),
                                               boost::bind( &BackgroundGraphRendererBackend::processKernelExecutionEvent, this,
                                                            boost::cref(clusterName), boost::cref(m_data.interval().begin()), _1 ) );
    synchronizer.addFuture( future2 );

    // wait for the visitations to complete
    synchronizer.waitForFinished();

    qDebug() << "BackgroundGraphRendererBackend::processThreadCudaEvents: DONE: thread=" << clusterName;

    return true; // continue the visitation
}


} // GUI
} // ArgoNavis

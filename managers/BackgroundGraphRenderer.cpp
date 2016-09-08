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

#include "BackgroundGraphRenderer.h"

#include "BackgroundGraphRendererBackend.h"

#include "QCustomPlot/CustomPlot.h"

#include "graphitems/OSSDataTransferItem.h"
#include "graphitems/OSSKernelExecutionItem.h"

#include <QImage>
#include <QtConcurrentRun>


namespace ArgoNavis { namespace GUI {


/**
 * @brief BackgroundGraphRenderer::BackgroundGraphRenderer
 * @param parent - the parent QObject instance
 *
 * Constructs a BackgroundGraphRenderer instance
 */
BackgroundGraphRenderer::BackgroundGraphRenderer(QObject *parent)
    : QObject( parent )
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "BackgroundGraphRenderer::BackgroundGraphRenderer: thread=" << QString::number((long long)QThread::currentThread(), 16);
    qDebug() << "BackgroundGraphRenderer::BackgroundGraphRenderer: &m_thread=" << QString::number((long long)&m_thread, 16);
#endif

    // setup signal-to-slot connection for creating QCustomPlot instance in the GUI thread
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( this, &BackgroundGraphRenderer::createPlotForClustering, this, &BackgroundGraphRenderer::handleCreatePlotForClustering, Qt::QueuedConnection );
    connect( &m_userChangeMgr, &UserGraphRangeChangeManager::timeout, this, &BackgroundGraphRenderer::handleGraphRangeChangedTimeout );
#else
    connect( this, SIGNAL(createPlotForClustering(QString,QString)), this, SLOT(handleCreatePlotForClustering(QString,QString)), Qt::QueuedConnection );
    connect( &m_userChangeMgr, SIGNAL(timeout(QString,double,double,QSize)), this, SLOT(handleGraphRangeChangedTimeout(QString,double,double,QSize)) );
#endif

    // start thread for backend processing
    m_thread.start();
}

/**
 * @brief BackgroundGraphRenderer::~BackgroundGraphRenderer
 *
 * Destroys the BackgroundGraphRenderer instance.
 */
BackgroundGraphRenderer::~BackgroundGraphRenderer()
{
    // stop thread and wait for termination
    m_thread.quit();
    m_thread.wait();
}

/**
 * @brief BackgroundGraphRenderer::setPerformanceData
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @param clusterNames - the list of associated clusters
 * @param data - the CUDA performance data object for the clustering criteria
 *
 * Create a new background graph renderer backend, which when signalled, will process the CUDA events maintained in the performance data object and
 * emit signals to be handled by this class to add CUDA event information to the QCustomPlot for the associated cluster.  These signal connections
 * are setup by this method.
 */
void BackgroundGraphRenderer::setPerformanceData(const QString& clusteringCriteriaName, const QVector< QString >& clusterNames, const CUDA::PerformanceData& data)
{
    BackgroundGraphRendererBackend* backend = new BackgroundGraphRendererBackend( clusteringCriteriaName, data );

    if ( backend ) {

        // emit signal to create QCustomPlot for each cluster
        // NOTE: signal will be handled in the GUI thread were the QCustomPlot instance needs to live
        foreach(const QString& clusterName, clusterNames) {
            if ( ! clusterName.contains( "(GPU)" ) ) {
                emit createPlotForClustering( clusteringCriteriaName, clusterName );
            }
        }

        // set backend object name to clustering criteria name (so it can be identified in timer handlers) and move to backend thread
        backend->setObjectName( clusteringCriteriaName );
        backend->moveToThread( &m_thread );

        // setup signal-to-signal and signal-to-slot connectionsthread
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( this, &BackgroundGraphRenderer::signalProcessCudaEventView, backend, &BackgroundGraphRendererBackend::signalProcessCudaEventViewStart );
        connect( backend, &BackgroundGraphRendererBackend::addDataTransfer, this, &BackgroundGraphRenderer::processDataTransferEvent, Qt::QueuedConnection );
        connect( backend, &BackgroundGraphRendererBackend::addKernelExecution, this, &BackgroundGraphRenderer::processKernelExecutionEvent, Qt::QueuedConnection );
        connect( backend, &BackgroundGraphRendererBackend::signalProcessCudaEventViewDone, this, &BackgroundGraphRenderer::handleProcessCudaEventViewDone, Qt::QueuedConnection );
#else
        connect( this, SIGNAL(signalProcessCudaEventView()), backend, SIGNAL(signalProcessCudaEventViewStart()) );
        connect( backend, SIGNAL(addDataTransfer(QString,Base::Time,CUDA::DataTransfer)), this, SLOT(processDataTransferEvent(QString,Base::Time,CUDA::DataTransfer)), Qt::QueuedConnection );
        connect( backend, SIGNAL(addKernelExecution(QString,Base::Time,CUDA::KernelExecution)), this, SLOT(processKernelExecutionEvent(QString,Base::Time,CUDA::KernelExecution)), Qt::QueuedConnection );
        connect( backend, SIGNAL(signalProcessCudaEventViewDone()), this, SLOT(handleProcessCudaEventViewDone()), Qt::QueuedConnection );
#endif

        // insert backend instance into the backend map
        m_backend.insert( clusteringCriteriaName, backend );
    }
}

/**
 * @brief BackgroundGraphRenderer::unloadCudaVie
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @param clusterNames - the list of associated clusters
 *
 * Delete the QCustomPlot instances associated with the provided list of clusters and remove from plot map.
 */
void BackgroundGraphRenderer::unloadCudaViews(const QString &clusteringCriteriaName, const QStringList &clusterNames)
{
    Q_UNUSED(clusteringCriteriaName)
    QMutableMapIterator< QString, CustomPlot* > piter( m_plot );
    while ( piter.hasNext() ) {
        piter.next();
        if ( clusterNames.contains( piter.key() ) ) {
            QCustomPlot* plot( piter.value() );
            delete plot;
            piter.remove();
        }
    }
}

/**
 * @brief BackgroundGraphRenderer::handleGraphRangeChanged
 * @param clusterName - the cluster group name
 * @param lower - the new X-axis lower range
 * @param upper - the new X-axis upper range
 * @param size - the size of the plot axis rectangle
 *
 * This method handles graph range changed events so that the processing of the CUDA events for the new view can be initiated after a waiting period.
 * The waiting period is handled by a timer run in a thread and allows processing only if the user has stopped manipulating the graph view (zoom and pan).
 */
void BackgroundGraphRenderer::handleGraphRangeChanged(const QString& clusterName, double lower, double upper, const QSize& size)
{
    m_userChangeMgr.cancel( clusterName );

    if ( ! m_plot.contains( clusterName ) )
        return;

#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "BackgroundGraphRenderer::handleGraphRangeChanged: clusterName=" << clusterName << "lower=" << lower << "upper=" << upper;
#endif

    QCustomPlot* plot( m_plot.value( clusterName ) );
    if ( plot ) {
        QCPAxisRect* axisRect = plot->axisRect();
        if ( axisRect ) {
            QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
            if ( xAxis ) {
                m_userChangeMgr.create( clusterName, lower, upper, size );
            }
        }
    }
}

/**
 * @brief BackgroundGraphRenderer::handleGraphRangeChangedTimeout
 * @param clusterName - the cluster group name
 * @param lower - the new X-axis lower range
 * @param upper - the new X-axis upper range
 * @param size - the size of the plot axis rectangle
 *
 * This handler in invoked when the waiting period has benn reached and actual processing of the CUDA events can proceed.
 */
void BackgroundGraphRenderer::handleGraphRangeChangedTimeout(const QString& clusterName, double lower, double upper, const QSize& size)
{
    if ( ! m_plot.contains( clusterName ) )
        return;

    CustomPlot* plot( m_plot.value( clusterName ) );
    if ( plot ) {
        QCPAxisRect* axisRect = plot->axisRect();
        if ( axisRect ) {
            QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
            if ( xAxis ) {
                xAxis->setRange( lower, upper );
                plot->setProperty( "imageWidth", size.width() );
                plot->setProperty( "imageHeight", size.height() );
                plot->replot();
            }
        }
    }
}

/**
 * @brief BackgroundGraphRenderer::processDataTransferEvent
 * @param clusteringName - the cluster group name
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the data transfer event
 *
 * Create the data transfer graph item from the details using the associated axis rect.
 * Add graph item to QCustomPlot instance.
 */
void BackgroundGraphRenderer::processDataTransferEvent(const QString& clusteringName,
                                                       const Base::Time &time_origin,
                                                       const CUDA::DataTransfer &details)
{
    if ( ! m_plot.contains( clusteringName ) )
        return;

    QCustomPlot* plot = m_plot.value( clusteringName );

    if ( Q_NULLPTR == plot )
        return;

    OSSDataTransferItem* dataXferItem = new OSSDataTransferItem( plot->axisRect(), plot );

    if ( ! dataXferItem )
        return;

    dataXferItem->setData( time_origin, details );

    plot->addItem( dataXferItem );

#ifdef HAS_PROCESS_EVENT_DEBUG
    QString line;
    QTextStream output(&line);
    output << "Data Transfer: " << *dataXferItem;
    qDebug() << line;
#endif
}

/**
 * @brief BackgroundGraphRenderer::processKernelExecutionEvent
 * @param clusteringName - the cluster group name
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the kernel execution event
 *
 * Find the axis rect associated with the specified metric group and metric name.  Create the kernel execution graph item from the details using the associated axis rect.
 * Add graph item to QCustomPlot instance.
 */
void BackgroundGraphRenderer::processKernelExecutionEvent(const QString& clusteringName,
                                                          const Base::Time &time_origin,
                                                          const CUDA::KernelExecution &details)
{
    if ( ! m_plot.contains( clusteringName ) )
        return;

    QCustomPlot* plot = m_plot.value( clusteringName );

    if ( Q_NULLPTR == plot )
        return;

    OSSKernelExecutionItem* kernelExecItem = new OSSKernelExecutionItem( plot->axisRect(), plot );

    if ( ! kernelExecItem )
        return;

    kernelExecItem->setData( time_origin, details );

    plot->addItem( kernelExecItem );

#ifdef HAS_PROCESS_EVENT_DEBUG
    QString line;
    QTextStream output(&line);
    output << "Kernel Execution: " << *kernelExecItem;
    qDebug() << line;
#endif
}

/**
 * @brief BackgroundGraphRenderer::handleProcessCudaEventViewDone
 *
 * This signal handler is invoked by the backend when the CUDA event processing concludes and
 * a new image representing the CUDA event plot can be generated and send to the plot view
 * for insertion in the QCustomPlot for the associated clustering criteria / clustering group.
 */
void BackgroundGraphRenderer::handleProcessCudaEventViewDone()
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "BackgroundGraphRenderer::handleProcessCudaEventViewDone: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif
    BackgroundGraphRendererBackend* backend = qobject_cast< BackgroundGraphRendererBackend* >( sender() );

    if ( backend ) {
        // get the associated clustering criteria name
        QString clusteringCriteriaName( backend->objectName() );

        QMap< QString, CustomPlot* >::iterator iter( m_plot.begin() );
        while ( iter != m_plot.end() ) {
            CustomPlot* plot( iter.value() );
            if ( plot ) {
#ifdef HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE
                QtConcurrent::run( this, &BackgroundGraphRenderer::processCudaEventSnapshot, plot );
#else
                processCudaEventSnapshot( plot );
#endif
            }
            iter++;
        }

        // remove the backend from the map and schedule for deletion
        m_backend.remove( clusteringCriteriaName );

        backend->deleteLater();
    }
}

/**
 * @brief BackgroundGraphRenderer::processCudaEventSnapshots
 *
 * Handle QCustomPlot afterReplot() signal.  Determine QCustomPlot emitting signal and further process in a different thread.
 */
void BackgroundGraphRenderer::processCudaEventSnapshot()
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "BackgroundGraphRenderer::processCudaEventSnapshot: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif
    CustomPlot* plot = qobject_cast< CustomPlot* >( sender() );

    if ( plot ) {
#ifdef HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE
        QtConcurrent::run( this, &BackgroundGraphRenderer::processCudaEventSnapshot, plot );
#else
        processCudaEventSnapshot( plot );
#endif
    }
}

/**
 * @brief BackgroundGraphRenderer::processCudaEventSnapshot
 * @param plot - the plot to be rendered to image
 *
 * Renders plot to image and emits signal to provide image to consumers.
 */
void BackgroundGraphRenderer::processCudaEventSnapshot(CustomPlot* plot)
{
#ifdef HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
    qDebug() << "BackgroundGraphRenderer::processCudaEventSnapshot: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif
    // get the associated clustering criteria / cluster name
    QString clusteringName( plot->property( "clusteringName" ).toString() );
    QString clusteringCriteriaName( plot->property( "clusteringCriteriaName" ).toString() );
    int width( plot->property( "imageWidth" ).toInt() );
    int height( plot->property( "imageHeight" ).toInt() );
    QCPRange range = plot->axisRect()->axis( QCPAxis::atBottom )->range();

    if ( 0 != width && 0 != height && range.lower != range.upper ) {
        // setup image and QCPPainter instance rendering to the image
        QImage image( width, height, QImage::Format_ARGB32 );
        image.fill( Qt::transparent );
        QCPPainter* painter = new QCPPainter( &image );
        if ( painter ) {
            plot->setBackground( Qt::transparent );
            // start the rendering to the image
            plot->toPainter( painter, width, height );

            // generate a cropped image with just the CUDA event portion
            QImage croppedImage = image.copy( 0, height * 0.45 + 1, width, height * 0.10 );

            // signal the new CUDA event snapshot
            emit signalCudaEventSnapshot( clusteringCriteriaName, clusteringName, range.lower, range.upper, croppedImage );

            // cleanup painter instance
            delete painter;
        }
        else {
            qWarning() << "Not able to allocate QCPPainter";
        }
    }
}

/**
 * @brief BackgroundGraphRenderer::handleCreatePlotForClustering
 * @param clusteringName - the cluster group name
 *
 * This handler creates a new QCustomPlot in the GUI thread to be used for background (non-display) rendering of CUDA events.
 */
void BackgroundGraphRenderer::handleCreatePlotForClustering(const QString& clusteringCriteriaName, const QString &clusteringName)
{
    CustomPlot* plot = new CustomPlot;
    if ( plot ) {
        plot->setProperty( "clusteringCriteriaName", clusteringCriteriaName );
        plot->setProperty( "clusteringName", clusteringName );
        connect( plot, SIGNAL(afterReplot()), this, SLOT(processCudaEventSnapshot()) );
        QCPAxisRect* axisRect = plot->axisRect();
        if ( axisRect ) {
            axisRect->setAutoMargins( QCP::msNone );
            axisRect->setMargins( QMargins(0,0,0,0) );
            QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
            QCPAxis* yAxis = axisRect->axis( QCPAxis::atLeft );
            if ( xAxis ) xAxis->setVisible( false );
            if ( yAxis ) yAxis->setVisible( false );
        }
        m_plot.insert( clusteringName, plot );
    }
}


} // GUI
} // ArgoNavis

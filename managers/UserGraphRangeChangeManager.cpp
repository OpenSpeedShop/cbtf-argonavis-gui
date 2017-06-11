/*!
   \file UserGraphRangeChangeManager.cpp
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

#include "UserGraphRangeChangeManager.h"

#include <QThread>
#include <QTimer>
#include <QVariant>
#include <QDebug>

#define GRAPH_RANGE_CHANGE_DELAY_TO_CUDA_EVENT_PROCESSING 800


namespace ArgoNavis { namespace GUI {


/**
 * @brief UserGraphRangeChangeManager::UserGraphRangeChangeManager
 * @param parent - the parent QObject instance
 *
 * Constructs a UserGraphRangeChangeManager instance
 */
UserGraphRangeChangeManager::UserGraphRangeChangeManager(QObject *parent)
    : QObject( parent )
{
    m_thread.start();
}

/**
 * @brief UserGraphRangeChangeManager::~UserGraphRangeChangeManager
 *
 * Destroys the UserGraphRangeChangeManager instance
 */
UserGraphRangeChangeManager::~UserGraphRangeChangeManager()
{
    m_thread.quit();
    m_thread.wait();
}

/**
 * @brief UserGraphRangeChangeManager::create
 * @param clusteringCriteriaName - the name of the cluster criteria
 * @param clusterName - the name of the cluster (thread/process identifier)
 * @param lower - the lower value of the graph range change
 * @param upper - the upper value of the graph range change
 * @param size - the size of the axis rect
 *
 * This method creates and starts a timer to delay processing of the graph range change until the timeout threshold has
 * been reached without further user interaction to change the graph range.
 */
void UserGraphRangeChangeManager::create(const QString &clusteringCriteriaName, const QString &clusterName, double lower, double upper, const QSize &size)
{
    QTimer* timer = new QTimer;
    if ( timer ) {
        // timer is single-shot expiring in 500ms
        timer->setSingleShot( true );
        timer->setInterval( GRAPH_RANGE_CHANGE_DELAY_TO_CUDA_EVENT_PROCESSING );
        // add timer to timer map
        {
            QMutexLocker guard( &m_mutex );
            m_timers.insert( clusterName, timer );
        }
        // move timer to thread to let signals manage timer start/stop state
        timer->moveToThread( &m_thread );
        // setup the timer expiry handler to process the graph range change only if the waiting period completes
        timer->setProperty( "clusteringCriteriaName", clusteringCriteriaName );
        timer->setProperty( "clusterName", clusterName );
        timer->setProperty( "lower", lower );
        timer->setProperty( "upper", upper );
        timer->setProperty( "size", size );
        // connect timer timeout signal to handler
        connect( timer, SIGNAL(timeout()), this, SLOT(handleTimeout()) );
#ifdef HAS_TIMER_THREAD_DESTROYED_CHECKING
        connect( timer, SIGNAL(destroyed(QObject*)), this, SLOT(timerDestroyed(QObject*)) );
#endif
        // start the timer
        QMetaObject::invokeMethod( timer, "start", Qt::QueuedConnection );
    }
    else {
        qWarning() << "Not able to allocate timer";
    }
}

/**
 * @brief UserGraphRangeChangeManager::cancel
 * @param clusterName - the name of the cluster (thread/process identifier)
 *
 * This method cancels the timer associated with the cluster name.
 */
void UserGraphRangeChangeManager::cancel(const QString &clusterName)
{
    QMutexLocker guard( &m_mutex );
    if ( m_timers.contains( clusterName ) ) {
        QTimer* timer = m_timers.take( clusterName );
        if ( timer ) {
            QMetaObject::invokeMethod( timer, "stop", Qt::QueuedConnection );
            timer->deleteLater();
        }
    }
}

/**
 * @brief UserGraphRangeChangeManager::handleTimeout
 *
 * This is the handler for the QTimer::timeout signal for any timer created and started by the UserGraphRangeChangeManager::create method.
 */
void UserGraphRangeChangeManager::handleTimeout()
{
    QTimer* timer = qobject_cast< QTimer* >( sender() );

    if ( ! timer )
        return;

    QString clusteringCriteriaName = timer->property( "clusteringCriteriaName" ).toString();
    QString clusterName = timer->property( "clusterName" ).toString();

    double lower = timer->property( "lower" ).toDouble();
    double upper = timer->property( "upper" ).toDouble();
    QSize size = timer->property( "size" ).toSize();

    qDebug() << "UserGraphRangeChangeManager::handleTimeout: clusterName=" << clusterName << " lower=" << lower << " upper=" << upper;

    emit timeout( clusteringCriteriaName, clusterName, lower, upper, size );

    cancel( clusterName );
}

#ifdef HAS_TIMER_THREAD_DESTROYED_CHECKING
/**
 * @brief UserGraphRangeChangeManager::timerDestroyed
 * @param obj - the QObject instance destroyed
 *
 * This is the handler for the QTimer::destroyed signal used for debugging purposes.
 */
void UserGraphRangeChangeManager::timerDestroyed(QObject* obj)
{
    Q_UNUSED(obj);
    qDebug() << "TIMER DESTROYED!!";
}
#endif


} // GUI
} // ArgoNavis

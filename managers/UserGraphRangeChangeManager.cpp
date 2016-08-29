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

#define GRAPH_RANGE_CHANGE_DELAY_TO_CUDA_EVENT_PROCESSING 500


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

}

/**
 * @brief UserGraphRangeChangeManager::create
 * @param clusterName
 * @param lower
 * @param upper
 * @param size
 */
void UserGraphRangeChangeManager::create(const QString &clusterName, double lower, double upper, const QSize &size)
{
    QThread* thread = new QThread;
    if ( thread ) {
        QTimer* timer = new QTimer;
        if ( timer ) {
            // timer is single-shot expiring in 500ms
            timer->setSingleShot( true );
            timer->setInterval( GRAPH_RANGE_CHANGE_DELAY_TO_CUDA_EVENT_PROCESSING );
            {
                QMutexLocker guard( &m_mutex );
                m_timerThreads[ clusterName ] = thread;
#if (QT_VERSION < QT_VERSION_CHECK(4, 8, 0))
                QUuid uuid = QUuid::createUuid();
                thread->setProperty( "timerId", uuid.toString() );
                m_timers.insert( uuid, timer );
#endif
            }
            // move timer to thread to let signals manage timer start/stop state
            timer->moveToThread( thread );
            // setup the timer expiry handler to process the graph range change only if the waiting period completes
            timer->setProperty( "clusterName", clusterName );
            timer->setProperty( "lower", lower );
            timer->setProperty( "upper", upper );
            timer->setProperty( "size", size );
            // connect timer timeout signal to handler
            connect( timer, SIGNAL(timeout()), this, SLOT(handleTimeout()) );
            // start the timer when the thread is started
            connect( thread, SIGNAL(started()), timer, SLOT(start()) );
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
            // stop the timer when the thread finishes
            connect( thread, SIGNAL(finished()), timer, SLOT(stop()) );
            // when the thread finishes schedule the timer instance for deletion
            connect( thread, SIGNAL(finished()), timer, SLOT(deleteLater()) );
#endif
            // when the thread finishes schedule the timer thread instance for deletion
            connect( thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );
#ifdef HAS_TIMER_THREAD_DESTROYED_CHECKING
            connect( thread, SIGNAL(destroyed(QObject*)), this, SLOT(threadDestroyed(QObject*)) );
            connect( timer, SIGNAL(destroyed(QObject*)), this, SLOT(timerDestroyed(QObject*)) );
#endif

            // start the thread (and the timer per previously setup signal-to-slot connection)
            thread->setObjectName( clusterName );
            thread->start();
        }
        else {
            qWarning() << "Not able to allocate timer";
            delete thread;
        }
    }
    else {
        qWarning() << "Not able to allocate thread";
    }
}

/**
 * @brief UserGraphRangeChangeManager::cancel
 * @param clusterName
 */
void UserGraphRangeChangeManager::cancel(const QString &clusterName)
{
    QMutexLocker guard( &m_mutex );
    if ( m_timerThreads.contains( clusterName ) ) {
        QThread* thread = m_timerThreads.take( clusterName );
#if (QT_VERSION < QT_VERSION_CHECK(4, 8, 0))
        QString uuid = thread->property( "timerId" ).toString();
        if ( m_timers.contains( uuid ) ) {
            QTimer* timer = m_timers.take( uuid );
            if ( timer ) {
                timer->stop();
                timer->deleteLater();
            }
        }
#endif
        thread->quit();
    }
}

/**
 * @brief UserGraphRangeChangeManager::handleTimeout
 */
void UserGraphRangeChangeManager::handleTimeout()
{
    QTimer* timer = qobject_cast< QTimer* >( sender() );

    if ( ! timer )
        return;

    QString clusterName = timer->property( "clusterName" ).toString();

    double lower = timer->property( "lower" ).toDouble();
    double upper = timer->property( "upper" ).toDouble();
    QSize size = timer->property( "size" ).toSize();

    emit timeout( clusterName, lower, upper, size );

    cancel( clusterName );
}


} // GUI
} // ArgoNavis

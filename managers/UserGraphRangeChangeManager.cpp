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
 * @param group - the name of the group the item being tracked belongs to
 * @param item - the name of the item being tracked
 * @param lower - the lower value of the graph range change
 * @param upper - the upper value of the graph range change
 * @param size - the size of the axis rect
 *
 * This method creates and starts a timer to delay processing of the graph range change until the timeout threshold has
 * been reached without further user interaction to change the graph range.
 */
void UserGraphRangeChangeManager::create(const QString &group, const QString &item, double lower, double upper, const QSize &size)
{
    QTimer* timer = new QTimer;
    if ( timer ) {
        // timer is single-shot expiring in 500ms
        timer->setSingleShot( true );
        timer->setInterval( GRAPH_RANGE_CHANGE_DELAY_TO_CUDA_EVENT_PROCESSING );
        // add timer to timer map
        {
            QMutexLocker guard( &m_mutex );
            m_timers.insert( item, timer );
            m_activeMap[ group ].insert( item );
        }
        // move timer to thread to let signals manage timer start/stop state
        timer->moveToThread( &m_thread );
        // setup the timer expiry handler to process the graph range change only if the waiting period completes
        timer->setProperty( "group", group );
        timer->setProperty( "item", item );
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
 * @param item - the name of the item being tracked
 *
 * This method cancels the timer associated with the named item.
 */
void UserGraphRangeChangeManager::cancel(const QString &item)
{
    QMutexLocker guard( &m_mutex );
    if ( m_timers.contains( item ) ) {
        QTimer* timer = m_timers.take( item );
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

    QString group = timer->property( "group" ).toString();
    QString item = timer->property( "item" ).toString();

    double lower = timer->property( "lower" ).toDouble();
    double upper = timer->property( "upper" ).toDouble();
    QSize size = timer->property( "size" ).toSize();

    qDebug() << "UserGraphRangeChangeManager::handleTimeout: item=" << item << " lower=" << lower << " upper=" << upper;

    emit timeout( group, item, lower, upper, size );

    cancel( item );

    QMutexLocker guard( &m_mutex );

    if ( m_activeMap.contains( group ) ) {
        QSet< QString>& active = m_activeMap[ group ];
        if ( active.remove( item ) && active.isEmpty() ) {
            emit timeoutGroup( group, lower, upper, size );
        }
    }
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
    Q_UNUSED( obj )
    qDebug() << "TIMER DESTROYED!!";
}
#endif


} // GUI
} // ArgoNavis

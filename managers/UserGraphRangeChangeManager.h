/*!
   \file UserGraphRangeChangeManager.h
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

#ifndef USERGRAPHRANGECHANGEMANAGER_H
#define USERGRAPHRANGECHANGEMANAGER_H

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QSize>
#include <QString>
#include <QThread>

class QTimer;


#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


class UserGraphRangeChangeManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(UserGraphRangeChangeManager)

public:

    explicit UserGraphRangeChangeManager(QObject *parent = 0);
    virtual ~UserGraphRangeChangeManager();

    void create(const QString& clusterName, double lower, double upper, const QSize& size = QSize() );

    void cancel(const QString& clusterName);

signals:

    void timeout(const QString& clusterName, double lower, double upper, const QSize& size);

private slots:

    void handleTimeout();

#ifdef HAS_TIMER_THREAD_DESTROYED_CHECKING
    void timerDestroyed(QObject* obj = Q_NULLPTR);
#endif

private:

    QMutex m_mutex;
    QThread m_thread;
    QMap< QString, QTimer* > m_timers;

};


} // GUI
} // ArgoNavis

#endif // USERGRAPHRANGECHANGEMANAGER_H
/*!
   \file LoadExperimentTaskWatcher.h
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

#ifndef LOADEXPERIMENTTASKWATCHER_H
#define LOADEXPERIMENTTASKWATCHER_H

#include <QObject>

#include <QtConcurrent>

#include "PerformanceDataManager.h"


namespace ArgoNavis { namespace GUI {


/*!
 * \brief The LoadExperimentTaskWatcher class
 *
 * This class provides a wrapper for the QFuture/QFutureWatcher handling for the method run currently using QtConcurrent::run().
 */

class LoadExperimentTaskWatcher : public QObject
{
    Q_OBJECT

public:

    explicit LoadExperimentTaskWatcher(PerformanceDataManager* manager, QObject *parent = 0);

    void run(const QString& filename);

signals:

    void finished();

private:

    QFuture<void> m_future;
    QFutureWatcher<void> m_watcher;

    PerformanceDataManager* m_manager;

};


} // GUI
} // ArgoNavis

#endif // LOADEXPERIMENTTASKWATCHER_H

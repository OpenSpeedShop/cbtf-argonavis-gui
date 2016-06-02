/*!
   \file LoadExperimentTaskWatcher.cpp
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

#include "LoadExperimentTaskWatcher.h"


namespace ArgoNavis { namespace GUI {


/**
 * @brief LoadExperimentTaskWatcher::LoadExperimentTaskWatcher
 * @param - parent - the parent widget
 *
 * Constructs a LoadExperimentTaskWatcher instance of the given parent.
 */
LoadExperimentTaskWatcher::LoadExperimentTaskWatcher(PerformanceDataManager* manager, QObject *parent)
    : QObject( parent )
    , m_manager( manager )
{

}

/**
 * @brief LoadExperimentTaskWatcher::run
 * @param filename - Filename path to experiment database file with CUDA data collection (.openss file)
 */
void LoadExperimentTaskWatcher::run(const QString &filename)
{
    connect( &m_watcher, &QFutureWatcher<void>::finished, this, &LoadExperimentTaskWatcher::finished );

    m_future = QtConcurrent::run( m_manager, &PerformanceDataManager::loadCudaView, filename );

    m_watcher.setFuture( m_future );
}


} // GUI
} // ArgoNavis

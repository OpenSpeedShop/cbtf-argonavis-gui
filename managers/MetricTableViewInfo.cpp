/*!
   \file MetricTableViewInfo.cpp
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2017 Argo Navis Technologies, LLC

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

#include "MetricTableViewInfo.h"

namespace ArgoNavis { namespace GUI {

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
const QStringList TRACED_MEMORY_FUNCTIONS = {
    QStringLiteral( "malloc" ),
    QStringLiteral( "free" ),
    QStringLiteral( "memalign" ),
    QStringLiteral( "posix_memalign" ),
    QStringLiteral( "calloc" ),
    QStringLiteral( "realloc" )
};
#else
const QStringList TRACED_MEMORY_FUNCTIONS = QStringList() << "malloc" << "free" << "memalign"
                                                          << "posix_memalign" << "calloc" << "realloc";
#endif

/**
 * @brief MetricTableViewInfo::MetricTableViewInfo
 * @param experiment - pointer to associated experiment
 * @param interval - the current time interval
 * @param metricViewList - the metric view list
 * @note class instance does not take ownership of the experiment object pointer
 *
 * Constructs a MetricTableViewInfo instance.
 */
MetricTableViewInfo::MetricTableViewInfo(const OpenSpeedShop::Framework::Experiment* experiment, OpenSpeedShop::Framework::TimeInterval interval, QStringList metricViewList)
    : m_experiment( experiment ), m_interval( interval ), m_metricViewList ( metricViewList )
{

}

/**
 * @brief MetricTableViewInfo::~MetricTableViewInfo
 *
 * Destroys the MetricTableViewInfo instance
 */
MetricTableViewInfo::~MetricTableViewInfo()
{

}

/**
 * @brief MetricTableViewInfo::MetricTableViewInfo
 *
 * Default MetricTableViewInfo constructor
 */
MetricTableViewInfo::MetricTableViewInfo()
    : m_experiment( NULL )
{

}

/**
 * @brief MetricTableViewInfo::MetricTableViewInfo
 * @param other - reference to object to copy
 *
 * Copy constructor
 */
MetricTableViewInfo::MetricTableViewInfo(const MetricTableViewInfo& other)
{
    operator=(other);
}

/**
 * @brief MetricTableViewInfo::add
 * @param metricViewName - new metric view to add to list
 *
 * This method adds the metric view to the metric view list if not already present.
 */
void MetricTableViewInfo::addMetricView(const QString& name)
{
    QMutexLocker guard( &m_mutex );
    if ( ! m_metricViewList.contains( name ) )
        m_metricViewList << name;
}

/**
 * @brief MetricTableViewInfo::getCollectors
 * @return - the set of collectors from the experiment
 *
 * This function returns the set of collectors from the experiment.
 */
OpenSpeedShop::Framework::CollectorGroup MetricTableViewInfo::getCollectors()
{
    QMutexLocker guard( &m_mutex );
    return m_experiment->getCollectors();
}

/**
 * @brief MetricTableViewInfo::getInterval
 * @return - the current time interval for the metric view
 *
 * This function returns the current time interval for the metric view.
 */
OpenSpeedShop::Framework::TimeInterval MetricTableViewInfo::getInterval()
{
    QMutexLocker guard( &m_mutex );
    return m_interval;
}

/**
 * @brief MetricTableViewInfo::getExtent
 * @return - the extent of the experiment
 *
 * This function returns the extent of the experiment.
 */
OpenSpeedShop::Framework::Extent MetricTableViewInfo::getExtent()
{
    QMutexLocker guard( &m_mutex );
    return m_experiment->getPerformanceDataExtent();
}

/**
 * @brief MetricTableViewInfo::getThreads
 * @return  - the set of threads in the experiment
 *
 * This function returns the set of threads in the experiment.
 */
OpenSpeedShop::Framework::ThreadGroup MetricTableViewInfo::getThreads()
{
    QMutexLocker guard( &m_mutex );
    return m_experiment->getThreads();
}

/**
 * @brief MetricTableViewInfo::searchForFunctions
 * @param name - input function name
 * @return - whether input function name is a traced memory function
 *
 * This function returns whether the input function name is a traced memory function.
 */
bool MetricTableViewInfo::isTracedMemoryFunction(const QString& name)
{
    bool result( false );

    foreach( const QString& traceableFunction, TRACED_MEMORY_FUNCTIONS ) {
        if ( name.contains( traceableFunction ) ) {
            result = true;
            break;
        }
    }

    return result;
}

/**
 * @brief MetricTableViewInfo::setInterval
 * @param lower - the new lower value of the time interval
 * @param upper - the new upper value of the time interval
 *
 * This method updates the current time interval for the metric view.
 */
void MetricTableViewInfo::setInterval(const OpenSpeedShop::Framework::Time& lower, const OpenSpeedShop::Framework::Time& upper)
{
    QMutexLocker guard( &m_mutex );
    m_interval = OpenSpeedShop::Framework::TimeInterval( lower, upper );
}

/**
 * @brief MetricTableViewInfo::getMetricViewList
 * @return - the list of metric views
 *
 * This method returns the list of metric views.
 */
QStringList MetricTableViewInfo::getMetricViewList()
{
    QMutexLocker guard( &m_mutex );
    return m_metricViewList;
}

/**
 * @brief MetricTableViewInfo::experiment
 * @return - the experiment instance
 *
 * Get experiment instance.
 */
const OpenSpeedShop::Framework::Experiment *MetricTableViewInfo::experiment()
{
    QMutexLocker guard( &m_mutex );
    return m_experiment;
}


} // GUI
} // ArgoNavis

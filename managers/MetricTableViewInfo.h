/*!
   \file MetricTableViewInfo.h
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

#ifndef METRICTABLEVIEWINFO_H
#define METRICTABLEVIEWINFO_H

#include "TimeInterval.hxx"
#include "CollectorGroup.hxx"
#include "Experiment.hxx"
#include "ThreadGroup.hxx"

#include <QString>
#include <QStringList>
#include <QMutex>

namespace ArgoNavis { namespace GUI {

class MetricTableViewInfo
{
public:

    explicit MetricTableViewInfo(const OpenSpeedShop::Framework::Experiment* experiment, OpenSpeedShop::Framework::TimeInterval interval, QStringList metricViewList);
    explicit MetricTableViewInfo();
    virtual ~MetricTableViewInfo();

    MetricTableViewInfo& operator=(const MetricTableViewInfo& other)
    {
        m_metricViewList = other.m_metricViewList;
        m_experiment = other.m_experiment;
        m_interval = other.m_interval;
        return *this;
    }

    MetricTableViewInfo(const MetricTableViewInfo& other);

    void addMetricView(const QString& name);
    OpenSpeedShop::Framework::CollectorGroup getCollectors();
    OpenSpeedShop::Framework::TimeInterval getInterval();
    OpenSpeedShop::Framework::Extent getExtent();
    OpenSpeedShop::Framework::ThreadGroup getThreads();
    void setInterval(const OpenSpeedShop::Framework::Time& lower, const OpenSpeedShop::Framework::Time& upper);
    QStringList getMetricViewList();

private:

    const OpenSpeedShop::Framework::Experiment* m_experiment;
    OpenSpeedShop::Framework::TimeInterval m_interval;
    QStringList m_metricViewList;           // [ <metric name> | "Details" ] - [ <View Name> ]
    QMutex m_mutex;

};

} // GUI
} // ArgoNavis


#endif // METRICTABLEVIEWINFO_H

/*!
   \file SourceViewMetricsCache.h
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

#ifndef SOURCEVIEWMETRICSCACHE_H
#define SOURCEVIEWMETRICSCACHE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QPair>
#include <QVector>
#include <QVariantList>
#include <QMutex>

namespace ArgoNavis { namespace GUI {


class SourceViewMetricsCache : public QObject
{
    Q_OBJECT

public:

    explicit SourceViewMetricsCache(QObject *parent = 0);
    virtual ~SourceViewMetricsCache();

    QMap< QString, QVector< double > > getMetricsCache(const QString& metricViewName);

public slots:

    void handleAddMetricView(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, const QStringList &metrics);
    void handleAddMetricViewData(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, const QVariantList &data, const QStringList &columnHeaders);

protected:

    void clear();

private:

    // maps metric view name to pair of integers representing the indexes of
    // the columns containing the defining location and the metric value.
    QMap< QString, QPair<int, int> > m_watchedMetricViews;

    // maps the metric view name to another map of filename to the metric value
    QMap< QString, QMap< QString, QVector<double> > > m_metrics;

    // mutex for cache
    QMutex m_mutex;
};


} // GUI
} // ArgoNavis

#endif // SOURCEVIEWMETRICSCACHE_H

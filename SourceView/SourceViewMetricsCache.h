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
#include <set>

namespace ArgoNavis { namespace GUI {


class SourceViewMetricsCache : public QObject
{
    Q_OBJECT

public:

    explicit SourceViewMetricsCache(QObject *parent = 0);
    virtual ~SourceViewMetricsCache();

    QVector< double > getMetricsCache(const QString& metricViewName, const QString& currentFileName);

    QStringList getMetricChoices(const QString &metricViewName) const;

    void getSelectedMetricDetails(const QString& metricViewName, QString& name, QVariant::Type& type) const;

    void clear();

signals:

    void signalSelectedMetricChanged(const QString& metricViewName, const QString& selectedMetricName);

public slots:

    void handleSelectedMetricChanged();
    void handleAddMetricView(const QString &clusteringCriteriaName, const QString& modeName, const QString &metricName, const QString &viewName, const QStringList &metrics);
    void handleAddMetricViewData(const QString &clusteringCriteriaName, const QString& modeName, const QString &metricName, const QString &viewName, const QVariantList &data, const QStringList &columnHeaders);

private:

    // maps metric view name to pair of integers representing the indexes of
    // the columns containing the defining location and the metric value.
    QMap< QString, QMap< QString, int > > m_watchedMetricViews;

    // maps the metric view name to another map of filename to another map of the metric name to the metric value
    QMap< QString, QMap< QString, QMap< QString, QVector<double> > > > m_metrics;

    // maps the metric view name to the name of the metric in the 'm_watchedMetricViews' that is selected
    QMap< QString, QString > m_watchedMetricNames;

    // maps the metric view name to the set of metrics that can be selected for the metric view
    QMap< QString, std::set< QString > > m_watchableMetricNames;

    // mutex for cache
    mutable QMutex m_mutex;

};


} // GUI
} // ArgoNavis

#endif // SOURCEVIEWMETRICSCACHE_H

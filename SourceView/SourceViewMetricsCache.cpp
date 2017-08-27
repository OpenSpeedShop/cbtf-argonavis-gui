/*!
   \file SourceViewMetricsCache.cpp
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

#include "SourceViewMetricsCache.h"

#include "ModifyPathSubstitutionsDialog.h"

#include <QVariant>


namespace ArgoNavis { namespace GUI {


const QString s_timeTitle = QStringLiteral("Time (msec)");
const QString s_functionTitle = QStringLiteral("Function (defining location)");


/**
 * @brief SourceViewMetricsCache::SourceViewMetricsCache
 * @param parent - the parent QObject instance
 *
 * Constructs a SourceViewMetricsCache instance
 */
SourceViewMetricsCache::SourceViewMetricsCache(QObject *parent)
    : QObject( parent )
{

}

/**
 * @brief SourceViewMetricsCache::~SourceViewMetricsCache
 *
 * Destroys the SourceViewMetricsCache instance.
 */
SourceViewMetricsCache::~SourceViewMetricsCache()
{
    clear();
}

/**
 * @brief SourceViewMetricsCache::getMetricsCache
 * @param metricViewName - the metric view name
 * @return the map for the specified metric view
 *
 * Get a copy of the metric cache for the specified metric view.
 */
QMap<QString, QVector<double> > SourceViewMetricsCache::getMetricsCache(const QString &metricViewName)
{
    QMutexLocker guard( &m_mutex );

    return m_metrics[ metricViewName ];
}

/**
 * @brief SourceViewMetricsCache::handleAddMetricView
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 * @param metrics - list of metrics for setting column headers
 *
 * Extracts the column numbers for the defining location and metric value for the specified metric view
 * and adds an entry in the watched metric views map.
 */
void SourceViewMetricsCache::handleAddMetricView(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, const QStringList &metrics)
{
    Q_UNUSED( clusteringCriteriaName );

    if ( metrics.contains( s_timeTitle ) && metrics.contains( s_functionTitle ) ) {
        const QString metricViewname = metricName + "-" + viewName;

        const int timeTitleIdx = metrics.indexOf( s_timeTitle );
        const int functionTitleIdx = metrics.indexOf( s_functionTitle );

        QMutexLocker guard( &m_mutex );

        m_watchedMetricViews.insert( metricViewname, qMakePair( timeTitleIdx, functionTitleIdx ) );
    }
}

/**
 * @brief SourceViewMetricsCache::handleAddMetricViewData
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 * @param data - the data to add to the model
 * @param columnHeaders - if present provides the names of the columns for each index in the data
 *
 * Extracts the data for one entry of the specified metric view and stores in the corresponding cache map .
 */
void SourceViewMetricsCache::handleAddMetricViewData(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, const QVariantList &data, const QStringList &columnHeaders)
{
    Q_UNUSED( clusteringCriteriaName );

    const QString metricViewName = metricName + "-" + viewName;

    QMutexLocker guard( &m_mutex );

    if ( ! m_watchedMetricViews.contains( metricViewName ) )
        return;

    QPair<int, int> indexes = m_watchedMetricViews.value( metricViewName );

    const double value = data.at( indexes.first ).toDouble();
    const QString definingLocation = data.at( indexes.second ).toString();

    int lineNumber;
    QString filename;

    ModifyPathSubstitutionsDialog::extractFilenameAndLine( definingLocation, filename, lineNumber );

    if ( filename.isEmpty() || lineNumber < 1 )
        return;

    QMap< QString, QVector< double > >& metricViewData = m_metrics[ metricViewName ];

    QVector< double >& metrics = metricViewData[ filename ];

    if ( metrics.size() == 0 ) {
        // initialize max value to first value
        metrics.push_back( value );
    }
    else {
        // update max value as appropriate
        if ( value > metrics[0] ) {
            metrics[0] = value;
        }
    }

    if ( metrics.size() < lineNumber+1 )
        metrics.resize( lineNumber+1 );

    metrics[ lineNumber ] = value;
}

/**
 * @brief SourceViewMetricsCache::clear
 *
 * Clears the map containers representing the metric cache state.  This needs to be called when the Metric
 * Table View no longer maintains the corresponding views.
 */
void SourceViewMetricsCache::clear()
{
    QMutexLocker guard( &m_mutex );

    m_watchedMetricViews.clear();
    m_metrics.clear();
}


} // GUI
} // ArgoNavis

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
#include "widgets/PerformanceDataMetricView.h"

#include <QVariant>
#include <QAction>


namespace ArgoNavis { namespace GUI {


const QString s_timeTitle = QStringLiteral("Time (msec)");
const QString s_timeSecTitle = QStringLiteral("Time (sec)");
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
 * @param currentFileName - the name of the file current displayed in the source-code view
 * @return the map for the specified metric view
 *
 * Get a copy of the metric cache for the specified metric view.
 */
QVector<double> SourceViewMetricsCache::getMetricsCache(const QString &metricViewName, const QString &currentFileName)
{
    QMutexLocker guard( &m_mutex );

    if ( m_watchedMetricNames.contains( metricViewName ) && m_metrics.contains( metricViewName ) && m_metrics[ metricViewName ].contains( currentFileName ) ) {

        const QMap< QString, QVector<double> >& metrics = m_metrics[ metricViewName ][ currentFileName ];

        const QString selectedMetricName = m_watchedMetricNames[ metricViewName ];

        if ( metrics.contains( selectedMetricName ) )
            return metrics[ selectedMetricName ];
    }

    return QVector<double>();
}

/**
 * @brief SourceViewMetricsCache::getMetricChoices
 * @param metricViewName - the current metric view name
 * @return - the list of metric names that can be selected
 *
 * This function returns the list of metric names that can be selected for a particular metric view.
 */
QStringList SourceViewMetricsCache::getMetricChoices(const QString& metricViewName) const
{
    QMutexLocker guard( &m_mutex );

    if ( m_watchableMetricNames.contains( metricViewName ) ) {
        QStringList choices;
        const std::set< QString >& choiceSet( m_watchableMetricNames[ metricViewName ] );
        for ( std::set< QString >::iterator iter = choiceSet.cbegin(); iter != choiceSet.cend(); iter++ ) {
            choices << *iter;
        }
        return choices;
    }

    return QStringList();
}

/**
 * @brief SourceViewMetricsCache::selectedMetricDetails
 * @param metricViewName - the current metric view name
 * @param name - returns the name of the selected metric
 * @param type - returns the value type of the selected metric
 *
 * This function returns details regarding the selected metric - the name and the value type.
 */
void SourceViewMetricsCache::getSelectedMetricDetails(const QString& metricViewName, QString &name, QVariant::Type &type) const
{
    QMutexLocker guard( &m_mutex );

    if ( m_watchedMetricNames.contains( metricViewName ) ) {
        name = m_watchedMetricNames[ metricViewName ];

        type = ( name == s_timeTitle || name == s_timeSecTitle ) ? QVariant::Double : QVariant::ULongLong;
    }
    else {
        name = QString();

        type = QVariant::Invalid;
    }
}

/**
 * @brief SourceViewMetricsCache::handleAddMetricView
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param modeName - the mode name
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 * @param metrics - list of metrics for setting column headers
 *
 * Extracts the column numbers for the defining location and metric value for the specified metric view
 * and adds an entry in the watched metric views map.
 */
void SourceViewMetricsCache::handleAddMetricView(const QString &clusteringCriteriaName, const QString& modeName, const QString &metricName, const QString &viewName, const QStringList &metrics)
{
    Q_UNUSED( clusteringCriteriaName );

    const QStringList PAPI_EVENT_LIST = metrics.filter( QStringLiteral("PAPI") );

    if ( metrics.contains( s_functionTitle ) && ( metrics.contains( s_timeTitle ) || metrics.contains( s_timeSecTitle ) || ! PAPI_EVENT_LIST.empty() ) ) {

        const QString metricViewName = PerformanceDataMetricView::getMetricViewName( modeName, metricName, viewName );

        const int timeTitleIdx = metrics.indexOf( s_timeTitle );
        const int timeSecTitleIdx = metrics.indexOf( s_timeSecTitle );
        const int functionTitleIdx = metrics.indexOf( s_functionTitle );

        QMap< QString, int > metricIndexes;

        metricIndexes[ s_functionTitle ] = functionTitleIdx;

        if ( timeTitleIdx != -1 ) {
            metricIndexes[ s_timeTitle ] = timeTitleIdx;
        }

        if ( timeSecTitleIdx != -1 ) {
            metricIndexes[ s_timeSecTitle ] = timeSecTitleIdx;
        }

        foreach ( const QString& papiEventName, PAPI_EVENT_LIST ) {
            metricIndexes[ papiEventName ] = metrics.indexOf( papiEventName );
        }

        QMutexLocker guard( &m_mutex );

        // initialize the map of indexes for each metric name
        m_watchedMetricViews.insert( metricViewName, metricIndexes );

        // determine default selected metrc name
        QString defaultSelectedMetric;

        // initialize the entire set of metric names that can be selected
        if ( timeTitleIdx != -1 ) {
            m_watchableMetricNames[ metricViewName ].insert( s_timeTitle );
            defaultSelectedMetric = s_timeTitle;
        }

        if ( timeSecTitleIdx != -1 ) {
            m_watchableMetricNames[ metricViewName ].insert( s_timeSecTitle );
            defaultSelectedMetric = s_timeSecTitle;
        }

        foreach ( const QString& event, PAPI_EVENT_LIST ) {
            m_watchableMetricNames[ metricViewName ].insert( event );
        }

        if ( defaultSelectedMetric.isEmpty() && ! PAPI_EVENT_LIST.isEmpty() ) {
            defaultSelectedMetric = PAPI_EVENT_LIST.first();
        }

        // default selected metric is either the time metric or the first PAPI event item
        m_watchedMetricNames.insert( metricViewName, defaultSelectedMetric );
    }
}

/**
 * @brief SourceViewMetricsCache::handleAddMetricViewData
 * @param clusteringCriteriaName - the name of the clustering criteria
 * @param modeName - the mode name
 * @param metricName - the name of the metric requested in the metric view
 * @param viewName - the name of the view requested in the metric view
 * @param data - the data to add to the model
 * @param columnHeaders - if present provides the names of the columns for each index in the data
 *
 * Extracts the data for one entry of the specified metric view and stores in the corresponding cache map .
 */
void SourceViewMetricsCache::handleAddMetricViewData(const QString &clusteringCriteriaName, const QString& modeName, const QString &metricName, const QString &viewName, const QVariantList &data, const QStringList &columnHeaders)
{
    Q_UNUSED( clusteringCriteriaName );
    Q_UNUSED( columnHeaders );

    const QString metricViewName = PerformanceDataMetricView::getMetricViewName( modeName, metricName, viewName );

    QMutexLocker guard( &m_mutex );

    // return if the metric view name is not contained in either watched data structures
    if ( ! m_watchedMetricViews.contains( metricViewName ) || ! m_watchedMetricNames.contains( metricViewName ) )
        return;

    QMap< QString, QMap< QString, QVector< double > > >& metricViewData = m_metrics[ metricViewName ];

    // get the list of metric name / index pairs
    const QMap< QString, int >& metricIndexes = m_watchedMetricViews[ metricViewName ];

    if ( ! metricIndexes.contains( s_functionTitle ) )
         return;

    const QString definingLocation = data.at( metricIndexes[ s_functionTitle ] ).toString();

    int lineNumber;
    QString filename;

    ModifyPathSubstitutionsDialog::extractFilenameAndLine( definingLocation, filename, lineNumber );

    if ( filename.isEmpty() || lineNumber < 1 )
        return;   // skip invalid filename or line number

    QMap< QString, QVector< double > >& metricFileData = metricViewData[ filename ];

    for ( QMap< QString, int >::const_iterator iter = metricIndexes.begin(); iter != metricIndexes.end(); iter++ ) {
        const QString metricName = iter.key();

        if ( metricName == s_functionTitle )
            continue;  // skip the function name metric

        const int metricIndex = iter.value();

        const double value = data.at( metricIndex ).toDouble();

        QVector< double >& metrics = metricFileData[ metricName ];

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
    m_watchedMetricNames.clear();
    m_watchableMetricNames.clear();
}

/**
 * @brief SourceViewMetricsCache::handleSelectedMetricChanged
 *
 * This handler is called when the user selects a metric name used to annotate the source-code view.
 */
void SourceViewMetricsCache::handleSelectedMetricChanged()
{
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( action ) {

        const QString metricViewName = action->property( "metricViewName" ).toString();

        QMutexLocker guard( &m_mutex );

        m_watchedMetricNames[ metricViewName ] = action->text();

        emit signalSelectedMetricChanged( metricViewName, action->text() );
    }
}


} // GUI
} // ArgoNavis

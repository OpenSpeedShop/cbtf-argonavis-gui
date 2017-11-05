/*!
   \file PerformanceDataGraphView.h
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

#ifndef PERFORMANCEDATAGRAPHVIEW_H
#define PERFORMANCEDATAGRAPHVIEW_H

#include <QWidget>

#include <QMutex>

#include <random>

#include "qcustomplot.h"

#include "common/openss-gui-config.h"

namespace Ui {
class PerformanceDataGraphView;
}

class CustomPlot;


namespace ArgoNavis { namespace GUI {


class PerformanceDataGraphView : public QWidget
{
    Q_OBJECT

public:

    explicit PerformanceDataGraphView(QWidget *parent = 0);
    virtual ~PerformanceDataGraphView();

    void unloadExperimentDataFromView(const QString& experimentName);

private slots:

    void handleSelectionChanged();

    void handleInitGraphView(const QString &clusteringCriteriaName,
                             const QString &metricNameTitle,
                             const QString &metricName,
                             const QString &viewName,
                             const QStringList &eventNames,
                             const QStringList &items);

    void handleAddGraphItem(const QString &clusteringCriteriaName,
                            const QString &metricNameTitle,
                            const QString &metricName,
                            double eventTime,
                            double eventData,
                            int rankOrThread);

    void handleAddGraphItem(const QString &metricName,
                            const QString &viewName,
                            const QString &eventName,
                            const QString &name,
                            double data);

    void handleGraphMinAvgMaxRanks(const QString &metricName,
                                   int rankWithMinValue,
                                   int rankClosestToAvgValue,
                                   int rankWithMaxValue);

    void handleAxisRangeChange(const QCPRange &requestedRange);

    void handleRequestMetricViewComplete(const QString &clusteringCriteriaName,
                                         const QString &modeName,
                                         const QString &metricName,
                                         const QString &viewName,
                                         double lower,
                                         double upper);

#if defined(HAS_QCUSTOMPLOT_V2)
    void handleTabChanged(int index);
#endif

private:

    CustomPlot* initPlotView(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName, const QString &viewName = QString(), bool handleGraphRangeChanges = true);
    QCPGraph* initGraph(CustomPlot* plot, int rankOrThread);
    QColor goldenRatioColor(std::mt19937 &mt) const;
    QString normalizedName(const QString &name, const QCustomPlot* plot, bool isFilePath) const;

private:

    Ui::PerformanceDataGraphView *ui;

    QMutex m_mutex;

    typedef struct MetricGroup {
        QCPRange xGraphRange;      // time range for metric group
        QCPRange yGraphRange;      // time range for metric group
        CustomPlot* graph;         // the QCustomPlot instance
        QMap< int, QCPGraph* > subgraphs;  // QCPGraph instance for rank/process
        QMap< QString, QCPBars* > bars;    // QCPBars instance for each event
        QMap< QString, QString > items;    // list of individually graphed items along x-axis: key=full name, value=elided name
        bool legendItemAdded;              // legend item added
        std::mt19937 mt;                   // use constant seed whose initial sequence of values seemed to generate good colors for small
        MetricGroup(CustomPlot* plot, const QCPRange& xRange = QCPRange(), const QCPRange& yRange = QCPRange())
            : xGraphRange( xRange ), yGraphRange( yRange ), graph( plot ), legendItemAdded( false ), mt( 2560000 ) { }
        MetricGroup()
            : graph( Q_NULLPTR ), legendItemAdded( false ), mt( 2560000 ) { }
    } MetricGroup;

    QMap< QString, MetricGroup > m_metricGroup;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAGRAPHVIEW_H

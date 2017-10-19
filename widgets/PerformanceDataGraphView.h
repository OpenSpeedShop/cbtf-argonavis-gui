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

signals:

    void graphRangeChanged(const QString& clusteringCriteriaName,const QString& clusterName, double lower, double upper, const QSize& size);


private slots:

    void handleAddGraphItem(const QString &clusteringCriteriaName,
                            const QString &metricNameTitle,
                            const QString &metricName,
                            double eventTime,
                            double eventData,
                            int rankOrThread);

    void handleAxisRangeChange(const QCPRange &requestedRange);

    void handleRequestMetricViewComplete(const QString &clusteringCriteriaName,
                                         const QString &modeName,
                                         const QString &metricName,
                                         const QString &viewName,
                                         double lower,
                                         double upper);

private:

    CustomPlot* initPlotView(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName);

private:

    Ui::PerformanceDataGraphView *ui;

    QMutex m_mutex;

    typedef struct MetricGroup {
        QCPRange xGraphRange;  // time range for metric group
        QCPRange yGraphRange;  // time range for metric group
        CustomPlot* graph;     // the QCustomPlot instance
        MetricGroup(CustomPlot* plot, const QCPRange& xRange = QCPRange(), const QCPRange& yRange = QCPRange())
            : xGraphRange( xRange ), yGraphRange( yRange ), graph( plot ) { }
        MetricGroup()
            : graph( Q_NULLPTR ) { }
    } MetricGroup;

    QMap< QString, MetricGroup > m_metricGroup;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAGRAPHVIEW_H

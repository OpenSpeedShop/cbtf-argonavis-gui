/*!
   \file PerformanceDataPlotView.h
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

#ifndef PERFORMANCE_DATA_VIEW_H
#define PERFORMANCE_DATA_VIEW_H

#include <QWidget>
#include <QMutex>

#include "qcustomplot.h"

#include "common/openss-gui-config.h"

namespace Ui {
class PerformanceDataPlotView;
}


namespace ArgoNavis {

namespace Base {
class Time;
}

namespace CUDA {
class DataTransfer;
class KernelExecution;
}

namespace GUI {

class OSSEventsSummaryItem;

class PerformanceDataPlotView : public QWidget
{
    Q_OBJECT

public:

    explicit PerformanceDataPlotView(QWidget *parent = 0);
    virtual ~PerformanceDataPlotView();

    void unloadExperimentDataFromView(const QString& experimentName);

    QList< QCPAxisRect* > getAxisRectsForMetricGroup(const QString& metricGroupName);

signals:

    void graphRangeChanged(const QString& clusterName, double lower, double upper, const QSize& size);

protected:

    QSize sizeHint() const Q_DECL_OVERRIDE;

private slots:

    void handleAxisRangeChange(const QCPRange &requestedRange);
    void handleAxisRangeChangeForMetricGroup(const QCPRange &requestedRange);
    void handleAxisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
    void handleSelectionChanged();
    void handleGraphClicked(QCPAbstractPlottable *plottable);
    void handleItemClick(QCPAbstractItem *item, QMouseEvent *event);

    void handleAddCluster(const QString& clusteringCriteriaName, const QString& clusterName);

    void handleSetMetricDuration(const QString& clusteringCriteriaName, const QString& clusterName, double duration);

    void handleAddDataTransfer(const QString &clusteringCriteriaName,
                               const QString &clusterName,
                               const Base::Time &time_origin,
                               const CUDA::DataTransfer &details);

    void handleAddKernelExecution(const QString& clusteringCriteriaName,
                                  const QString& clusterName,
                                  const Base::Time& time_origin,
                                  const CUDA::KernelExecution& details);

    void handleAddPeriodicSample(const QString& clusteringCriteriaName,
                                 const QString& clusterName,
                                 const double& time_begin,
                                 const double& time_end,
                                 const double& count);
    void handleCudaEventSnapshot(const QString& clusteringCriteriaName, const QString& clusteringName, double lower, double upper, const QImage& image);

private:

    void initPlotView(const QString& metricGroupName, QCPAxisRect* axisRect);
    QList< QCPAxis* > getAxesForMetricGroup(const QCPAxis::AxisType axisType, const QString& metricGroupName);
    const QCPRange getRange(const QVector<double> &values, bool sortHint = false);
    double getGraphInfoForMetricGroup(const QCPAxis *axis, QString& clusterName, QSize& size);

private:

    Ui::PerformanceDataPlotView *ui;

    typedef struct {
        double duration;                          // duration for metric group
        QCPLayoutGrid* layout;                    // layout for the axis rects
        QMap< QString, QCPAxisRect* > axisRects;  // an axis rect for each group item
        QStringList metricList;                   // list of metrics
        QCPMarginGroup* marginGroup;              // one margin group to line up the left and right axes
        QMap< QString, OSSEventsSummaryItem* > eventSummary;
    } MetricGroup;

    QMap< QString, MetricGroup* > m_metricGroups; // defines each metric group

    QMutex m_mutex;

    int m_metricCount;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCE_DATA_VIEW_H

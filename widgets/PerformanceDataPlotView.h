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


class PerformanceDataPlotView : public QWidget
{
    Q_OBJECT

public:

    explicit PerformanceDataPlotView(QWidget *parent = 0);
    virtual ~PerformanceDataPlotView();

    void unloadExperimentDataFromView(const QString& experimentName);

    QList< QCPAxisRect* > getAxisRectsForMetricGroup(const QString& metricGroupName);

public slots:

    void addMetric(const QString& metricGroupName, const QString& metricName);

    void setMetricDuration(const QString& metricGroupName, const QString& metricName, double duration);

    void handleAddDataTransfer(const QString &metricGroupName,
                               const QString &metricName,
                               const Base::Time &time_origin,
                               const CUDA::DataTransfer &details);

    void handleAddKernelExecution(const QString& metricGroupName,
                                  const QString& metricName,
                                  const Base::Time& time_origin,
                                  const CUDA::KernelExecution& details);

    void handleAddPeriodicSample(const QString& metricGroupName,
                                 int counterIndex,
                                 const double& time_begin,
                                 const double& time_end,
                                 const double& count);

protected:

    QSize sizeHint() const Q_DECL_OVERRIDE;

private slots:

    void handleAxisRangeChange(const QCPRange &requestedRange);
    void handleAxisRangeChangeForMetricGroup(const QCPRange &requestedRange);
    void handleAxisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
    void handleSelectionChanged();
    void handleGraphClicked(QCPAbstractPlottable *plottable);
    void handleItemClick(QCPAbstractItem *item, QMouseEvent *event);

private:

    void initPlotView(const QString& metricGroupName, QCPAxisRect* axisRect);
    QList< QCPAxis* > getAxesForMetricGroup(const QCPAxis::AxisType axisType, const QString& metricGroupName);
    const QCPRange getRange(const QVector<double> &values, bool sortHint = false);
    double getDurationForMetricGroup(const QCPAxis* axis);

private:

    Ui::PerformanceDataPlotView *ui;

    typedef struct {
        double duration;                          // duration for metric group
        QCPLayoutGrid* layout;                    // layout for the axis rects
        QMap< QString, QCPAxisRect* > axisRects;  // an axis rect for each group item
        QStringList metricList;                   // list of metrics
        QCPMarginGroup* marginGroup;              // one margin group to line up the left and right axes
    } MetricGroup;

    QMap< QString, MetricGroup* > m_metricGroups; // defines each metric group

    QMutex m_mutex;

    int m_metricCount;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCE_DATA_VIEW_H

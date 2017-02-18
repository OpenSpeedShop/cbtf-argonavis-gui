#ifndef METRICPLOTVIEWMANAGER_H
#define METRICPLOTVIEWMANAGER_H

#include <QScrollArea>

namespace Ui {
class MetricPlotViewManager;
}

class MetricPlotViewManager : public QScrollArea
{
    Q_OBJECT

public:
    explicit MetricPlotViewManager(QWidget *parent = 0);
    ~MetricPlotViewManager();

private:
    Ui::MetricPlotViewManager *ui;
};

#endif // METRICPLOTVIEWMANAGER_H

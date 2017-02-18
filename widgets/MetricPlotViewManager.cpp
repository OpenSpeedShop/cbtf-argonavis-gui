#include "MetricPlotViewManager.h"
#include "ui_MetricPlotViewManager.h"

MetricPlotViewManager::MetricPlotViewManager(QWidget *parent) :
    QScrollArea(parent),
    ui(new Ui::MetricPlotViewManager)
{
    ui->setupUi(this);
}

MetricPlotViewManager::~MetricPlotViewManager()
{
    delete ui;
}

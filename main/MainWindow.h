/*!
   \file MainWindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>

#include "common/openss-gui-config.h"

#include "widgets/MetricViewManager.h"

// Forward Declarations

namespace Ui {
class MainWindow;
}


namespace ArgoNavis { namespace GUI {


/*!
 * \brief The MainWindow class
 */

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

    void setExperimentDatabase(const QString& filename);

protected:

    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private slots:

    void loadOpenSsExperiment();
    void unloadOpenSsExperiment();
    void handleLoadComplete();
    void addUnloadOpenSsExperimentMenuItem(const QString& filePath);
    void handleAdjustPlotViewScrollArea(const QString& clusteringCriteriaName, const QString& clusterName);
    void handleRemoveCluster(const QString& clusteringCriteriaName, const QString& clusterName);
    void handleSetDefaultMetricView(const MetricViewTypes& view, bool hasCompareViews, bool hasLoadBalanceViews, bool hasTraceViews);
    void shutdownApplication();
    void handleViewQuickStartGuide();
    void handleViewReferenceGuide();
    void handleAbout();

private:

    void loadExperimentDatabase(const QString& filepath);

private:

    Ui::MainWindow *ui;

    QString m_filename;

    QSet< QString > m_plotsMap;  // key = <ClusteringCriteriaName>-<ClusterName>

};


} // GUI
} // ArgoNavis

#endif // MAINWINDOW_H

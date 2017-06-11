/*!
   \file ExperimentPanel.h
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

#ifndef EXPERIMENTPANEL_H
#define EXPERIMENTPANEL_H

#include <QWidget>
#include <QTreeView>
#include <QVector>
#include <QString>
#include <QMutex>

#include <ArgoNavis/CUDA/PerformanceData.hpp>

#include "ToolAPI.hxx"

using namespace OpenSpeedShop;
using namespace OpenSpeedShop::Framework;


namespace ArgoNavis { namespace GUI {


// [ Forward Declarations ]
class TreeModel;
class TreeItem;


/*!
 * \brief The ExperimentPanel class
 */

class ExperimentPanel : public QWidget
{
    Q_OBJECT

public:

    explicit ExperimentPanel(QWidget *parent = 0);

signals:

    void criteriaSelectionUpdate();
    void signalSelectedClustersChanged(const QString& criteriaName, const QSet< QString >& selected);

public slots:

    void handleAddExperiment(const QString& name,
                             const QString& clusteringCriteriaName,
                             const QVector< QString >& clusterNames,
                             const QVector<bool> &clusterHasGpuSampleCounters,
                             const QVector< QString >& sampleCounterNames);


    void handleRemoveExperiment(const QString& name);

private slots:

    void handleCheckedChanged(bool value);

private:

    QTreeView m_expView;
    TreeModel* m_expModel;
    TreeItem* m_root;

    QMutex m_mutex;
    QSet< QString > m_selectedClusters;
    QStringList m_loadedExperiments;

};


} // GUI
} // ArgoNavis

#endif // EXPERIMENTPANEL_H

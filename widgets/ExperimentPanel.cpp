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

#include "ExperimentPanel.h"

#include "TreeModel.h"
#include "TreeItem.h"

#include "managers/PerformanceDataManager.h"

#include <QVBoxLayout>
#include <QDebug>


namespace ArgoNavis { namespace GUI {


/**
 * @brief ExperimentPanel::ExperimentPanel
 * @param parent - the parent widget
 *
 * Constructs an experiment panel instance of the given parent.
 */
ExperimentPanel::ExperimentPanel(QWidget *parent)
    : QWidget( parent )
{
    setStyleSheet("QWidget {"
                  "   font: 14px;"
                  "}");

    QVBoxLayout* vLayout = new QVBoxLayout( this );
    vLayout->addWidget( &m_expView );

    QList<QVariant> rootData;
    rootData << "Currently Loaded Experiment Information" << QString() << QString();
    m_root = new TreeItem( rootData );

    m_expModel = new TreeModel( m_root, this );

    m_expView.setModel( m_expModel );

    m_expView.resizeColumnToContents( 0 );
    m_expView.setEditTriggers( QAbstractItemView::NoEditTriggers );
    m_expView.setSelectionMode( QAbstractItemView::NoSelection );
    m_expView.setSizeAdjustPolicy( QAbstractItemView::AdjustToContents );
    m_expView.setStyleSheet("QTreeView {"
                            "   font: 14px;"
                            "}"
                            "QTreeView::branch:has-siblings:!adjoins-item {"
                            "    border-image: url(:/images/vline) 0;"
                            "}"
                            " "
                            "QTreeView::branch:has-siblings:adjoins-item {"
                            "    border-image: url(:/images/branch-more) 0;"
                            "}"
                            " "
                            "QTreeView::branch:!has-children:!has-siblings:adjoins-item {"
                            "    border-image: url(:/images/branch-end) 0;"
                            "}"
                            " "
                            "QTreeView::branch:has-children:!has-siblings:closed,"
                            "QTreeView::branch:closed:has-children:has-siblings {"
                            "        border-image: none;"
                            "        image: url(:/images/branch-closed);"
                            "}"
                            " "
                            "QTreeView::branch:open:has-children:!has-siblings,"
                            "QTreeView::branch:open:has-children:has-siblings  {"
                            "        border-image: none;"
                            "        image: url(:/images/branch-open);"
                            "}");

    // connect performance data manager signals to performance data metric view slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, &PerformanceDataManager::updateComputeDataTransferRatio, this, &ExperimentPanel::handleUpdateComputeDataTransferRatio );
#else
        connect( dataMgr, SIGNAL(updateComputeDataTransferRatio(QString,double)), this, SLOT(handleUpdateComputeDataTransferRatio(QString,double)) );
#endif
    }
}

/**
 * @brief ExperimentPanel::handleAddExperiment
 * @param name - the experiment name
 * @param clusteringCriteriaName - the clustering criteria name
 * @param clusterNames - the clustering group names
 * @param sampleCounters - the sample counter identifiers
 *
 * Add the given experiment to the tree model which will be detected and added to the view.
 */
void ExperimentPanel::handleAddExperiment(const QString &name, const QString &clusteringCriteriaName, const QVector<QString> &clusterNames, const QVector<QString> &sampleCounterNames)
{
#if 1
    // create experiment item and add as child of the root item
    TreeItem* expItem = new TreeItem( QList< QVariant>() << name << "" << "", m_root );
    m_root->appendChild( expItem );

    // create criteria item and add as child of the experiment item
    TreeItem* expCriteriaItem = new TreeItem( QList< QVariant >() << clusteringCriteriaName << "In View" << "Overall", expItem );
    expItem->appendChild( expCriteriaItem );

    foreach( const QString& clusterName, clusterNames ) {
        // create new cluster item and add as child of the criteria item
        TreeItem* clusterItem = new TreeItem( QList< QVariant >() << clusterName << 0.0 << 0.0, expCriteriaItem );
        clusterItem->setCheckable( true );
        clusterItem->setChecked( true );
        clusterItem->setEnabled( false );
        expCriteriaItem->appendChild( clusterItem );

        // is this cluster item associated with a GPU view?
        bool isGpuCluster( clusterName.contains( QStringLiteral("GPU") ) );

        // add children: experiment sample counters
        foreach( const QString& counterName, sampleCounterNames ) {
            // is this counter a GPU counter?
            bool isGpuSampleCounter( counterName.contains( QStringLiteral("GPU") ) );
            // if flags match then add the counter to the cluster item
            if ( isGpuCluster == isGpuSampleCounter ) {
                // create new counter item and add as child of the cluster item
                TreeItem* counterItem = new TreeItem( QList< QVariant >() << counterName, clusterItem );
                clusterItem->appendChild( counterItem );
            }
        }
    }

    m_expView.resizeColumnToContents( 0 );
    m_expView.expandAll();
#else
    // get number of experiments loaded
    int expCount = m_expModel->rowCount();

    // add root child: experiment
    bool success = m_expModel->insertRow( expCount, QModelIndex() );
    Q_ASSERT( success );
    QModelIndex expIndex = m_expModel->index( expCount, 0 );
    m_expModel->setData( expIndex, name, Qt::EditRole );

    // add children: experiment group criteria
    success = m_expModel->insertRow( expCount, expIndex );
    Q_ASSERT( success );
    QModelIndex critIndex = m_expModel->index( 0, 0, expIndex );
    m_expModel->setData( critIndex, clusteringCriteriaName, Qt::EditRole );

    int clusterNum = 0;
    foreach( const QString& clusterName, clusterNames ) {
        // add children: experiment thread group
        success = m_expModel->insertRow( clusterNum, critIndex );
        Q_ASSERT( success );
        QModelIndex threadIndex = m_expModel->index( clusterNum, 0, critIndex );
        TreeItem* item = m_expModel->getItem( threadIndex );
        item->setCheckable( true );
        m_expModel->setData( threadIndex, true, Qt::CheckStateRole );
        m_expModel->setData( threadIndex, clusterName, Qt::EditRole );
        ++clusterNum;

        bool isGpuCluster( clusterName.contains( QStringLiteral("GPU") ) );

        // add children: experiment sample counters
        int counter = 0;
        foreach( const QString& counterName, sampleCounterNames ) {
            bool isGpuSampleCounter( counterName.contains( QStringLiteral("GPU") ) );
            if ( isGpuCluster == isGpuSampleCounter ) {
                success = m_expModel->insertRow( counter, threadIndex );
                Q_ASSERT( success );
                QModelIndex counterIndex = m_expModel->index( counter, 0, threadIndex );
                m_expModel->setData( counterIndex, counterName, Qt::EditRole );
                ++counter;
            }
        }
    }
#endif
}

/**
 * @brief ExperimentPanel::handleRemoveExperiment
 * @param name - the experiment name
 *
 * Remove the given experiment from the tree model which will be detected and removed from the view.
 */
void ExperimentPanel::handleRemoveExperiment(const QString &name)
{
    for ( int i=0; i<m_expModel->rowCount(); ++i ) {
        QVariant var = m_expModel->data( m_expModel->index( i, 0 ) );
        if ( var.isValid() && var.toString() == name ) {
            m_expModel->removeRow( i );
            break;
        }
    }
}

/**
 * @brief updateComputerDataTransferRatio
 * @param clusterName
 * @param value
 */
void ExperimentPanel::handleUpdateComputeDataTransferRatio(const QString &clusterName, double value)
{
    static bool first = true;
    qDebug() << "COMPUTE/DATA TRANSFER RATION =" << value;
    if ( m_root->childCount() == 1 && m_root->columnCount() == 3 ) {
        TreeItem* expItem = m_root->child( 0 );
        QModelIndex expModelIndex = m_expModel->index( 0, 0 );
        if ( expItem ) {
            TreeItem* expCriteriaItem = expItem->child( 0 );
            QModelIndex expCriteriaIndex = m_expModel->index( 0, 0, expModelIndex );
            if ( expCriteriaItem ) {
                for ( int i=0; i<expCriteriaItem->childCount(); ++i ) {
                    TreeItem* clusterItem = expCriteriaItem->child( i );
                    if ( clusterItem && clusterItem->data( 0 ).toString() == clusterName ) {
                        QModelIndex index = m_expModel->index( i, first ? 2 : 1, expCriteriaIndex );
                        m_expModel->setData( index, value );
                    }
                }
                first = false;
            }
        }
    }
}


} // GUI
} // ArgoNavis

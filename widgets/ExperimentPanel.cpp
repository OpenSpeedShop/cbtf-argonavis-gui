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
    rootData << "Currently Loaded Experiment Information";
    m_root = new TreeItem( rootData );

    m_expModel = new TreeModel( m_root, this );

    m_expView.setModel( m_expModel );

    m_expView.resizeColumnToContents( 0 );
    m_expView.setEditTriggers( QAbstractItemView::NoEditTriggers );
    m_expView.setSelectionMode( QAbstractItemView::NoSelection );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    m_expView.setSizeAdjustPolicy( QAbstractItemView::AdjustToContents );
#endif
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
}

/**
 * @brief ExperimentPanel::handleCheckedChanged
 * @param value
 */
void ExperimentPanel::handleCheckedChanged(bool value)
{
    QMutexLocker guard( &m_mutex );

    if ( m_loadedExperiments.isEmpty() )
        return;

    TreeItem* item = qobject_cast< TreeItem* >( sender() );
    if ( item ) {
        const QSet<QString>::size_type size = m_selectedClusters.size();
        const QString clusterName = item->data( 0 ).toString();
        qDebug() << Q_FUNC_INFO << clusterName << " = " << value;
        if ( value )
            m_selectedClusters.insert( clusterName );
        else
            m_selectedClusters.remove( clusterName );
        if ( size != m_selectedClusters.size() ) {
            TreeItem* parent = item->parentItem();
            const QString clusteringCriteriaName = parent->data( 0 ).toString();
            emit signalSelectedClustersChanged( clusteringCriteriaName, m_selectedClusters );
        }
    }
}

/**
 * @brief ExperimentPanel::handleAddExperiment
 * @param name - the experiment name
 * @param clusteringCriteriaName - the clustering criteria name
 * @param clusterNames - the clustering group names
 * &param clusterHasGpuSampleCounters - whether the cluster has GPU sample counter data
 * @param sampleCounters - the sample counter identifiers
 *
 * Add the given experiment to the tree model which will be detected and added to the view.
 */
void ExperimentPanel::handleAddExperiment(const QString &name, const QString &clusteringCriteriaName, const QVector<QString> &clusterNames, const QVector< bool >& clusterHasGpuSampleCounters, const QVector<QString> &sampleCounterNames)
{
    // create experiment item and add as child of the root item
    TreeItem* expItem = new TreeItem( QList< QVariant>() << name, m_root );
    m_root->appendChild( expItem );

    // create criteria item and add as child of the experiment item
    TreeItem* expCriteriaItem = new TreeItem( QList< QVariant >() << clusteringCriteriaName, expItem );
    expItem->appendChild( expCriteriaItem );

    int index( 0 );

    foreach( const QString& clusterName, clusterNames ) {
        // create new cluster item and add as child of the criteria item
        TreeItem* clusterItem = new TreeItem( QList< QVariant >() << clusterName, expCriteriaItem, true, true );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( clusterItem, &TreeItem::checkedChanged, this, &ExperimentPanel::handleCheckedChanged );
#else
        connect( clusterItem, SIGNAL(checkedChanged(bool)), this, SLOT(handleCheckedChanged(bool)) );
#endif
        clusterItem->setChecked( true );

        expCriteriaItem->appendChild( clusterItem );

        // is this cluster item associated with a GPU view?
        bool isGpuCluster( index < clusterHasGpuSampleCounters.size() && clusterHasGpuSampleCounters[ index++ ] );

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

    QMutexLocker guard( &m_mutex );

    m_loadedExperiments << name;

    foreach( QString name, clusterNames ) {
        m_selectedClusters.insert( name );
    }
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

    QMutexLocker guard( &m_mutex );

    m_loadedExperiments.removeOne( name );

    m_selectedClusters.clear();
}


} // GUI
} // ArgoNavis

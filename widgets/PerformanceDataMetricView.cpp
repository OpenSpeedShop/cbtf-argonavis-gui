/*!
   \file PerformanceDataTableView.cpp
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

#include "PerformanceDataMetricView.h"
#include "ui_PerformanceDataMetricView.h"
#include "common/openss-gui-config.h"

#include "managers/PerformanceDataManager.h"

#include <QStackedLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>


namespace ArgoNavis { namespace GUI {


/**
 * @brief PerformanceDataTableView::PerformanceDataTableView
 * @param parent - specify parent of the PerformanceDataTableView instance
 *
 * Constructs a widget which is a child of parent.  If parent is 0, the new widget becomes a window.  If parent is another widget,
 * this widget becomes a child window inside parent. The new widget is deleted when its parent is deleted.
 */
PerformanceDataMetricView::PerformanceDataMetricView(QWidget *parent)
    : QWidget( parent )
    , ui( new Ui::PerformanceDataMetricView )
{
    ui->setupUi( this );

    setStyleSheet("QWidget {"
                  "   font: 14px;"
                  "}");

    // create stacked layout to hold various metric views
    m_viewStack = new QStackedLayout( ui->widget_ViewStack );

    // define blank view
    QTreeView* blankView = new QTreeView;
    m_views[ "none" ] = blankView;
    m_viewStack->addWidget( blankView );

    // connect performance data manager signals to performance data metric view slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, &PerformanceDataManager::addMetricView, this, &PerformanceDataMetricView::handleInitModel );
        connect( dataMgr, &PerformanceDataManager::addMetricViewData, this, &PerformanceDataMetricView::handleAddData );
#else
        connect( dataMgr, SIGNAL(addMetricView(QString,QStringList)), this, SLOT(handleInitModel(QString,QStringList)) );
        connect( dataMgr, SIGNAL(addMetricViewData(QString,QVariantList)), this, SLOT(handleAddData(QString,QVariantList)) );
#endif
    }

    // connect signal/slot for metric view selection handling
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( ui->comboBox_MetricViews, &QComboBox::currentTextChanged, this, &PerformanceDataMetricView::handleMetricViewChanged );
#else
    connect( ui->comboBox_MetricViews, SIGNAL(currentIndexChanged(QString)), this, SLOT(handleMetricViewChanged(QString)) );
#endif

    // initially show blank view
    showBlankView();
}

/**
 * @brief PerformanceDataMetricView::PerformanceDataMetricView
 *
 * Destroys the PerformanceDataMetricView instance.
 */
PerformanceDataMetricView::~PerformanceDataMetricView()
{
    QMutableMapIterator< QString, QTreeView* > viter( m_views );
    while ( viter.hasNext() ) {
        viter.next();

        m_viewStack->removeWidget( viter.value() );
        delete viter.value();
        viter.remove();
    }

    QMutableMapIterator< QString, QStandardItemModel* > miter( m_models );
    while ( miter.hasNext() ) {
        miter.next();

        delete miter.value();
        miter.remove();
    }

    QMutableMapIterator< QString, QSortFilterProxyModel* > pmiter( m_proxyModels );
    while ( pmiter.hasNext() ) {
        pmiter.next();

        delete pmiter.value();
        pmiter.remove();
    }

    delete ui;
}

/**
 * @brief PerformanceDataMetricView::showBlankView
 *
 * Show the blank view (set to be index zero).
 */
void PerformanceDataMetricView::showBlankView()
{
    m_viewStack->setCurrentIndex( 0 );
}

/**
 * @brief PerformanceDataMetricView::deleteAllModelsViews
 *
 * Delete all views and models.  Show the blank view.
 */
void PerformanceDataMetricView::deleteAllModelsViews()
{
    showBlankView();

    QMutableMapIterator< QString, QTreeView* > viter( m_views );
    while ( viter.hasNext() ) {
        viter.next();

        if ( viter.key() != "none" ) {
            m_viewStack->removeWidget( viter.value() );
            delete viter.value();
            viter.remove();
        }
    }

    QMutableMapIterator< QString, QStandardItemModel* > miter( m_models );
    while ( miter.hasNext() ) {
        miter.next();

        delete miter.value();
        miter.remove();
    }

    QMutableMapIterator< QString, QSortFilterProxyModel* > pmiter( m_proxyModels );
    while ( pmiter.hasNext() ) {
        pmiter.next();

        delete pmiter.value();
        pmiter.remove();
    }

    ui->comboBox_MetricViews->clear();
}

/**
 * @brief PerformanceDataMetricView::initModel
 * @param metricView - name of metric view to create
 * @param metrics - list of metrics for setting column headers
 *
 * Create and initialize the model and view for the new metric view.
 */
void PerformanceDataMetricView::handleInitModel(const QString &metricView, const QStringList &metrics)
{
    if ( m_models.contains( metricView ) ) {
        delete m_models.value( metricView );
    }

    if ( m_proxyModels.contains( metricView ) ) {
        delete m_proxyModels.value( metricView );
    }

    QStandardItemModel* model = new QStandardItemModel( 0, metrics.size(), this );
    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel;

    if ( Q_NULLPTR == model || Q_NULLPTR == proxyModel )
        return;

    for ( int i=0; i<metrics.size(); ++i ) {
        model->setHeaderData( i, Qt::Horizontal, metrics.at( i ) );
    }

    proxyModel->setFilterKeyColumn( 0 );
    proxyModel->setSourceModel( model );
    proxyModel->sort( 0, Qt::DescendingOrder );

    QTreeView* view = m_views.value( metricView, Q_NULLPTR );

    if ( Q_NULLPTR == view ) {
        view = new QTreeView;
    }

    if ( Q_NULLPTR == view )
        return;

    view->setSortingEnabled( true );
    view->setModel( proxyModel );

    for ( int i=0; i<metrics.size(); ++i ) {
       view->resizeColumnToContents( i );
    }

    m_viewStack->addWidget( view );

    m_models[ metricView ] = model;
    m_proxyModels[ metricView ] = proxyModel;
    m_views[ metricView ] = view;

    // Add metric view to combobox and make current view
    ui->comboBox_MetricViews->addItem( metricView );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    ui->comboBox_MetricViews->setCurrentText( metricView );
#else
    ui->comboBox_MetricViews->setCurrentIndex( ui->comboBox_MetricViews->count()-1 );
#endif
}

/**
 * @brief PerformanceDataMetricView::handleAddData
 * @param metricView - name of metric view for which to add data to model
 * @param data - the data to add to the model
 *
 * Inserts a row into the model of the specified metric view.
 */
void PerformanceDataMetricView::handleAddData(const QString &metricView, const QVariantList& data)
{
    QStandardItemModel* model = m_models.value( metricView );

    if ( Q_NULLPTR == model )
        return;

    model->insertRow( 0 );

    for ( int i=0; i<data.size(); ++i ) {
        model->setData( model->index( 0, i ), data.at( i ) );
    }
}

/**
 * @brief PerformanceDataMetricView::handleMetricViewChanged
 * @param metricView - name of metric view for which to add data to model
 *
 * Handles user request to switch metric view.
 */
void PerformanceDataMetricView::handleMetricViewChanged(const QString &metricView)
{
    QTreeView* view = m_views.value( metricView, Q_NULLPTR );

    if ( Q_NULLPTR != view ) {
        m_viewStack->setCurrentWidget( view );
    }
}


} // GUI
} // ArgoNavis

/*!
   \file PerformanceDataMetricView.cpp
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

#include "ViewSortFilterProxyModel.h"
#include "MetricViewDelegate.h"

#include "managers/PerformanceDataManager.h"
#include "managers/ApplicationOverrideCursorManager.h"
#include "SourceView/ModifyPathSubstitutionsDialog.h"
#include "widgets/ShowDeviceDetailsDialog.h"

#include <QStackedLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QAction>
#include <QFileInfo>


namespace ArgoNavis { namespace GUI {


QString PerformanceDataMetricView::s_functionTitle( tr("Function (defining location)") );
QString PerformanceDataMetricView::s_deviceTitle( tr("Device") );

QString PerformanceDataMetricView::s_metricModeName( tr("Metric") );
QString PerformanceDataMetricView::s_detailsModeName( tr("Details") );
QString PerformanceDataMetricView::s_calltreeModeName( tr("CallTree") );
QString PerformanceDataMetricView::s_compareModeName( tr("Compare") );
QString PerformanceDataMetricView::s_compareByRankModeName( tr("Compare By Rank") );
QString PerformanceDataMetricView::s_compareByHostModeName( tr("Compare By Host") );
QString PerformanceDataMetricView::s_compareByProcessModeName( tr("Compare By Process") );
QString PerformanceDataMetricView::s_loadBalanceModeName( tr("Load Balance") );
QString PerformanceDataMetricView::s_traceModeName( tr("Trace") );

QString PerformanceDataMetricView::s_functionViewName( tr("Functions") );
QString PerformanceDataMetricView::s_statementsViewName( tr("Statements") );
QString PerformanceDataMetricView::s_linkedObjectsViewName( tr("LinkedObjects") );
QString PerformanceDataMetricView::s_loopsViewName( tr("Loops") );

QString PerformanceDataMetricView::s_noneName( QStringLiteral("none") );

QString PerformanceDataMetricView::s_allEventsDetailsName( tr("All Events") );


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
    m_views[ s_noneName ] = blankView;
    m_viewStack->addWidget( blankView );

    // initialize model used for view combobox when in metric mode
    m_metricViewModel.appendRow( new QStandardItem( s_functionViewName ) );
    m_metricViewModel.appendRow( new QStandardItem( s_statementsViewName ) );
    m_metricViewModel.appendRow( new QStandardItem( s_linkedObjectsViewName ) );
    m_metricViewModel.appendRow( new QStandardItem( s_loopsViewName ) );

    // initialize model used for view combobox when in load balance mode
    m_loadBalanceViewModel.appendRow( new QStandardItem( s_functionViewName ) );
    m_loadBalanceViewModel.appendRow( new QStandardItem( s_statementsViewName ) );
    m_loadBalanceViewModel.appendRow( new QStandardItem( s_linkedObjectsViewName ) );
    m_loadBalanceViewModel.appendRow( new QStandardItem( s_loopsViewName ) );

    // initialize model used for view combobox when in calltree mode
    m_calltreeViewModel.appendRow( new QStandardItem( s_calltreeModeName) );

    // initialize model used for view combobox when in compare mode
    m_compareViewModel.appendRow( new QStandardItem( s_functionViewName ) );
    m_compareViewModel.appendRow( new QStandardItem( s_statementsViewName ) );
    m_compareViewModel.appendRow( new QStandardItem( s_linkedObjectsViewName ) );
    m_compareViewModel.appendRow( new QStandardItem( s_loopsViewName ) );

    // default mode is metric mode
    m_mode = METRIC_MODE;

    // since default mode is metric model, set the metric mode model as the view selection model
    ui->comboBox_ViewSelection->setModel( &m_metricViewModel );

    // connect performance data manager signals to performance data metric view slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, &PerformanceDataManager::addMetricView, this, &PerformanceDataMetricView::handleInitModel, Qt::QueuedConnection );
        connect( dataMgr, &PerformanceDataManager::addAssociatedMetricView, this, &PerformanceDataMetricView::handleInitModelView, Qt::QueuedConnection );
        connect( dataMgr, &PerformanceDataManager::addMetricViewData, this, &PerformanceDataMetricView::handleAddData, Qt::QueuedConnection );
        connect( dataMgr, &PerformanceDataManager::requestMetricViewComplete, this, &PerformanceDataMetricView::handleRequestMetricViewComplete, Qt::QueuedConnection );
#else
        connect( dataMgr, SIGNAL(addMetricView(QString,QString,QString,QStringList)),
                 this, SLOT(handleInitModel(QString,QString,QString,QStringList)), Qt::QueuedConnection );
        connect( dataMgr, SIGNAL(addAssociatedMetricView(QString,QString,QString,QString,QStringList)),
                 this, SLOT(handleInitModelView(QString,QString,QString,QString,QStringList)), Qt::QueuedConnection );
        connect( dataMgr, SIGNAL(addMetricViewData(QString,QString,QString,QVariantList,QStringList)),
                 this, SLOT(handleAddData(QString,QString,QString,QVariantList,QStringList)), Qt::QueuedConnection );
        connect( dataMgr, SIGNAL(requestMetricViewComplete(QString,QString,QString,double,double)),
                 this, SLOT(handleRequestMetricViewComplete(QString,QString,QString,double,double)), Qt::QueuedConnection );
#endif
    }

    // connect signal/slot for mode selection handling
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( ui->comboBox_ModeSelection, &QComboBox::currentTextChanged, this, &PerformanceDataMetricView::handleViewModeChanged );
#else
    connect( ui->comboBox_ModeSelection, SIGNAL(currentIndexChanged(QString)), this, SLOT(handleViewModeChanged(QString)) );
#endif

    // connect signal/slot for metric view selection handling
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( ui->comboBox_MetricSelection, &QComboBox::currentTextChanged, this, &PerformanceDataMetricView::handleMetricViewChanged );
#else
    connect( ui->comboBox_MetricSelection, SIGNAL(currentIndexChanged(QString)), this, SLOT(handleMetricViewChanged(QString)) );
#endif

    // connect signal/slot for metric view selection handling
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( ui->comboBox_ViewSelection, &QComboBox::currentTextChanged, this, &PerformanceDataMetricView::handleMetricViewChanged );
#else
    connect( ui->comboBox_ViewSelection, SIGNAL(currentIndexChanged(QString)), this, SLOT(handleMetricViewChanged(QString)) );
#endif

    // initially show blank view
    showBlankView();

    // create modify path substitutions dialog
    m_modifyPathsDialog = new ModifyPathSubstitutionsDialog( this );

    // create show device details dialog
    m_deviceDetailsDialog = new ShowDeviceDetailsDialog( this );

    // connect signal/slot for handling adding device information to show device details dialog
    connect( this, SIGNAL(signalAddDevice(quint32,quint32,NameValueList,NameValueList)), m_deviceDetailsDialog, SLOT(handleAddDevice(quint32,quint32,NameValueList,NameValueList)) );

    // re-emit 'signalAddPathSubstitution' signal so it can be handled externally
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( m_modifyPathsDialog, &ModifyPathSubstitutionsDialog::signalAddPathSubstitution, this, &PerformanceDataMetricView::signalAddPathSubstitution );
#else
    connect( m_modifyPathsDialog, SIGNAL(signalAddPathSubstitution(int,QString,QString)), this, SIGNAL(signalAddPathSubstitution(int,QString,QString)) );
#endif
}

/**
 * @brief PerformanceDataMetricView::PerformanceDataMetricView
 *
 * Destroys the PerformanceDataMetricView instance.
 */
PerformanceDataMetricView::~PerformanceDataMetricView()
{
    deleteAllModelsViews();

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

    {
        QMutexLocker guard( &m_mutex );

        QMutableMapIterator< QString, QTreeView* > viter( m_views );
        while ( viter.hasNext() ) {
            viter.next();

            if ( viter.key() != s_noneName ) {
                m_viewStack->removeWidget( viter.value() );
                delete viter.value();
                viter.remove();
            }
        }

        qDeleteAll( m_models );
        m_models.clear();

        qDeleteAll( m_proxyModels );
        m_proxyModels.clear();
    }

    resetUI();
}

/**
 * @brief PerformanceDataMetricView::deleteModelsAndViews
 * @return - boolean value representing whether current view was deleted
 *
 * This method deletes all models and views not pertaining to the 'Details' and 'Trace' mode views.
 */
bool PerformanceDataMetricView::deleteModelsAndViews()
{
    bool currentDeleted( false );

    QMutexLocker guard( &m_mutex );

    QMutableMapIterator< QString, QTreeView* > viter( m_views );
    while ( viter.hasNext() ) {
        viter.next();

        const QString key = viter.key();

        // don't delete trace or details views or the blank view
        if ( key == s_noneName || key.startsWith( s_detailsModeName ) || key.startsWith( s_traceModeName ) )
            continue;

        currentDeleted = ( viter.value() == m_viewStack->currentWidget() );

        m_viewStack->removeWidget( viter.value() );
        delete viter.value();
        viter.remove();

        m_proxyModels.remove( key );
        m_models.remove( key );
    }

    return currentDeleted;
}

/**
 * @brief PerformanceDataMetricView::resetUI
 *
 * This method is called to reset UI state to a clear state.
 */
void PerformanceDataMetricView::resetUI()
{
    ui->comboBox_ModeSelection->blockSignals( true );
    ui->comboBox_ModeSelection->clear();
    ui->comboBox_ModeSelection->blockSignals( false );

    ui->comboBox_MetricSelection->blockSignals( true );
    ui->comboBox_MetricSelection->clear();
    ui->comboBox_MetricSelection->blockSignals( false );

    ui->comboBox_ViewSelection->blockSignals( true );
    ui->comboBox_ViewSelection->setModel( &m_metricViewModel );
    ui->comboBox_ViewSelection->blockSignals( false );

    m_detailsViewModel.clear();
    m_traceViewModel.clear();

    m_deviceDetailsDialog->clearAllDevices();

    m_clusteringCritieriaName = QString();

    // default mode is metric mode
    m_mode = METRIC_MODE;
}

/**
 * @brief PerformanceDataMetricView::setAvailableMetricModes
 * @param modes - set of modes available to the user
 *
 * This method adds an item to the "Mode:" combobox for each desired mode.
 */
void PerformanceDataMetricView::setAvailableMetricModes(const ModeTypes &modes)
{
    ui->comboBox_ModeSelection->blockSignals( true );

    if ( modes.testFlag( METRIC_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_metricModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_metricModeName );

    if ( modes.testFlag( DETAILS_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_detailsModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_detailsModeName );

    if ( modes.testFlag( CALLTREE_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_calltreeModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_calltreeModeName );

    if ( modes.testFlag( COMPARE_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_compareModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_compareModeName );

    if ( modes.testFlag( COMPARE_BY_RANK_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_compareByRankModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_compareByRankModeName );

    if ( modes.testFlag( COMPARE_BY_HOST_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_compareByHostModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_compareByHostModeName );

    if ( modes.testFlag( COMPARE_BY_PROCESS_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_compareByProcessModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_compareByProcessModeName );

    if ( modes.testFlag( LOAD_BALANCE_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_loadBalanceModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_loadBalanceModeName );

    if ( modes.testFlag( TRACE_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( s_traceModeName ) ) )
        ui->comboBox_ModeSelection->addItem( s_traceModeName );

    ui->comboBox_ModeSelection->blockSignals( false );
}

/**
 * @brief PerformanceDataMetricView::getMetricModeName
 * @param mode - the mode type
 *
 * This function returns the name for the specified mode type.
 */
QString PerformanceDataMetricView::getMetricModeName(const PerformanceDataMetricView::ModeType mode)
{
    switch( mode ) {
    case DETAILS_MODE:
        return s_detailsModeName;
    case METRIC_MODE:
        return s_metricModeName;
    case CALLTREE_MODE:
        return s_calltreeModeName;
    case COMPARE_MODE:
        return s_compareModeName;
    case COMPARE_BY_RANK_MODE:
        return s_compareByRankModeName;
    case COMPARE_BY_HOST_MODE:
        return s_compareByHostModeName;
    case COMPARE_BY_PROCESS_MODE:
        return s_compareByProcessModeName;
    case LOAD_BALANCE_MODE:
        return s_loadBalanceModeName;
    case TRACE_MODE:
        return s_traceModeName;
    default:
        return QString();
    }
}

/**
 * @brief PerformanceDataMetricView::clearExistingModelsAndViews
 *
 * Clear existing models for specified metric view as well as view if requested.
 */
void PerformanceDataMetricView::clearExistingModelsAndViews(const QString& metricViewName, bool deleteModel, bool deleteView)
{
    qDebug() << Q_FUNC_INFO << "metricViewName=" << metricViewName << " deleteModel=" << deleteModel << " deleteView=" << deleteView;
    QMutexLocker guard( &m_mutex );

    QSortFilterProxyModel* proxyModel = m_proxyModels.value( metricViewName, Q_NULLPTR );
    if ( proxyModel ) {
        proxyModel->setSourceModel( Q_NULLPTR );
        m_proxyModels.remove( metricViewName );
        delete proxyModel;
    }

    if ( deleteModel ) {
        QStandardItemModel* model = m_models.value( metricViewName, Q_NULLPTR );
        if ( model ) {
            m_models.remove( metricViewName );
            delete model;
        }
    }

    if ( deleteView ) {
        QTreeView* view = m_views.value( metricViewName, Q_NULLPTR );
        if ( view ) {
            m_views.remove( metricViewName );
            delete view;
        }
    }
}

/**
 * @brief PerformanceDataMetricView::handleInitModel
 * @param clusteringCriteriaName - clustering criteria name associated to the metric view
 * @param metricName - name of metric view for which to add data to model
 * @param viewName - name of the view for which to add data to model
 * @param metrics - list of metrics for setting column headers
 *
 * Create and initialize the model and view for the new metric view.
 */
void PerformanceDataMetricView::handleInitModel(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName, const QStringList &metrics)
{
    if ( m_clusteringCritieriaName.isEmpty() )
        m_clusteringCritieriaName = clusteringCriteriaName;

    if ( m_clusteringCritieriaName != clusteringCriteriaName )
        return;

    const QString metricViewName = metricName + "-" + viewName;

    clearExistingModelsAndViews( metricViewName );

    QStandardItemModel* model = new QStandardItemModel( 0, metrics.size(), this );

    if ( Q_NULLPTR == model )
        return;

    model->setHorizontalHeaderLabels( metrics );

    m_models[ metricViewName ] = model;

    if ( s_detailsModeName == metricName  )
        return;

    QSortFilterProxyModel* proxyModel( Q_NULLPTR );

    if ( ! metricName.contains( s_compareModeName ) ) {
        ViewSortFilterProxyModel* viewProxyModel = new ViewSortFilterProxyModel;

        if ( Q_NULLPTR == viewProxyModel )
            return;

        viewProxyModel->setSourceModel( model );
        viewProxyModel->setColumnHeaders( metrics );

        proxyModel = viewProxyModel;
    }
    else {
        proxyModel = new QSortFilterProxyModel;

        if ( Q_NULLPTR == proxyModel )
            return;

        proxyModel->setSourceModel( model );

        for( int i=0; i<metrics.size(); ++i ) {
            proxyModel->setHeaderData( i, Qt::Horizontal, metrics.at(i) );
        }
    }

    QTreeView* view = m_views.value( metricViewName, Q_NULLPTR );
    bool newViewCreated( false );

    if ( Q_NULLPTR == view ) {
        view = new QTreeView;
        // set static view properties: has custom context menu, doesn't allow editing, has no root decoration
        view->setContextMenuPolicy( Qt::CustomContextMenu );
        view->setEditTriggers( QAbstractItemView::NoEditTriggers );
        view->setSelectionBehavior( QTreeView::SelectItems );
        view->setRootIsDecorated( false );
        view->setItemDelegate( new MetricViewDelegate(this) );
        // initially sorting is disabled and enabled once all data has been added to the metric/detail model
        view->setSortingEnabled( false );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( view, &QTreeView::clicked, [=](const QModelIndex& index) {
            processTableViewItemClicked( view, index );
        } );
        connect( view, &QTreeView::customContextMenuRequested, [=](const QPoint& pos) {
            processCustomContextMenuRequested( view, pos );
        } );
#else
        connect( view, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTableViewItemClicked(QModelIndex)) );
        connect( view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(handleCustomContextMenuRequested(QPoint)) );
#endif
        newViewCreated = true;
    }

    if ( Q_NULLPTR == view )
        return;

    {
        QMutexLocker guard( &m_mutex );

        view->setModel( proxyModel );

        // resize header view columns to maximum size required to fit all column contents
        QHeaderView* headerView = view->header();
        if ( headerView ) {
            headerView->setStretchLastSection( true );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            headerView->setSectionResizeMode( QHeaderView::ResizeToContents );
#else
            headerView->setResizeMode( QHeaderView::ResizeToContents );
#endif
        }

        m_proxyModels[ metricViewName ] = proxyModel;

        if ( newViewCreated ) {
            m_viewStack->addWidget( view );
            m_views[ metricViewName ] = view;
        }
    }

    // Make sure metric view not already in combobox
    if ( metricName != s_traceModeName && metricName != s_calltreeModeName &&
         metricName != s_loadBalanceModeName && ! metricName.startsWith( s_compareModeName ) &&
         ui->comboBox_MetricSelection->findText( metricName ) == -1 ) {
        // Add metric view to combobox
        ui->comboBox_MetricSelection->addItem( metricName );
    }

    // initialize this as the current view only when the blank view is active
    if ( m_viewStack->currentWidget() == m_views[ s_noneName ] ) {
        m_viewStack->setCurrentWidget( view );
    }
}

/**
 * @brief PerformanceDataMetricView::handleInitModelView
 * @param clusteringCriteriaName - clustering criteria name associated to the metric view
 * @param metricName - name of metric view for which to add data to model
 * @param viewName - name of the view for which to add data to model
 * @param attachedMetricViewName - name of metric view whose model should also be attached to this new metric view
 * @param metrics - list of metrics for setting column headers
 *
 * Create and initialize the model and view for the new metric view.  Attach the view to the model associated with the attached metric view.
 */
void PerformanceDataMetricView::handleInitModelView(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, const QString &attachedMetricViewName, const QStringList &metrics)
{
    if ( m_clusteringCritieriaName.isEmpty() )
        m_clusteringCritieriaName = clusteringCriteriaName;

    if ( m_clusteringCritieriaName != clusteringCriteriaName )
        return;

    const QString metricViewName = metricName + "-" + viewName;

    clearExistingModelsAndViews( metricViewName, false );

    const QString type = ( viewName == s_allEventsDetailsName ) ? "*" : viewName;

    ViewSortFilterProxyModel* proxyModel = new ViewSortFilterProxyModel( type );

    if ( Q_NULLPTR == proxyModel )
        return;

    QTreeView* view = m_views.value( metricViewName, Q_NULLPTR );
    bool newViewCreated( false );

    if ( Q_NULLPTR == view ) {
        view = new QTreeView;
        // set static view properties: has custom context menu, doesn't allow editing, has no root decoration
        view->setContextMenuPolicy( Qt::CustomContextMenu );
        view->setEditTriggers( QAbstractItemView::NoEditTriggers );
        view->setSelectionBehavior( QTreeView::SelectItems );
        view->setRootIsDecorated( false );
        view->setItemDelegate( new MetricViewDelegate(this) );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( view, &QTreeView::clicked, [=](const QModelIndex& index) {
            processTableViewItemClicked( view, index );
        } );
        connect( view, &QTreeView::customContextMenuRequested, [=](const QPoint& pos) {
            processCustomContextMenuRequested( view, pos );
        } );
#else
        connect( view, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTableViewItemClicked(QModelIndex)) );
        connect( view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(handleCustomContextMenuRequested(QPoint)) );
#endif
        newViewCreated = true;
    }

    if ( Q_NULLPTR == view )
        return;

    {
        QMutexLocker guard( &m_mutex ); 

        QStandardItemModel* model = m_models.value( attachedMetricViewName, Q_NULLPTR );

        if ( Q_NULLPTR == model )
            return;

        proxyModel->setSourceModel( model );
        proxyModel->setColumnHeaders( metrics );

        // the model is set to the proxy model
        view->setModel( proxyModel );
        // initially sorting is disabled and enabled once all data has been added to the metric/detail model
        view->setSortingEnabled( false );

        // resize header view columns to maximum size required to fit all column contents
        QHeaderView* headerView = view->header();
        if ( headerView ) {
            headerView->setStretchLastSection( true );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            headerView->setSectionResizeMode( QHeaderView::ResizeToContents );
#else
            headerView->setResizeMode( QHeaderView::ResizeToContents );
#endif
        }

        m_proxyModels[ metricViewName ] = proxyModel;

        if ( newViewCreated ) {
            m_viewStack->addWidget( view );
            m_views[ metricViewName ] = view;
        }

        // select appropriate model
        QStandardItemModel* viewModel = Q_NULLPTR;
        if ( s_detailsModeName == metricName ) {
            viewModel = &m_detailsViewModel;
        } else if ( s_traceModeName == metricName ) {
            viewModel = &m_traceViewModel;
        }

        // create and insert or append item to appropriate model

        QStandardItem* viewItem = new QStandardItem( viewName );

        if ( viewModel && viewItem && viewModel->findItems( viewName ).isEmpty() ) {
            // insert "All Events" selection as first item; otherwise append the item at the end
            if ( s_allEventsDetailsName == viewName )
                viewModel->insertRow( 0, viewItem );
            else
                viewModel->appendRow( viewItem );
        }
    }
}

/**
 * @brief PerformanceDataMetricView::processTableViewItemClicked
 * @param index - model index of item clicked
 *
 * Handler invoked when item on Metric Table View clicked.
 */
void PerformanceDataMetricView::handleTableViewItemClicked(const QModelIndex& index)
{
    QTreeView* view = qobject_cast< QTreeView* >( sender() );

    processTableViewItemClicked( view, index );
}

/**
 * @brief PerformanceDataMetricView::processTableViewItemClicked
 * @param view - pointer to QTreeView instance
 * @param index - model index of item clicked
 *
 * This method is called from signal handler when item on Metric Table View clicked.
 */
void PerformanceDataMetricView::processTableViewItemClicked(QTreeView* view, const QModelIndex& index)
{
    if ( view ) {
        QAbstractItemModel* model = view->model();
        if ( model ) {
            QString text( model->data( index ).toString() );
            QVariant titleVar( model->headerData( index.column(), Qt::Horizontal ) );
            QString title;
            if ( titleVar.isValid() ) {
                title = titleVar.toString();
            }
            if ( s_functionTitle == title ) {
                QString filename;
                int lineNumber;
                extractFilenameAndLine( text, filename, lineNumber );
                if ( filename.isEmpty() || -1 == lineNumber ) {
                    emit signalClearSourceView();
                }
                else {
                    emit signalDisplaySourceFileLineNumber( filename, lineNumber );
                }
            }
        }
    }
}

/**
 * @brief PerformanceDataMetricView::handleCustomContextMenuRequested
 * @param pos - point on widget were custom context menu was request
 *
 * Handler invoked when custom context menu requested.
 */
void PerformanceDataMetricView::handleCustomContextMenuRequested(const QPoint &pos)
{
    QTreeView* view = qobject_cast< QTreeView* >( sender() );

    processCustomContextMenuRequested( view, pos );
}

/**
 * @brief PerformanceDataMetricView::processCustomContextMenuRequested
 * @param view - pointer to QTreeView instance
 * @param pos - point on widget were custom context menu was request
 *
 * This method is called from signal handler when custom context menu requested.
 */
void PerformanceDataMetricView::processCustomContextMenuRequested(QTreeView* view, const QPoint &pos)
{
    if ( view ) {
        QAbstractItemModel* model = view->model();
        if ( model ) {
            const QModelIndex index = view->indexAt( pos );
            const QVariant columnHeader = view->model()->headerData( index.column(), Qt::Horizontal );
            DetailsMenuTypes menuType( MENU_TYPE_UNDEFINED );
            if ( columnHeader.toString() == s_functionTitle ) {
                menuType = DEFINE_PATH_MAPPINGS;
            }
            else if ( columnHeader.toString() == s_deviceTitle ) {
                menuType = SHOW_DEVICE_DETAILS;
            }
            if ( menuType != MENU_TYPE_UNDEFINED ) {
                showContextMenu( menuType, model->data( index ), view->viewport()->mapToGlobal( pos ) );
            }
        }
    }
}

/**
 * @brief PerformanceDataMetricView::extractFilenameAndLine
 * @param text - text containing defining location information
 * @param filename - the filename obtained from the defining location information
 * @param lineNumber - the line number obtained from the defining location information
 *
 * Extracts the filename and line number from the data in a particular cell of the metric table view.
 */
void PerformanceDataMetricView::extractFilenameAndLine(const QString& text, QString& filename, int& lineNumber)
{
    QString definingLocation;
    lineNumber = -1;
    int startParenIdx = text.lastIndexOf( '(' );
    if ( -1 != startParenIdx ) {
        definingLocation = text.mid( startParenIdx + 1 );
    }
    else {
        definingLocation = text;
    }
    int sepIdx = definingLocation.lastIndexOf( ',' );
    if ( -1 != sepIdx ) {
        filename = definingLocation.left( sepIdx );
        QString lineNumberStr = definingLocation.mid( sepIdx + 1 );
        int endParenIdx = lineNumberStr.lastIndexOf( ')' );
        if ( -1 != endParenIdx ) {
            lineNumberStr.chop( lineNumberStr.length() - endParenIdx );
        }
        lineNumber = lineNumberStr.toInt();
    }
}

/**
 * @brief PerformanceDataMetricView::handleAddData
 * @param clusteringCriteriaName - clustering criteria name associated to the metric view
 * @param metricName - name of metric view for which to add data to model
 * @param viewName - name of the view for which to add data to model
 * @param data - the data to add to the model
 * @param columnHeaders - if present provides the names of the columns for each index in the data
 *
 * Inserts a row into the model of the specified metric view.
 */
void PerformanceDataMetricView::handleAddData(const QString& clusteringCriteriaName, const QString &metricName, const QString& viewName, const QVariantList& data, const QStringList &columnHeaders)
{
    if ( m_clusteringCritieriaName != clusteringCriteriaName || ( ! columnHeaders.isEmpty() && data.size() != columnHeaders.size() ) )
        return;

    const QString metricViewName = metricName + "-" + viewName;

    QMutexLocker guard( &m_mutex );

    QStandardItemModel* model = m_models.value( metricViewName );

    if ( Q_NULLPTR == model )
        return;

    // make a new row for the data
    model->insertRow( 0 );

    if ( columnHeaders.isEmpty() ) {
        // without column headers just insert into the model in sequential order of the data indexes
        for ( int i=0; i<data.size(); ++i ) {
            model->setData( model->index( 0, i ), data.at( i ) );
        }
    }
    else {
        // with column headers need to map the name of the column associated with each data index to
        // the proper column index in the model
        QStringList modelColumnHeaders;

        for (int i=0; i<model->columnCount(); ++i) {
            modelColumnHeaders << model->headerData( i, Qt::Horizontal ).toString();
        }

        int count = 0;
        foreach( const QString& name, columnHeaders ) {
            int index = modelColumnHeaders.indexOf( name );
            if ( index != -1 ) {
                model->setData( model->index( 0, index ), data.at( count ) );
            }
            ++count;
        }
    }
}

/**
 * @brief PerformanceDataMetricView::handleRangeChanged
 * @param clusteringCriteriaName - clustering criteria name associated to the metric view
 * @param metricName - name of metric view having a time range change
 * @param viewName - name of the view having a time range change
 * @param lower - lower value of range to actually view
 * @param upper - upper value of range to actually view
 *
 * After a details view was requested and the model, proxy model and view are created and initialized, this method will handle changes to the time range
 * of data shown in the details view as the user changes the graph timeline.
 */
void PerformanceDataMetricView::handleRangeChanged(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, double lower, double upper)
{
    if ( m_clusteringCritieriaName != clusteringCriteriaName )
        return;

    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QStringLiteral("metric-view-filtering") );
    }

    const QString metricViewName = metricName + "-" + viewName;

    QMutexLocker guard( &m_mutex );

    QSortFilterProxyModel* sortFilterProxyModel = m_proxyModels.value( metricViewName );

    if ( sortFilterProxyModel != Q_NULLPTR ) {
        ViewSortFilterProxyModel* proxyModel = qobject_cast< ViewSortFilterProxyModel* >( sortFilterProxyModel );

        if ( proxyModel != Q_NULLPTR ) {
            proxyModel->setFilterRange( lower, upper );
        }
    }

    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QStringLiteral("metric-view-filtering") );
    }
}

/**
 * @brief PerformanceDataMetricView::handleRequestViewUpdate
 *
 * Handle request to update the current view by emitting the appropriate signal for the mode selected.  The 'clearExistingViews'
 * flag would be set true when the originator of the signal is the ExperimentPanel when a metric view update was requested.
 */
void PerformanceDataMetricView::handleRequestViewUpdate(bool clearExistingViews)
{
    bool currentDeleted( false );

    if ( clearExistingViews ) {
       currentDeleted = deleteModelsAndViews();
    }

    switch( m_mode ) {
    case COMPARE_MODE:
        emit signalRequestCompareView( m_clusteringCritieriaName, s_compareModeName, ui->comboBox_MetricSelection->currentText(), ui->comboBox_ViewSelection->currentText() );
        break;
    case COMPARE_BY_RANK_MODE:
        emit signalRequestCompareView( m_clusteringCritieriaName, s_compareByRankModeName, ui->comboBox_MetricSelection->currentText(), ui->comboBox_ViewSelection->currentText() );
        break;
    case COMPARE_BY_HOST_MODE:
        emit signalRequestCompareView( m_clusteringCritieriaName, s_compareByHostModeName, ui->comboBox_MetricSelection->currentText(), ui->comboBox_ViewSelection->currentText() );
        break;
    case COMPARE_BY_PROCESS_MODE:
        emit signalRequestCompareView( m_clusteringCritieriaName, s_compareByProcessModeName, ui->comboBox_MetricSelection->currentText(), ui->comboBox_ViewSelection->currentText() );
        break;
    case CALLTREE_MODE:
        emit signalRequestCalltreeView( m_clusteringCritieriaName, s_calltreeModeName, s_calltreeModeName );
        break;
    case LOAD_BALANCE_MODE:
        emit signalRequestLoadBalanceView( m_clusteringCritieriaName, ui->comboBox_MetricSelection->currentText(), ui->comboBox_ViewSelection->currentText() );
        break;
    // NOTE: This views are currently pre-built and never should be requested
    case DETAILS_MODE:
        //emit signalRequestDetailView( m_clusteringCritieriaName, ui->comboBox_ViewSelection->currentText() );
        break;
    case TRACE_MODE:
        //emit signalRequestTraceView( m_clusteringCritieriaName, s_traceModeName, s_allEventsDetailsName );
        break;
    default:
        emit signalRequestMetricView( m_clusteringCritieriaName, ui->comboBox_MetricSelection->currentText(), ui->comboBox_ViewSelection->currentText() );
    }

    if ( currentDeleted ) {
        showBlankView();
    }
}

/**
 * @brief PerformanceDataMetricView::handleViewModeChanged
 * @param text - the value of the view mode combobox
 *
 * Handle changes to the view mode.  Sets the appropriate model for the contents of the view selection combobox and
 * enables/disables use of the metric selection combobox depending on the view mode selected.
 */
void PerformanceDataMetricView::handleViewModeChanged(const QString &text)
{
    if ( s_detailsModeName == text ) {
        m_mode = DETAILS_MODE;
        ui->comboBox_ViewSelection->setModel( &m_detailsViewModel );
        ui->comboBox_MetricSelection->setEnabled( false );
    }
    else if ( s_calltreeModeName == text ) {
        m_mode = CALLTREE_MODE;
        ui->comboBox_ViewSelection->setModel( &m_calltreeViewModel );
        ui->comboBox_MetricSelection->setEnabled( false );
    }
    else if ( s_traceModeName == text ) {
        m_mode = TRACE_MODE;
        ui->comboBox_ViewSelection->setModel( &m_traceViewModel );
        ui->comboBox_MetricSelection->setEnabled( false );
    }
    else if ( text.startsWith( s_compareModeName ) ) {
        ui->comboBox_ViewSelection->blockSignals( true );
        ui->comboBox_ViewSelection->setModel( &m_dummyModel );
        ui->comboBox_ViewSelection->blockSignals( false );
        if ( s_compareModeName== text ) {
            m_mode = COMPARE_MODE;
            ui->comboBox_ViewSelection->setModel( &m_compareViewModel );
            ui->comboBox_MetricSelection->setEnabled( true );
        }
        else if ( s_compareByRankModeName == text ) {
            m_mode = COMPARE_BY_RANK_MODE;
            ui->comboBox_ViewSelection->setModel( &m_compareViewModel );
            ui->comboBox_MetricSelection->setEnabled( true );
        }
        else if ( s_compareByHostModeName == text ) {
            m_mode = COMPARE_BY_HOST_MODE;
            ui->comboBox_ViewSelection->setModel( &m_compareViewModel );
            ui->comboBox_MetricSelection->setEnabled( true );
        }
        else if ( s_compareByProcessModeName == text ) {
            m_mode = COMPARE_BY_PROCESS_MODE;
            ui->comboBox_ViewSelection->setModel( &m_compareViewModel );
            ui->comboBox_MetricSelection->setEnabled( true );
        }
    }
    else if ( s_loadBalanceModeName == text ) {
        m_mode = LOAD_BALANCE_MODE;
        ui->comboBox_ViewSelection->setModel( &m_loadBalanceViewModel );
        ui->comboBox_MetricSelection->setEnabled( true );
    }
    else {
        m_mode = METRIC_MODE;
        ui->comboBox_ViewSelection->setModel( &m_metricViewModel );
        ui->comboBox_MetricSelection->setEnabled( true );
    }
}

/**
 * @brief PerformanceDataMetricView::getMetricViewName
 * @return - metric view name formed from internal class state and values of UI comboboxes.
 *
 * Build metric view name string formed from internal class state and values of UI comboboxes.
 */
QString PerformanceDataMetricView::getMetricViewName() const
{
    QString metricViewName;

    if ( DETAILS_MODE == m_mode )
        metricViewName = s_detailsModeName + "-" + ui->comboBox_ViewSelection->currentText();
    else if ( CALLTREE_MODE == m_mode )
        metricViewName = s_calltreeModeName + "-" + s_calltreeModeName;
    else if ( TRACE_MODE == m_mode )
        metricViewName = s_traceModeName + "-" + ui->comboBox_ViewSelection->currentText();
    else if ( COMPARE_MODE == m_mode )
        metricViewName = s_compareModeName + "-" + ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();
    else if ( COMPARE_BY_RANK_MODE == m_mode )
        metricViewName = s_compareByRankModeName + "-" + ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();
    else if ( COMPARE_BY_HOST_MODE == m_mode )
        metricViewName = s_compareByHostModeName + "-" + ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();
    else if ( COMPARE_BY_PROCESS_MODE == m_mode )
        metricViewName = s_compareByProcessModeName + "-" + ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();
    else if ( LOAD_BALANCE_MODE == m_mode )
        metricViewName = s_loadBalanceModeName + "-" + ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();
    else
        metricViewName = ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();

    return metricViewName;
}

/**
 * @brief PerformanceDataMetricView::handleMetricViewChanged
 * @param text - name of metric or view changed
 *
 * Handles user request to switch metric view.
 */
void PerformanceDataMetricView::handleMetricViewChanged(const QString &text)
{
    Q_UNUSED( text )

    if ( ui->comboBox_MetricSelection->currentText().isEmpty() )
        return;

    QTreeView* view( Q_NULLPTR );

    const QString metricViewName = getMetricViewName();

    {
        QMutexLocker guard( &m_mutex );

        view = m_views.value( metricViewName, Q_NULLPTR );
    }

    // if the request view has not been generated yet, then show blank view and request view update
    if ( Q_NULLPTR == view ) {
        // show blank view
        showBlankView();
        // emit signal to have Performance Data Manager process requested metric view
        handleRequestViewUpdate( false );
    }
    else {
        // display existing metric view
        m_viewStack->setCurrentWidget( view );
    }

    emit signalMetricViewChanged( metricViewName );
}

/**
 * @brief PerformanceDataMetricView::handleRequestMetricViewComplete
 * @param clusteringCriteriaName - the name of the cluster criteria
 * @param metricName - name of metric view for which to add data to model
 * @param viewName - name of the view for which to add data to model
 * @param lower - lower value of range to actually view
 * @param upper - upper value of range to actually view
 *
 * Once a handled request metric or detail view signal ('signalRequestMetricView' or 'signalRequestDetailView') is completed,
 * this method will insure the currently selected view is shown.
 */
void PerformanceDataMetricView::handleRequestMetricViewComplete(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, double lower, double upper)
{
    qDebug() << "PerformanceDataMetricView::handleRequestMetricViewComplete: clusteringCriteriaName=" << clusteringCriteriaName << "metricName=" << metricName << "viewName=" << viewName;

    if ( m_clusteringCritieriaName == clusteringCriteriaName || ! metricName.isEmpty() || ! viewName.isEmpty() ) {
        QTreeView* view( Q_NULLPTR );

        const QString metricViewName = metricName + "-" + viewName;

        {
            QMutexLocker guard( &m_mutex );

            view = m_views.value( metricViewName, Q_NULLPTR );

            if ( view != Q_NULLPTR ) {
                // now sorting can be enabled
                if ( s_calltreeModeName == metricName ) {
                    // calltree has fixed order
                    view->setSortingEnabled( false );
                }
                else {
                    view->setSortingEnabled( true );
                    if ( s_detailsModeName == metricName ) {
                        if ( s_allEventsDetailsName == viewName )
                            view->sortByColumn( 1, Qt::AscendingOrder );
                        else
                            view->sortByColumn( 2, Qt::AscendingOrder );
                    }
                    else if ( metricName.startsWith( s_compareModeName ) ) {
                        view->sortByColumn( 1, Qt::DescendingOrder );
                    }
                    else if ( metricName == s_traceModeName ) {
                        if ( s_allEventsDetailsName == viewName )
                            view->sortByColumn( 1, Qt::AscendingOrder );
                        else
                            view->sortByColumn( 0, Qt::AscendingOrder );
                    }
                    else
                        view->sortByColumn( 0, Qt::DescendingOrder );
                }
            }
        }

        if ( Q_NULLPTR != view ) {
            handleRangeChanged( clusteringCriteriaName, metricName, viewName, lower, upper );

            return;
        }
    }

    // if this point is reached then show blank view
    showBlankView();
}

/**
 * @brief PerformanceDataMetricView::showContextMenu
 * @param menuType - the context menu type to build and show
 * @param data - variant data from table item under which the custom context menu was requested
 * @param globalPos - the widget position at which the custom context menu was requested
 *
 * Prepare and show the context menu.
 */
void PerformanceDataMetricView::showContextMenu(const DetailsMenuTypes menuType, const QVariant& data, const QPoint &globalPos)
{
    QMenu menu;

    // setup action for modifying path substitutions
    QAction* action;

    if ( DEFINE_PATH_MAPPINGS == menuType ) {
        action = menu.addAction( tr("&Modify Path Substitutions"), m_modifyPathsDialog, SLOT(exec()) );

        QString filename;
        int lineNumber;

        extractFilenameAndLine( data.toString(), filename, lineNumber );

        QFileInfo fileInfo( filename );

        action->setData( fileInfo.path() );
    }
    else if ( SHOW_DEVICE_DETAILS == menuType ) {
        action = menu.addAction( tr("&Show Device Info"), m_deviceDetailsDialog, SLOT(exec()) );

        action->setData( data );
    }
    else {
        return;
    }

    menu.exec( globalPos );
}


} // GUI
} // ArgoNavis

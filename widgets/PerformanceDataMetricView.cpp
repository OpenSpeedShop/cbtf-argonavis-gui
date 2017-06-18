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

    // initialize model used for view combobox when in details mode
    m_detailsViewModel.appendRow( new QStandardItem( QStringLiteral("All Events") ) );
    m_detailsViewModel.appendRow( new QStandardItem( QStringLiteral("Data Transfers") ) );
    m_detailsViewModel.appendRow( new QStandardItem( QStringLiteral("Kernel Executions") ) );

    // initialize model used for view combobox when in metric mode
    m_metricViewModel.appendRow( new QStandardItem( QStringLiteral("Functions") ) );
    m_metricViewModel.appendRow( new QStandardItem( QStringLiteral("Statements") ) );
    m_metricViewModel.appendRow( new QStandardItem( QStringLiteral("LinkedObjects") ) );
    m_metricViewModel.appendRow( new QStandardItem( QStringLiteral("Loops") ) );

    // initialize model used for view combobox when in calltree mode
    m_calltreeViewModel.appendRow( new QStandardItem( QStringLiteral("CallTree") ) );

    // initialize model used for view combobox when in compare mode
    m_compareViewModel.appendRow( new QStandardItem( QStringLiteral("By Thread") ) );

    // initial mode is details mode
    m_mode = METRIC_MODE;

    // so set view selectiom model to metric view model
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

            if ( viter.key() != "none" ) {
                m_viewStack->removeWidget( viter.value() );
                delete viter.value();
                viter.remove();
            }
        }

        m_models.clear();

        qDeleteAll( m_proxyModels );
        m_proxyModels.clear();
    }

    ui->comboBox_ModeSelection->setCurrentIndex( 0 );

    ui->comboBox_MetricSelection->clear();

    ui->comboBox_ViewSelection->setCurrentIndex( 0 );

    ui->comboBox_ModeSelection->clear();

    m_deviceDetailsDialog->clearAllDevices();

    m_clusteringCritieriaName = QString();
}

/**
 * @brief PerformanceDataMetricView::setAvailableMetricModes
 * @param modes - set of modes available to the user
 *
 * This method adds an item to the "Mode:" combobox for each desired mode.
 */
void PerformanceDataMetricView::setAvailableMetricModes(const ModeTypes &modes)
{
    if ( modes.testFlag( METRIC_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( QStringLiteral("Metric") ) ) )
        ui->comboBox_ModeSelection->addItem( QStringLiteral("Metric") );

    if ( modes.testFlag( DETAILS_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( QStringLiteral("Details") ) ) )
        ui->comboBox_ModeSelection->addItem( QStringLiteral("Details") );

    if ( modes.testFlag( CALLTREE_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( QStringLiteral("CallTree") ) ) )
        ui->comboBox_ModeSelection->addItem( QStringLiteral("CallTree") );

    if ( modes.testFlag( COMPARE_MODE ) && ( -1 == ui->comboBox_ModeSelection->findText( QStringLiteral("Compare") ) ) )
        ui->comboBox_ModeSelection->addItem( QStringLiteral("Compare") );
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

    {
        QMutexLocker guard( &m_mutex );

        QSortFilterProxyModel* proxyModel = m_proxyModels.value( metricViewName, Q_NULLPTR );
        if ( proxyModel ) {
            proxyModel->setSourceModel( Q_NULLPTR );
            m_proxyModels.remove( metricViewName );
            delete proxyModel;
        }

        QStandardItemModel* model = m_models.value( metricViewName, Q_NULLPTR );
        if ( model ) {
            m_models.remove( metricViewName );
            delete model;
        }
    }

    QStandardItemModel* model = new QStandardItemModel( 0, metrics.size(), this );

    if ( Q_NULLPTR == model )
        return;

    model->setHorizontalHeaderLabels( metrics );

    m_models[ metricViewName ] = model;

    if ( QStringLiteral("Details") == metricName  )
        return;

    ViewSortFilterProxyModel* proxyModel = new ViewSortFilterProxyModel;

    if ( Q_NULLPTR == proxyModel )
        return;

    proxyModel->setSourceModel( model );
    proxyModel->setColumnHeaders( metrics );

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
    if ( ui->comboBox_MetricSelection->findText( metricName ) == -1 && metricName != QStringLiteral("CallTree") ) {
        // Add metric view to combobox
        ui->comboBox_MetricSelection->addItem( metricName );
    }

    // initialize this as the current view only when the blank view is active
    if ( m_viewStack->currentWidget() == m_views[ "none" ] ) {
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

    QStandardItemModel* model( Q_NULLPTR );

    {
        QMutexLocker guard( &m_mutex );

        model = m_models.value( attachedMetricViewName, Q_NULLPTR );

        QSortFilterProxyModel* proxyModel = m_proxyModels.value( metricViewName, Q_NULLPTR );
        if ( proxyModel ) {
            proxyModel->setSourceModel( Q_NULLPTR );
            m_proxyModels.remove( metricViewName );
            delete proxyModel;
        }
    }

    const QString type = ( viewName == "All Events" ) ? "*" : viewName.left( viewName.length()-1 );    // remove 's' at end

    ViewSortFilterProxyModel* proxyModel = new ViewSortFilterProxyModel( type );

    if ( Q_NULLPTR == model || Q_NULLPTR == proxyModel )
        return;

    proxyModel->setSourceModel( model );

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

    const QString metricViewName = metricName + "-" + viewName;

    QMutexLocker guard( &m_mutex );

    QSortFilterProxyModel* sortFilterProxyModel = m_proxyModels.value( metricViewName );

    if ( Q_NULLPTR == sortFilterProxyModel )
        return;

    ViewSortFilterProxyModel* proxyModel = qobject_cast< ViewSortFilterProxyModel* >( sortFilterProxyModel );

    if ( Q_NULLPTR == proxyModel )
        return;

    proxyModel->setFilterRange( lower, upper );
}

/**
 * @brief PerformanceDataMetricView::handleRequestViewUpdate
 *
 * Handle request to update the current view by emitting the appropriate signal for the mode selected.
 */
void PerformanceDataMetricView::handleRequestViewUpdate()
{
    if ( DETAILS_MODE == m_mode )
        emit signalRequestDetailView( m_clusteringCritieriaName, ui->comboBox_ViewSelection->currentText() );
    else if ( CALLTREE_MODE == m_mode )
        emit signalRequestMetricView( m_clusteringCritieriaName, QStringLiteral("CallTree"), QStringLiteral("CallTree") );
    else
        emit signalRequestMetricView( m_clusteringCritieriaName, ui->comboBox_MetricSelection->currentText(), ui->comboBox_ViewSelection->currentText() );
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
    if ( QStringLiteral("Details") == text ) {
        m_mode = DETAILS_MODE;
        ui->comboBox_ViewSelection->setModel( &m_detailsViewModel );
        ui->comboBox_MetricSelection->setEnabled( false );
    }
    else if ( QStringLiteral("CallTree") == text ) {
        m_mode = CALLTREE_MODE;
        ui->comboBox_ViewSelection->setModel( &m_calltreeViewModel );
        ui->comboBox_MetricSelection->setEnabled( false );
    }
    else {
        m_mode = METRIC_MODE;
        ui->comboBox_ViewSelection->setModel( &m_metricViewModel );
        ui->comboBox_MetricSelection->setEnabled( true );
    }
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

    QString metricViewName;

    if ( DETAILS_MODE == m_mode )
        metricViewName = QStringLiteral("Details") + "-" + ui->comboBox_ViewSelection->currentText();
    else if ( CALLTREE_MODE == m_mode )
        metricViewName = QStringLiteral("CallTree") + "-" + QStringLiteral("CallTree");
    else
        metricViewName = ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();

    {
        QMutexLocker guard( &m_mutex );

        view = m_views.value( metricViewName, Q_NULLPTR );
    }

    if ( Q_NULLPTR == view ) {
        // show blank view and the emit signal to have Performance Data Manager process requested metric view
        showBlankView();

        if ( DETAILS_MODE == m_mode )
            emit signalRequestDetailView( m_clusteringCritieriaName, ui->comboBox_ViewSelection->currentText() );
        else if ( CALLTREE_MODE == m_mode )
            emit signalRequestMetricView( m_clusteringCritieriaName, QStringLiteral("CallTree"), QStringLiteral("CallTree") );
        else
            emit signalRequestMetricView( m_clusteringCritieriaName, ui->comboBox_MetricSelection->currentText(), ui->comboBox_ViewSelection->currentText() );
    }
    else {
        // display existing metric view
        m_viewStack->setCurrentWidget( view );
    }
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

    if ( m_clusteringCritieriaName != clusteringCriteriaName || metricName.isEmpty() || viewName.isEmpty() )
        return;

    QTreeView* view( Q_NULLPTR );

    const QString metricViewName = metricName + "-" + viewName;

    {
        QMutexLocker guard( &m_mutex );

        view = m_views.value( metricViewName, Q_NULLPTR );

        if ( Q_NULLPTR == view )
            return;

        // now sorting can be enabled
        if ( QStringLiteral("CallTree") == metricName ) {
            // calltree has fixed order
            view->setSortingEnabled( false );
        }
        else {
            view->setSortingEnabled( true );
            if ( QStringLiteral("Details") == metricName ) {
                if ( QStringLiteral("All Events") == viewName )
                    view->sortByColumn( 1, Qt::AscendingOrder );
                else
                    view->sortByColumn( 2, Qt::AscendingOrder );
            }
            else
                view->sortByColumn( 0, Qt::DescendingOrder );
        }
    }

    handleRangeChanged( clusteringCriteriaName, metricName, viewName, lower, upper );

    QString currentMetricViewName;

    if ( DETAILS_MODE == m_mode )
        currentMetricViewName = QStringLiteral("Details") + "-" + ui->comboBox_ViewSelection->currentText();
    else
        currentMetricViewName = ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();

    if ( currentMetricViewName == metricViewName && Q_NULLPTR != view ) {
        m_viewStack->setCurrentWidget( view );
    }
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

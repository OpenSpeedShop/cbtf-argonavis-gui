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

#include "managers/PerformanceDataManager.h"
#include "SourceView/ModifyPathSubstitutionsDialog.h"

#include <QStackedLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QAction>
#include <QFileInfo>


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
        connect( dataMgr, SIGNAL(addMetricView(QString,QString,QStringList)), this, SLOT(handleInitModel(QString,QString,QStringList)) );
        connect( dataMgr, SIGNAL(addMetricViewData(QString,QString,QVariantList)), this, SLOT(handleAddData(QString,QString,QVariantList)) );
#endif
    }

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

        qDeleteAll( m_models );
        m_models.clear();

        qDeleteAll( m_proxyModels );
        m_proxyModels.clear();
    }

    ui->comboBox_MetricSelection->clear();
    ui->comboBox_ViewSelection->clear();
}

/**
 * @brief PerformanceDataMetricView::initModel
 * @param metricName - name of metric view for which to add data to model
 * @param viewName - name of the view for which to add data to model
 * @param metrics - list of metrics for setting column headers
 *
 * Create and initialize the model and view for the new metric view.
 */
void PerformanceDataMetricView::handleInitModel(const QString& metricName, const QString& viewName, const QStringList &metrics)
{
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
    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel;

    if ( Q_NULLPTR == model || Q_NULLPTR == proxyModel )
        return;

    for ( int i=0; i<metrics.size(); ++i ) {
        model->setHeaderData( i, Qt::Horizontal, metrics.at( i ) );
    }

    proxyModel->setFilterKeyColumn( 0 );
    proxyModel->setSourceModel( model );
    proxyModel->sort( 0, Qt::DescendingOrder );

    QTreeView* view = m_views.value( metricViewName, Q_NULLPTR );
    bool newViewCreated( false );

    if ( Q_NULLPTR == view ) {
        view = new QTreeView;
        view->setContextMenuPolicy( Qt::CustomContextMenu );
        view->setEditTriggers( QAbstractItemView::NoEditTriggers );
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
        view->setSortingEnabled( true );
        for ( int i=0; i<metrics.size(); ++i ) {
           view->resizeColumnToContents( i );
        }

        m_models[ metricViewName ] = model;
        m_proxyModels[ metricViewName ] = proxyModel;

        if ( newViewCreated ) {
            m_viewStack->addWidget( view );
            m_views[ metricViewName ] = view;
        }
    }

    // Make sure metric view not already in combobox
    if ( ui->comboBox_MetricSelection->findText( metricName ) == -1 ) {
        // Add metric view to combobox
        ui->comboBox_MetricSelection->addItem( metricName );
    }

    // Make sure view not already in combobox
    if ( ui->comboBox_ViewSelection->findText( viewName ) == -1 ) {
        // Add metric view to combobox
        ui->comboBox_ViewSelection->addItem( viewName );
    }

    // Make the current view
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    ui->comboBox_MetricSelection->setCurrentText( metricName );
#else
    ui->comboBox_MetricSelection->setCurrentIndex( ui->comboBox_MetricSelection->count()-1 );
#endif
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
            QModelIndex index = view->indexAt( pos );
            showContextMenu( model->data( index ), view->viewport()->mapToGlobal( pos ) );
        }
    }
}

/**
 * @brief PerformanceDataMetricView::extractFilenameAndLine
 * @param text - text containing defining location information
 * @param filename - the filename obtained from the defining location information
 * @param lineNumber - the line number obtained from the defining location information
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
 * @param metricName - name of metric view for which to add data to model
 * @param viewName - name of the view for which to add data to model
 * @param data - the data to add to the model
 *
 * Inserts a row into the model of the specified metric view.
 */
void PerformanceDataMetricView::handleAddData(const QString &metricName, const QString& viewName, const QVariantList& data)
{
    const QString metricViewName = metricName + "-" + viewName;

    QMutexLocker guard( &m_mutex );

    QStandardItemModel* model = m_models.value( metricViewName );

    if ( Q_NULLPTR == model )
        return;

    model->insertRow( 0 );

    for ( int i=0; i<data.size(); ++i ) {
        model->setData( model->index( 0, i ), data.at( i ) );
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
    Q_UNUSED(text)

    QTreeView* view( Q_NULLPTR );

    const QString metricViewName = ui->comboBox_MetricSelection->currentText() + "-" + ui->comboBox_ViewSelection->currentText();

    {
        QMutexLocker guard( &m_mutex );

        view = m_views.value( metricViewName, Q_NULLPTR );
    }

    if ( Q_NULLPTR != view ) {
        m_viewStack->setCurrentWidget( view );
    }
}

/**
 * @brief PerformanceDataMetricView::showContextMenu
 * @param data - variant data from table item under which the custom context menu was requested
 * @param globalPos - the widget position at which the custom context menu was requested
 *
 * Prepare and show the context menu.
 */
void PerformanceDataMetricView::showContextMenu(const QVariant& data, const QPoint &globalPos)
{
    QMenu menu;

    // setup action for modifying path substitutions
    QAction* action = menu.addAction( tr("&Modify Path Substitutions"), m_modifyPathsDialog, SLOT(exec()) );

    QString filename;
    int lineNumber;

    extractFilenameAndLine( data.toString(), filename, lineNumber );

    QFileInfo fileInfo( filename );

    action->setData( fileInfo.path() );

    menu.exec( globalPos );
}


} // GUI
} // ArgoNavis

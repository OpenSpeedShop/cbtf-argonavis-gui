/*!
   \file MainWindow.cpp
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

#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "managers/PerformanceDataManager.h"

#include <QFileDialog>
#include <QMetaMethod>
#include <QMessageBox>
#include <QThread>


namespace ArgoNavis { namespace GUI {


/**
 * \brief MainWindow::MainWindow
 * @param parent - specify parent of the MainWindow instance
 *
 * Constructs a widget which is a child of parent.  If parent is 0, the new widget becomes a window.  If parent is another widget,
 * this widget becomes a child window inside parent. The new widget is deleted when its parent is deleted.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
{
    ui->setupUi( this );

#if defined(HAS_CONCURRENT_PROCESSING_VIEW_DEBUG)
    qDebug() << "MainWindow::MainWindow: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif

    setStyleSheet(
                "QSplitter::handle:vertical   { height: 4px; image: url(:/images/vsplitter-handle); background-color: rgba(200, 200, 200, 80); }"
                "QSplitter::handle:horizontal { width:  4px; image: url(:/images/hsplitter-handle); background-color: rgba(200, 200, 200, 80); }"
                );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( ui->actionLoad_OSS_Experiment, &QAction::triggered, this, &MainWindow::loadOpenSsExperiment );
    connect( ui->actionExit, &QAction::triggered, this, &MainWindow::shutdownApplication );
#else
    connect( ui->actionLoad_OSS_Experiment, SIGNAL(triggered(bool)), this, SLOT(loadOpenSsExperiment()) );
    connect( ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(shutdownApplication()) );
#endif

    // connect performance data manager signals to experiment panel slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, &PerformanceDataManager::loadComplete, this, &MainWindow::handleLoadComplete );
        connect( dataMgr, &PerformanceDataManager::addExperiment, ui->widget_ExperimentPanel, &ExperimentPanel::handleAddExperiment );
#else
        connect( dataMgr, SIGNAL(loadComplete()), this, SLOT(handleLoadComplete()) );
        connect( dataMgr, SIGNAL(addExperiment(QString,QString,QVector<QString>,QVector<QString>)),
                 ui->widget_ExperimentPanel, SLOT(handleAddExperiment(QString,QString,QVector<QString>,QVector<QString>)) );
#endif
    }
}

/**
 * \brief MainWindow::~MainWindow
 *
 * Destroys the MainWindow instance.
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief MainWindow::loadOpenSsExperiment
 *
 * Action handler for loading Open|SpeedShop experiments.  Present open file dialog to user so
 * the user can browse the local file system to select the desired Open|SpeedShop experiment
 * (files with .openss extension).  The specified filename is passed to the performance data manager
 * in order to parse the experiment database into local data structures for viewing by the
 * performance data view.  Add the experiment loaded to the unload Open|SpeedShop experiment menu.
 */
void MainWindow::loadOpenSsExperiment()
{
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( ! dataMgr )
        return;

    QString filePath = QApplication::applicationDirPath();
    filePath = QFileDialog::getOpenFileName( this, tr("Open File"), filePath, "*.openss" );
    if ( filePath.isEmpty() )
        return;

    QApplication::setOverrideCursor( Qt::WaitCursor );

    QByteArray normalizedSignature = QMetaObject::normalizedSignature( "asyncLoadCudaViews(QString)" );
    int methodIndex = dataMgr ->metaObject()->indexOfMethod( normalizedSignature );
    if ( -1 != methodIndex ) {
        QMetaMethod method = dataMgr->metaObject()->method( methodIndex );
        method.invoke( dataMgr, Qt::QueuedConnection, Q_ARG( QString, filePath ) );
    }

    addUnloadOpenSsExperimentMenuItem( filePath );
}

/**
 * @brief MainWindow::addUnloadMenuItem
 * @param filePath - filepath of experiment to be added to unload menu
 *
 * Added experiment loaded to the unload menu.
 */
void MainWindow::addUnloadOpenSsExperimentMenuItem(const QString& filePath)
{
    QFileInfo fileInfo( filePath );
    QString expName( fileInfo.fileName() );
    expName.replace( QString(".openss"), QString("") );

    // Add menu item to allow unloading the loaded experiment.
    // NOTE: Unload menu takes ownership of the returned QAction.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    ui->menuUnload_OSS_Experiment->addAction( expName, this, &MainWindow::unloadOpenSsExperiment );
#else
    ui->menuUnload_OSS_Experiment->addAction( expName, this, SLOT(unloadOpenSsExperiment()) );
#endif
    ui->menuUnload_OSS_Experiment->setEnabled( true );
    ui->actionLoad_OSS_Experiment->setDisabled( true );
}

/**
 * @brief MainWindow::unloadOpenSsExperiment
 *
 * Action handler for unloading Open|SpeedShop experiments.  Present confirmation dialog to user so
 * the user can acknowledge unload action.  If the confirmation is acknowledged, then the performance data manager is
 * invoked to remove related data from local data structures and viewing in the
 * performance data view.  Remove the experiment from the unload Open|SpeedShop experiment menu.
 */
void MainWindow::unloadOpenSsExperiment()
{
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( ! action )
        return;

    if ( QMessageBox::Yes == QMessageBox::question( this, tr("Unload Experiment"), tr("Are you sure that you want to unload this experiment?"), QMessageBox::Yes | QMessageBox::No ) ) {

        QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

        QString expName( action->text() );

        ui->widget_MetricPlotView->unloadExperimentDataFromView( expName );

        ui->widget_MetricTableView->deleteAllModelsViews();

        ui->widget_ExperimentPanel->handleRemoveExperiment( expName );

        ui->menuUnload_OSS_Experiment->removeAction( action );
        ui->menuUnload_OSS_Experiment->setDisabled( true );
        ui->actionLoad_OSS_Experiment->setEnabled( true );

        QApplication::restoreOverrideCursor();
    }
}

/**
 * @brief MainWindow::handleLoadComplete
 *
 * Handles special processing after completion of task to load experiment database.
 *
 * Reset override cursor.
 */
void MainWindow::handleLoadComplete()
{
    QApplication::restoreOverrideCursor();
}

/**
 * @brief MainWindow::shutdownApplication
 *
 * Action handler for terminating the application.
 */
void MainWindow::shutdownApplication()
{
    qApp->quit();
}


} // GUI
} // ArgoNavis

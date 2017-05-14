/*!
   \file MetricViewManager.cpp
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2017 Argo Navis Technologies, LLC

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

#include "MetricViewManager.h"

#include "ui_MetricViewManager.h"

#include "CBTF-ArgoNavis-Ext/NameValueDefines.h"

#include <QDebug>


namespace ArgoNavis { namespace GUI {


/**
 * @brief MetricViewManager::MetricViewManager
 * @param parent - the parent widget
 *
 * Constructs an MetricViewManager instance of the given parent.
 */
MetricViewManager::MetricViewManager(QWidget *parent)
    : QStackedWidget( parent )
    , ui( new Ui::MetricViewManager )
{
    ui->setupUi( this );

    qRegisterMetaType<MetricViewTypes>("MetricViewTypes");
}

/**
 * @brief MetricViewManager::~MetricViewManager
 *
 * Destroys the MetricViewManager instance.
 */
MetricViewManager::~MetricViewManager()
{
    delete ui;
}

/**
 * @brief MetricViewManager::handleSwitchView
 * @param viewType - the view type
 *
 * This method set the current widget of the stack widget in accordance with the view type.
 */
void MetricViewManager::handleSwitchView(const MetricViewTypes viewType)
{
    qDebug() << "MetricViewManager::handleSwitchView: viewType=" << ( viewType == CUDA_VIEW ? "CUDA_VIEW" : "CALLTREE_VIEW" );

    if ( CUDA_VIEW == viewType )
        setCurrentWidget( ui->widget_MetricPlotView );
    else if ( CALLTREE_VIEW == viewType )
        setCurrentWidget( ui->widget_CalltreeGraphView );
}

/**
 * @brief MetricViewManager::unloadExperimentDataFromView
 * @param experimentName - the name of the experiment being unloaded from the application
 *
 * This method is invoked when an experiment is unloaded from the application.
 */
void MetricViewManager::unloadExperimentDataFromView(const QString &experimentName)
{
    ui->widget_MetricPlotView->unloadExperimentDataFromView( experimentName );
    ui->widget_CalltreeGraphView->handleDisplayGraphView( QString() );
}


} // GUI
} // ArgoNavis

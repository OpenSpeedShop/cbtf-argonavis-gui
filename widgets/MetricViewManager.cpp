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


namespace ArgoNavis { namespace GUI {


MetricViewManager::MetricViewManager(QWidget *parent)
    : QStackedWidget( parent )
    , ui( new Ui::MetricViewManager )
{
    ui->setupUi( this );

#if 0
    switchView( "Call Tree" );
#endif
}

MetricViewManager::~MetricViewManager()
{
    delete ui;
}

void MetricViewManager::switchView(const QString &viewName)
{
    if ( "CUDA Timeline" == viewName )
        setCurrentWidget( ui->widget_MetricPlotView );
    else if ( "Call Tree" == viewName )
        setCurrentWidget( ui->widget_CalltreeGraphView );
}

void MetricViewManager::unloadExperimentDataFromView(const QString &experimentName)
{
    ui->widget_MetricPlotView->unloadExperimentDataFromView( experimentName );
}


} // GUI
} // ArgoNavis

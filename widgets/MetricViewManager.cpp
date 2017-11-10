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
    , m_defaultView( TIMELINE_VIEW )
{
    ui->setupUi( this );

    qRegisterMetaType<MetricViewTypes>("MetricViewTypes");

    connect( this, SIGNAL(signalTraceItemSelected(QString,double,double,int)),
             ui->widget_MetricTimelineView, SIGNAL(signalTraceItemSelected(QString,double,double,int)) );
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
 * This method sets the default view type.
 */
void MetricViewManager::handleSwitchView(const MetricViewTypes viewType)
{
    const QVector<QString> VIEW_TYPES = { "TIMELINE_VIEW", "GRAPH_VIEW", "CALLTREE_VIEW" };

    qDebug() << "MetricViewManager::handleSwitchView: viewType=" << VIEW_TYPES[ viewType ];

    m_defaultView = viewType;
}

/**
 * @brief MetricViewManager::handleMetricViewChanged
 * @param metricView - currently selected metric table view
 *
 * This handler causes the metric plot view to change to match the currently selected metric table view.
 */
void MetricViewManager::handleMetricViewChanged(const QString &metricView)
{
    const QString modeName = metricView.section( QChar('-'), 0, 0 );

    if ( modeName == QStringLiteral("CallTree") ) {
        setCurrentWidget( ui->widget_CalltreeGraphView );
    }
    else {
        // if not calltree mode active then switch to default view
        if ( TIMELINE_VIEW == m_defaultView && currentWidget() != ui->widget_MetricTimelineView )
            setCurrentWidget( ui->widget_MetricTimelineView );
        else if ( CALLTREE_VIEW == m_defaultView && currentWidget() != ui->widget_CalltreeGraphView )
            setCurrentWidget( ui->widget_CalltreeGraphView );
        else if ( GRAPH_VIEW == m_defaultView && currentWidget() != ui->widget_MetricGraphView )
            setCurrentWidget( ui->widget_MetricGraphView );
    }

    qDebug() << "MetricViewManager::handleMetricViewChanged: currentWidget->objectName()=" << currentWidget()->objectName();
}

/**
 * @brief MetricViewManager::unloadExperimentDataFromView
 * @param experimentName - the name of the experiment being unloaded from the application
 *
 * This method is invoked when an experiment is unloaded from the application.
 */
void MetricViewManager::unloadExperimentDataFromView(const QString &experimentName)
{
    ui->widget_MetricTimelineView->unloadExperimentDataFromView( experimentName );
    ui->widget_MetricGraphView->unloadExperimentDataFromView( experimentName );
    ui->widget_CalltreeGraphView->handleDisplayGraphView( QString() );
}


} // GUI
} // ArgoNavis

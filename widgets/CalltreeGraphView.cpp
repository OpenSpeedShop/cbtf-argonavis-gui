/*!
   \file CalltreeGraphView.cpp
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

#include "CalltreeGraphView.h"

#include <QWheelEvent>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtMath>
#else
#include <qmath.h>
#include <QPair>
#endif

#include "QtGraph/QGraphCanvas.h"
#include "QtGraph/QGraphNode.h"
#include "QtGraph/QGraphEdge.h"

#include "managers/PerformanceDataManager.h"


namespace ArgoNavis { namespace GUI {


CalltreeGraphView::CalltreeGraphView(QWidget *parent)
    : QGraphicsView( parent )
{
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );

    // connect performance data manager signals to performance data metric view slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, &PerformanceDataManager::signalDisplayCalltreeGraph, this, &CalltreeGraphView::handleDisplayGraphView );
#else
        connect( dataMgr, SIGNAL(signalDisplayCalltreeGraph(QString)), this, SLOT(handleDisplayGraphView(QString)) );
#endif
    }
}

void CalltreeGraphView::handleDisplayGraphView(const QString& graph)
{
    QGraphCanvas* g = NULL;

    if ( ! graph.isEmpty() ) {
        // set graph attributes
        QGraphCanvas::NameValueList graphAttributeList;
        graphAttributeList.push_back( qMakePair( QStringLiteral("nodesep"), QStringLiteral("0.4") ) );;

        // set default node attributes
        QGraphCanvas::NameValueList nodeAttributeList;
        nodeAttributeList.push_back( qMakePair( QStringLiteral("style"), QStringLiteral("filled") ) );
        nodeAttributeList.push_back( qMakePair( QStringLiteral("fillcolor"), QStringLiteral("white") ) );

        // set default edge attributes
        QGraphCanvas::NameValueList edgeAttributeList;

        g = new QGraphCanvas( graph.toLocal8Bit().data(), graphAttributeList, nodeAttributeList, edgeAttributeList );

        g->updateLayout();
    }

    setScene( g );
}

void CalltreeGraphView::wheelEvent(QWheelEvent* event)
{
    qreal scaleFactor = qPow( 2.0, event->delta() / 240.0 ); // how fast zooming occurs
    qreal factor = transform().scale( scaleFactor, scaleFactor ).mapRect( QRectF(0, 0, 1, 1) ).width();
    if ( 0.05 < factor && factor < 10.0 ) {
        // limit zoom extents
        scale( scaleFactor, scaleFactor );
    }
}


} // GUI
} // ArgoNavis

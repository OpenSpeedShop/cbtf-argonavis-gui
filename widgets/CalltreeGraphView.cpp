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
#include <QtMath>

#include "QtGraph/QGraphCanvas.h"
#include "QtGraph/QGraphNode.h"
#include "QtGraph/QGraphEdge.h"


namespace ArgoNavis { namespace GUI {


CalltreeGraphView::CalltreeGraphView(const QString &calltreeData, QWidget *parent)
    : QGraphicsView( parent )
{
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );

    // set graph attributes
    QGraphCanvas::NameValueList graphAttributeList;
    graphAttributeList.push_back( qMakePair( QStringLiteral("nodesep"), QStringLiteral("0.4") ) );;

    // set default node attributes
    QGraphCanvas::NameValueList nodeAttributeList;
    nodeAttributeList.push_back( qMakePair( QStringLiteral("style"), QStringLiteral("filled") ) );
    nodeAttributeList.push_back( qMakePair( QStringLiteral("fillcolor"), QStringLiteral("white") ) );

    // set default edge attributes
    QGraphCanvas::NameValueList edgeAttributeList;

    const std::string digraphStr2 =
            "digraph G {"
            "0 [label=\"main\", shape=\"square\", file=\"mutatee.c\", line=\"43\", unit=\"mutatee\"];"
            "1 [label=\"work\", file=\"mutatee.c\", line=\"33\", unit=\"mutatee\"];"
            "2 [label=\"f3\", file=\"mutatee.c\", line=\"24\", unit=\"mutatee\"];"
            "3 [label=\"f2\", file=\"mutatee.c\", line=\"15\", unit=\"mutatee\"];"
            "4 [label=\"f1\", file=\"mutatee.c\", line=\"6\", unit=\"mutatee\"];"
            "0->1  [label=\"0\"];"
            "1->2  [label=\"50\"];"
            "1->3  [label=\"36.3636\"];"
            "1->4  [label=\"13.6364\"];"
            "}";

    QGraphCanvas* g = new QGraphCanvas( digraphStr2.c_str(), graphAttributeList, nodeAttributeList, edgeAttributeList );

    g->updateLayout();

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

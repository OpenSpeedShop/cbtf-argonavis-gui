/*!
   \file OSSMpiFunctionItem.cpp
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

#include "OSSTraceItem.h"


namespace ArgoNavis { namespace GUI {


double OSSTraceItem::s_halfHeight = 0.2;


/**
 * @brief OSSTraceItem::OSSTraceItem
 * @param axisRect - the associated axis rect
 * @param parentPlot - the parent QCustomPlot instance
 *
 * Constructs an OSSTraceItem instance
 */
OSSTraceItem::OSSTraceItem(QCPAxisRect *axisRect, QCustomPlot *parentPlot)
    : OSSEventItem( axisRect, parentPlot )
{
    // set position types to plot coordinates for X axis and viewport ratio for Y axis
    foreach( QCPItemPosition* position, positions() ) {
        position->setAxisRect( axisRect );
        position->setAxes( axisRect->axis( QCPAxis::atBottom ), axisRect->axis( QCPAxis::atLeft ) );
        position->setTypeX( QCPItemPosition::ptPlotCoords );
        position->setTypeY( QCPItemPosition::ptPlotCoords );
    }
}

/**
 * @brief OSSTraceItem::~OSSTraceItem
 *
 * Destroys the OSSTraceItem instance.
 */
OSSTraceItem::~OSSTraceItem()
{

}

/**
 * @brief OSSTraceItem::setData
 * @param functionName - the name of the function
 * @param timeBegin - the start time of the trace item
 * @param timeEnd - the end time of the trace item
 * @param rank
 *
 * Set class state from trace details.  Set the top-left corner to the time begin location on the x axis timeline
 * and at the rank + half of rect height location on the y axis.  Set the bottom-right corner to the time end location
 * on the x axis timeline and at the rank - half of rect height location on the y axis.
 */
void OSSTraceItem::setData(const QString& functionName, double timeBegin, double timeEnd, int rank)
{
    m_functionName = functionName;

    // set brushes and pens for normal (non-selected) appearance
    setBrush( functionName );

    // set brushes and pens for selected appearance (only highlight border)
    setSelectedBrush( brush() );  // same brush as normal appearance

    topLeft->setCoords( timeBegin, (double) rank + s_halfHeight );
    bottomRight->setCoords( timeEnd, (double) rank - s_halfHeight );
}

/**
 * @brief OSSTraceItem::setBrush
 * @param functionName - the name of the function
 */
void OSSTraceItem::setBrush(const QString &functionName)
{
    if ( functionName.contains( QStringLiteral("MPI_Init") ) )
        OSSEventItem::setBrush( QColor( 0x3d, 0xea, 0x63 ) );
    else if ( functionName.contains( QStringLiteral("MPI_Finalize") ) )
        OSSEventItem::setBrush( QColor( 0xca, 0x2b, 0x2b ) );
    else if ( functionName.contains( QStringLiteral("MPI_Barrier") ) )
        OSSEventItem::setBrush( QColor( 0xca, 0x2b, 0x2b ) );
    else if ( functionName.contains( QStringLiteral("MPI_Send") ) )
        OSSEventItem::setBrush( QColor( 0xcc, 0x7d, 0xaf ) );
    else if ( functionName.contains( QStringLiteral("MPI_Recv") ) )
        OSSEventItem::setBrush( QColor( 0xcc, 0x7d, 0xaf ) );
    else
        OSSEventItem::setBrush( QColor( 0x43, 0x8e, 0xc8 ) );
}

/**
 * @brief OSSTraceItem::draw
 * @param painter - the painter used for drawing
 *
 * Re-implements the OSSEventItem::draw method.
 */
void OSSTraceItem::draw(QCPPainter *painter)
{
    const QPointF p1 = topLeft->pixelPoint();
    const QPointF p2 = bottomRight->pixelPoint();

    const QRectF boundingRect = QRectF( p1, p2 ).normalized();

    if ( boundingRect.intersects( clipRect() ) ) { // only draw if bounding rect of rect item is visible in cliprect
        QPainterPath path;
        path.addRoundedRect( boundingRect, 5.0, 5.0 );

        // set pen and brush
        painter->setPen( mainPen() );
        painter->setBrush( mainBrush() );

        // draw the rounded rectangle representing the trace event
        painter->drawPath( path );

        // draw the name of the function inside the rectangle
        if ( ! m_functionName.isEmpty() ) {
            // set font color to white
            painter->setPen( Qt::white );

            // write the text centered within the trace event rectangle
            painter->drawText( boundingRect, Qt::AlignCenter, m_functionName );
        }
    }
}


} // GUI
} // ArgoNavis

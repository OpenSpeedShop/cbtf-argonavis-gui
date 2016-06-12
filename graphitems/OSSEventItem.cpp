/*!
   \file OSSEventItem.cpp
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

#include "OSSEventItem.h"


namespace ArgoNavis { namespace GUI {


/**
 * @brief OSSEventItem::OSSEventItem
 * @param axisRect - the associated axis rect
 * @param parentPlot - the parent QCustomPlot instance
 *
 * Constructs an OSSEventItem instance which is clipped to the specified axis rect.
 * Each position of this QCPItemRect is associated to the specifed axis rect and
 * X and Y axes of the axis rect.  Each position typeX property is set to use plot
 * coordinates and typeY property is set to use axis rect ratio.
 */
OSSEventItem::OSSEventItem(QCPAxisRect* axisRect, QCustomPlot *parentPlot)
    : QCPItemRect( parentPlot )
{
    // event belongs to axis rect
    setClipAxisRect( axisRect );

    // set position types to plot coordinates for X axis and viewport ratio for Y axis
    foreach( QCPItemPosition* position, positions() ) {
        position->setAxisRect( axisRect );
        position->setAxes( axisRect->axis( QCPAxis::atBottom ), axisRect->axis( QCPAxis::atLeft ) );
        position->setTypeX( QCPItemPosition::ptPlotCoords );
        position->setTypeY( QCPItemPosition::ptAxisRectRatio );
    }
}

/**
 * @brief OSSEventItem::~OSSEventItem
 *
 * Destroys the OSSEventItem instance.
 */
OSSEventItem::~OSSEventItem()
{

}

/**
 * @brief OSSEventItem::draw
 * @param painter
 */
void OSSEventItem::draw(QCPPainter *painter)
{
    QPointF p1 = topLeft->pixelPoint();
    QPointF p2 = bottomRight->pixelPoint();

    if ( p1.toPoint() == p2.toPoint() )
        return;

    QRectF rect = QRectF( p1, p2 ).normalized();
    double clipPad = mainPen().widthF();
    QRectF boundingRect = rect.adjusted( -clipPad, -clipPad, clipPad, clipPad );

    QPainterPath path;
    path.addRoundedRect( boundingRect, 5.0, 5.0 );

    if ( boundingRect.intersects( clipRect() ) ) { // only draw if bounding rect of rect item is visible in cliprect
        painter->setPen( mainPen() );
        painter->setBrush( mainBrush() );
        painter->drawPath( path );
    }
}


} // GUI
} // ArgoNavis

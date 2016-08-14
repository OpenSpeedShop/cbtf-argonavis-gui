/*!
   \file OSSEventsSummaryItem.cpp
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

#include "OSSEventsSummaryItem.h"


namespace ArgoNavis { namespace GUI {


/**
 * @brief OSSEventsSummaryItem::OSSEventsSummaryItem
 * @param axisRect - the associated axis rect
 * @param parentPlot - the parent QCustomPlot instance
 *
 * Constructs an OSSEventsSummaryItem instance
 */
OSSEventsSummaryItem::OSSEventsSummaryItem(QCPAxisRect *axisRect, QCustomPlot *parentPlot)
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

    // set brushes and pens for normal (non-selected) appearance
    setBrush( Qt::transparent );
    setPen( Qt::NoPen );

    // set brushes and pens for selected appearance (only highlight border)
    setSelectedBrush( Qt::transparent );
}

/**
 * @brief OSSEventsSummaryItem::~OSSEventsSummaryItem
 *
 * Destroys the OSSEventsSummaryItem instance.
 */
OSSEventsSummaryItem::~OSSEventsSummaryItem()
{

}

/**
 * @brief OSSEventsSummaryItem::setData
 * @param timeBegin - begin time for the summary event item
 * @param timeEnd - end time for the summary event item
 * @param image - image used for graph item
 */
void OSSEventsSummaryItem::setData(double timeBegin, double timeEnd, const QImage &image)
{
    m_image = image;

    //qDebug() << "OSSEventsSummaryItem::setData: timeBegin=" << timeBegin << "timeEnd=" << timeEnd;

    topLeft->setCoords( timeBegin, 0.45 );
    bottomRight->setCoords( timeEnd, 0.55 );
}

/**
 * @brief OSSEventsSummaryItem::draw
 * @param painter
 */
void OSSEventsSummaryItem::draw(QCPPainter *painter)
{
    QPointF p1 = topLeft->pixelPoint();
    QPointF p2 = bottomRight->pixelPoint();

    if ( p1.toPoint() == p2.toPoint() )
        return;

    QRectF boundingRect = QRectF( p1, p2 ).normalized();

    if ( boundingRect.intersects( clipRect() ) ) { // only draw if bounding rect of rect item is visible in cliprect
        painter->setBackground( Qt::transparent );
        painter->drawImage( boundingRect, m_image );
    }
}

} // GUI
} // ArgoNavis

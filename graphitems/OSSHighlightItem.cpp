/*!
   \file OSSHighlightItem.cpp
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

#include "OSSHighlightItem.h"

#include "OSSTraceItem.h"

#include <QTimer>


namespace ArgoNavis { namespace GUI {


const double HIGHLIGHT_AREA_SIZE = 10.0;

const QColor OUTLINE_BRUSH_COLOR = QColor( qRgba( 0xfe, 0xe2, 0x70, 0xff ) );
const QBrush MARKER_BRUSH = QBrush( QColor( qRgba( 0xfa, 0xff, 0xcd, 0x5f ) ), Qt::Dense7Pattern );
const QPen MARKER_PEN = QPen( Qt::black, 1.0, Qt::DotLine );


/**
 * @brief OSSHighlightItem::OSSHighlightItem
 * @param axisRect - the associated axis rect
 * @param parentPlot - the parent QCustomPlot instance
 *
 * Constructs an OSSHighlightItem instance which is clipped to the specified axis rect.
 * Initially the highlight item is hidden from the graph.
 */
OSSHighlightItem::OSSHighlightItem(QCPAxisRect* axisRect, QCustomPlot *parentPlot)
    : QCPItemRect( parentPlot )
    , m_axisRect( axisRect )
    , m_counter( 0 )
{
    // event belongs to axis rect
    setClipAxisRect( axisRect );

    // highlight item is only visible when setData slot called and automatically hides after 5 seconds
    setVisible( false );
}

/**
 * @brief OSSHighlightItem::~OSSHighlightItem
 *
 * Destroys the OSSHighlightItem instance.
 */
OSSHighlightItem::~OSSHighlightItem()
{

}

/**
 * @brief OSSHighlightItem::setData
 * @param annotation - the text annotation
 * @param timeBegin - the starting time of the event (left x-axis value)
 * @param timeEnd - the ending time of the event (right x-axis value)
 * @param rank - the rank of the event
 *
 * Set the top-left corner x axis value to the starting time of the event ('timeBegin') and the bottom-right corner
 * x-axis value to the ending time of the event ('timeEnd').  The y-axis values of the top-left and bottom-right corner
 * depend on the rank value.  If rank is not equal to -1, then this signifies an event plotted over a periodic sample background
 * and the y-axis values are fixed ratio values; otherwise it is an MPI trace event and the rank is the y-axis value.  Once the
 * axis coordinates are set and the item is made visible, the graph is replotted.  A single-shot timer is started to hide the
 * highlight item after an appropriate delay.
 */
void OSSHighlightItem::setData(const QString &annotation, double timeBegin, double timeEnd, int rank)
{
    m_annotation = annotation;

    // set brushes and pens for normal (non-selected) appearance
    setBrush( QBrush( OUTLINE_BRUSH_COLOR ) );

    // set brushes and pens for selected appearance (only highlight border)
    setSelectedBrush( brush() );  // same brush as normal appearance

    // set position types to plot coordinates for X axis and viewport ratio for Y axis
    const QCPItemPosition::PositionType yPosType( -1 == rank ? QCPItemPosition::ptAxisRectRatio : QCPItemPosition::ptPlotCoords );
    foreach( QCPItemPosition* position, positions() ) {
        position->setAxisRect( m_axisRect );
        position->setAxes( m_axisRect->axis( QCPAxis::atBottom ), m_axisRect->axis( QCPAxis::atLeft ) );
        position->setTypeX( QCPItemPosition::ptPlotCoords );
        position->setTypeY( yPosType );
    }

    if ( -1 == rank ) {
        topLeft->setCoords( timeBegin, 0.40 );
        bottomRight->setCoords( timeEnd, 0.60 );
    }
    else {
        topLeft->setCoords( timeBegin, (double) rank + OSSTraceItem::s_halfHeight + 0.1 );
        bottomRight->setCoords( timeEnd, (double) rank - OSSTraceItem::s_halfHeight - 0.1 );
    }

    // make visible
    setVisible( true );

    // increment reference counter for number of times one-shot timer has been fired
    m_counter.ref();

    // start timer to hide highlight in 10 seconds
    QTimer::singleShot( 10000, this, SLOT(handleTimeout()) );

    // force parent graph to refresh
    parentPlot()->replot();
}

/**
 * @brief OSSHighlightItem::draw
 * @param painter - the painter used for drawing
 *
 * Reimplements the QCPItemRect::draw method.
 */
void OSSHighlightItem::draw(QCPPainter *painter)
{
#if defined(HAS_QCUSTOMPLOT_V2)
    QPointF p1 = topLeft->pixelPosition();
    QPointF p2 = bottomRight->pixelPosition();
#else
    QPointF p1 = topLeft->pixelPoint();
    QPointF p2 = bottomRight->pixelPoint();
#endif

    if ( p1.toPoint() == p2.toPoint() )
        return;

    // compute the rounded rectangle drawn in the normal event item draw method

    QRectF rect = QRectF( p1, p2 ).normalized();

    double clipPad = mainPen().widthF();
    QRectF boundingRect = rect.adjusted( -clipPad, -clipPad, clipPad, clipPad );

    // compute the painter path item producing an outline around the normal event item

    QPainterPath innerPath;
    innerPath.addRoundedRect( boundingRect, 5.0, 5.0 );

    QRectF outerBoundingRect = rect.adjusted( -HIGHLIGHT_AREA_SIZE, -HIGHLIGHT_AREA_SIZE, HIGHLIGHT_AREA_SIZE, HIGHLIGHT_AREA_SIZE );

    QPainterPath outerPath;
    outerPath.addRoundedRect( outerBoundingRect, 5.0, 5.0 );

    QPainterPath path = outerPath.intersected( innerPath );

    // compute the painter path item producing a marker the stretches across the entire y-axis
    // so that in dense traces this item can be more clearly indicated in the graph timeline

    QRectF outerRect = QRectF( QPointF( p1.x(), 0.0 ), QPointF( p2.x(), m_axisRect->bottom() ) );

    QPainterPath markerPath;
    markerPath.addRect( outerRect );

    if ( boundingRect.intersects( clipRect() ) ) { // only draw if bounding rect of rect item is visible in cliprect
        // draw the marker painter path in a lighter color
        painter->setPen( MARKER_PEN );
        painter->setBrush( MARKER_BRUSH );
        painter->drawPath( markerPath );
        // draw the outline painter path to highlight a particular event item enclosed within
        painter->setPen( mainPen() );
        painter->setBrush( mainBrush() );
        painter->drawPath( path );
    }
}

/**
 * @brief OSSHighlightItem::handleTimeout
 *
 * The one-shot timer timeout signal handler to hide the highlight item.
 * NOTE:  Need to account for the possibility that multiple one-shot timers had
 * been started.  The 'm_counter' variable is a reference counter for the number
 * of one-shot timers started.
 */
void OSSHighlightItem::handleTimeout()
{
    if ( ! m_counter.deref() ) {
        // hide the highlight item
        setVisible( false );

        // force parent graph to refresh
        parentPlot()->replot();
    }
}

} // GUI
} // ArgoNavis

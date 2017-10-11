/*!
   \file OSSPeriodicSampleItem.cpp
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

#include "OSSPeriodicSampleItem.h"


namespace ArgoNavis { namespace GUI {


/**
 * @brief OSSPeriodicSampleItem::OSSPeriodicSampleItem
 * @param axisRect - the associated axis rect
 * @param parentPlot - the parent QCustomPlot instance
 *
 * Constructs an OSSPeriodicSampleItem instance which is clipped to the specified axis rect.
 * Each position of this QCPItemRect is associated to the specifed axis rect and
 * X and Y axes of the axis rect.  Each position typeX and typeY property is set to use plot
 * coordinates.
 */
OSSPeriodicSampleItem::OSSPeriodicSampleItem(QCPAxisRect* axisRect, QCustomPlot *parentPlot)
    : QCPItemRect( parentPlot )
    , m_time_begin( 0.0 )
    , m_time_end( 0.0 )
    , m_count( 0.0 )
{
    // set brushes and pens for normal (non-selected) appearance
    setBrush( QColor(140, 140, 140, 80) );
    setPen( Qt::NoPen );

    // set brushes and pens for selected appearance (only highlight border)
    setSelectedBrush( brush() );  // same brush as normal appearance

    // sample belongs to axis rect
    setClipAxisRect( axisRect );

    // set position types to plot coordinates for X axis and viewport ratio for Y axis
    foreach( QCPItemPosition* position, positions() ) {
        position->setAxisRect( axisRect );
        position->setAxes( axisRect->axis( QCPAxis::atBottom ), axisRect->axis( QCPAxis::atLeft ) );
        position->setTypeX( QCPItemPosition::ptPlotCoords );
        position->setTypeY( QCPItemPosition::ptPlotCoords );
    }
}

/**
 * @brief OSSPeriodicSampleItem::~OSSPeriodicSampleItem
 *
 * Destroys the OSSPeriodicSampleItem instance.
 */
OSSPeriodicSampleItem::~OSSPeriodicSampleItem()
{

}

/**
 * @brief OSSPeriodicSampleItem::setData
 * @param time_begin - the relative begin time from experiment start
 * @param time_end - the relative end time from experiment start
 * @param count - the counter value (an average) for this period
 *
 * Set class state from periodic sample data.  Set the top-left corner to the time begin location on the x axis timeline
 * and at the counter value location on the y axis.  Set the bottom-right corner to the time end location on the x axis timeline
 * and at the zero counter value location on the y axis.
 */
void OSSPeriodicSampleItem::setData(const double &time_begin, const double &time_end, const double &count)
{
    m_time_begin = time_begin;
    m_time_end = time_end;
    m_count = count;

    //qDebug() << "OSSPeriodicSample::setData: timeBegin=" << m_time_begin << "timeEnd=" << m_time_end << "count=" << count;

    topLeft->setCoords( m_time_begin, m_count );
    bottomRight->setCoords( m_time_end, 0.0 );
}


} // GUI
} // ArgoNavis

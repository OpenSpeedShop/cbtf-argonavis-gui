/*!
   \file CustomPlot.cpp
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

#include "CustomPlot.h"


/**
 * @brief CustomPlot::CustomPlot
 * @param parent - specify parent of the CustomPlot instance
 *
 * Constructs a CustomPlot which is a child of parent.  If parent is 0, the new CustomPlot becomes a window.  If parent is another widget,
 * this CustomPlot becomes a child window inside parent. The new CustomPlot is deleted when its parent is deleted.
 */
CustomPlot::CustomPlot(QWidget *parent)
    : QCustomPlot( parent )
{
    connect( this, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(handleMousePress(QMouseEvent*)) );
}

/**
 * @brief CustomPlot::resizeEvent
 * @param event - the pointer to the resize event
 *
 * Event handler for a resize of the CustomPlot widget. Overrides QCustomPlot::resizeEvent() to check
 * new width and height and only call QCustomPlot::resizeEvent() if both are greater than zero.
 */
void CustomPlot::resizeEvent(QResizeEvent *event)
{
    QSize newSize = event->size();

    // invoke QCustomPlot::resizeEvent method only if width and height not zero
    if ( newSize.width() > 0 && newSize.height() > 0 )
        QCustomPlot::resizeEvent( event );
}

void CustomPlot::handleMousePress(QMouseEvent *event)
{
    QCPAxisRect* axisrect( axisRect() );

    if ( ! axisrect || event->pos().y() < axisrect->bottom() )
        return;

    QCPAxis* xAxis = axisrect->axis( QCPAxis::atBottom );
    if ( xAxis ) {
        int index = qRound( xAxis->pixelToCoord( event->pos().x() ) );
        QVector<QString> tickLabelVector( xAxis->tickVectorLabels() );
        if ( index >= 0 && index < tickLabelVector.size() ) {
            emit signalXAxisTickLabelSelected( tickLabelVector.at( index ) );
        }
    }
}

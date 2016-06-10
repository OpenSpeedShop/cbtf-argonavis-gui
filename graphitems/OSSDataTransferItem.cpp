/*!
   \file OSSDataTransferItem.cpp
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

#include "OSSDataTransferItem.h"

#include <ArgoNavis/CUDA/stringify.hpp>

#include <QVariant>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QJsonDocument>
#include <QJsonObject>
#endif

namespace ArgoNavis { namespace GUI {


/**
 * @brief OSSDataTransferItem::OSSDataTransferItem
 * @param axisRect - the associated axis rect
 * @param parentPlot - the parent QCustomPlot instance
 *
 * Constructs an OSSDataTransferItem instance
 */
OSSDataTransferItem::OSSDataTransferItem(QCPAxisRect* axisRect, QCustomPlot *parentPlot)
    : OSSEventItem( axisRect, parentPlot )
{
    // set brushes and pens for normal (non-selected) appearance
    setBrush( QColor(255, 0, 0, 64) );
    setPen( QPen( brush(), 0.0 ) ); // cosmetic pen

    // set brushes and pens for selected appearance (only highlight border)
    setSelectedBrush( brush() );  // same brush as normal appearance
}

/**
 * @brief OSSDataTransferItem::~OSSDataTransferItem
 *
 * Destroys the OSSDataTransferItem instance.
 */
OSSDataTransferItem::~OSSDataTransferItem()
{

}

/**
 * @brief OSSDataTransferItem::setData
 * @param time_origin - the experiment time origin
 * @param details - a reference to the data transfer record from the experiment database
 *
 * Set class state from data transfer details.  Set the top-left corner to the time begin location on the x axis timeline
 * and at the 0.45 ratio position of the y axis.  Set the bottom-right corner to the time end location on the x axis timeline
 * and at the 0.55 ratio position of the y axis.
 */
void OSSDataTransferItem::setData(const Base::Time &time_origin, const CUDA::DataTransfer &details)
{
    m_timeOrigin = time_origin;
    m_details = details;

    double timeBegin = static_cast<uint64_t>(details.time_begin - time_origin) / 1000000.0;
#if defined(USE_DISCRETE_SAMPLES)
    timeBegin /= 10.0;
#endif
    double timeEnd = static_cast<uint64_t>(details.time_end - time_origin) / 1000000.0;
#if defined(USE_DISCRETE_SAMPLES)
    timeEnd /= 10.0;
#endif
    //qDebug() << "OSSDataTransferItem::setData: timeBegin=" << timeBegin << "timeEnd=" << timeEnd;

    topLeft->setCoords( timeBegin, 0.45 );
    bottomRight->setCoords( timeEnd, 0.55 );
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
/**
 * @brief operator <<
 * @param os - the input QTextStream object
 * @param item - the referenced OSSDataTransferItem object to dump as JSON
 * @return - the reference of the input QTextStream object
 *
 * Friend function of OSSDataTransferItem class.  Writes state of OSSDataTransferItem as UTF-8 encoded JSON document.
 */
QTextStream& operator<<(QTextStream& os, const OSSDataTransferItem& item)
{
    CUDA::DataTransfer details = item.m_details;

    QVariantMap keyValueMap;

    keyValueMap[ "call_site" ] = QVariant::fromValue( static_cast<uint64_t>( details.call_site ) );
    keyValueMap[ "device" ] = QVariant::fromValue( static_cast<uint64_t>( details.device ) );
    keyValueMap[ "Time" ] = QVariant::fromValue( static_cast<uint64_t>(details.time - item.m_timeOrigin) );
    keyValueMap[ "TimeBegin" ] = QVariant::fromValue( static_cast<uint64_t>(details.time_begin - item.m_timeOrigin) );
    keyValueMap[ "TimeEnd" ] = QVariant::fromValue( static_cast<uint64_t>(details.time_end - item.m_timeOrigin) );
    keyValueMap[ "Size" ] = QVariant::fromValue( details.size );
    keyValueMap[ "Kind" ] = QString::fromStdString( CUDA::stringify(details.kind) );
    keyValueMap[ "SourceKind" ] = QString::fromStdString( CUDA::stringify(details.source_kind) );
    keyValueMap[ "DestinationKind" ] = QString::fromStdString( CUDA::stringify(details.destination_kind) );
    keyValueMap[ "Asynchronous" ] = details.asynchronous;

    QJsonObject root = QJsonObject::fromVariantMap( keyValueMap );

    QJsonDocument doc( root );

    os << doc.toJson();

    return os;
}
#endif


} // GUI
} // ArgoNavis

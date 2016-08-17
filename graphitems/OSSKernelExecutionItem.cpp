/*!
   \file OSSKernelExecutionItem.cpp
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

#include "OSSKernelExecutionItem.h"

#include <ArgoNavis/CUDA/stringify.hpp>

#include <QVariant>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
#include <QJsonDocument>
#include <QJsonObject>
#endif


namespace ArgoNavis { namespace GUI {


/**
 * @brief OSSKernelExecutionItem::OSSKernelExecutionItem
 * @param axisRect - the associated axis rect
 * @param parentPlot - the parent QCustomPlot instance
 *
 * Constructs an OSSKernelExecutionItem instance
 */
OSSKernelExecutionItem::OSSKernelExecutionItem(QCPAxisRect* axisRect, QCustomPlot* parentPlot)
    : OSSEventItem( axisRect, parentPlot )
{
    // set brushes and pens for normal (non-selected) appearance
    setBrush( QColor( 0xaf, 0xdb, 0xaf ) );

    // set brushes and pens for selected appearance (only highlight border)
    setSelectedBrush( brush() );  // same brush as normal appearance
}

/**
 * @brief OSSKernelExecutionItem::~OSSKernelExecutionItem
 *
 * Destroys the OSSKernelExecutionItem instance.
 */
OSSKernelExecutionItem::~OSSKernelExecutionItem()
{

}

/**
 * @brief OSSKernelExecutionItem::setData
 * @param time_origin - the experiment time origin
 * @param details - a reference to the kernel execution record from the experiment database
 *
 * Set class state from kernel execution details.  Set the top-left corner to the time begin location on the x axis timeline
 * and at the 0.45 ratio position of the y axis.  Set the bottom-right corner to the time end location on the x axis timeline
 * and at the 0.55 ratio position of the y axis.
 */
void OSSKernelExecutionItem::setData(const Base::Time &time_origin, const CUDA::KernelExecution &details)
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
    //qDebug() << "OSSKernelExecutionItem::setData: timeBegin=" << timeBegin << "timeEnd=" << timeEnd;

    topLeft->setCoords( timeBegin, 0.45 );
    bottomRight->setCoords( timeEnd, 0.55 );
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
/**
 * @brief demangle
 * @param mangled - the mangled function name
 * @return - the demanged variant of the input mangled function name
 *
 * Called by friend function of OSSKernelExecutionItem class.  Demangle a C++ function name.
 */
QString demangle(const std::string& mangled)
{
    std::string demangled = mangled;

    int status = -2;
    char* tmp = abi::__cxa_demangle(mangled.c_str(), NULL, NULL, &status);

    if (tmp != NULL) {
        if (status == 0) {
            demangled = std::string(tmp);
        }
        free(tmp);
    }

    return QString::fromStdString( demangled );
}

/**
 * @brief xyz
 * @param value - three element vector to dump as json object having X, Y and Z values.
 * @return - the json object built from the input vector
 *
 * Called by friend function of OSSKernelExecutionItem class.  Create a JSON object with x, y, and z attributes from a Vector3u value.
 */
QJsonObject xyz(const CUDA::Vector3u& value)
{
    return QJsonObject(
        { { "x", QJsonValue::fromVariant( value.get<0>() ) },
          { "y", QJsonValue::fromVariant( value.get<1>() ) },
          { "z", QJsonValue::fromVariant( value.get<2>() ) }
        } );
}

/**
 * @brief operator <<
 * @param os - the input QTextStream object
 * @param item - the referenced OSSKernelExecutionItem object to dump as JSON
 * @return - the reference of the input QTextStream object
 *
 * Friend function of OSSKernelExecutionItem class.  Writes state of OSSKernelExecutionItem as UTF-8 encoded JSON document.
 */
QTextStream& operator<<(QTextStream& os, const OSSKernelExecutionItem& item)
{
    CUDA::KernelExecution details = item.m_details;

    QVariantMap keyValueMap;

    keyValueMap[ "call_site" ] = QVariant::fromValue( static_cast<uint64_t>( details.call_site ) );
    keyValueMap[ "device" ] = QVariant::fromValue( static_cast<uint64_t>( details.device ) );
    keyValueMap[ "Time" ] = QVariant::fromValue( static_cast<uint64_t>(details.time - item.m_timeOrigin) );
    keyValueMap[ "TimeBegin" ] = QVariant::fromValue( static_cast<uint64_t>(details.time_begin - item.m_timeOrigin) );
    keyValueMap[ "TimeEnd" ] = QVariant::fromValue( static_cast<uint64_t>(details.time_end - item.m_timeOrigin) );
    keyValueMap[ "Function" ] = demangle( details.function );
    keyValueMap[ "Grid" ] = xyz( details.grid );
    keyValueMap[ "Block" ] = xyz( details.block );
    keyValueMap[ "CachePreference" ] = QString::fromStdString( CUDA::stringify(details.cache_preference) );
    keyValueMap[ "RegistersPerThread" ] = QVariant::fromValue( details.registers_per_thread );
    keyValueMap[ "StaticSharedMemory" ] = QVariant::fromValue( details.static_shared_memory );
    keyValueMap[ "DynamicSharedMemory" ] = QVariant::fromValue( details.dynamic_shared_memory );
    keyValueMap[ "LocalMemory" ] = QVariant::fromValue( details.local_memory );

    QJsonObject root = QJsonObject::fromVariantMap( keyValueMap );

    QJsonDocument doc( root );

    os << doc.toJson();

    return os;
}
#endif


} // GUI
} // ArgoNavis

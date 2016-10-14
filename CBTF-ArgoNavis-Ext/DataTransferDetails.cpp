/*!
   \file ModifyPathSubstitutionDialog.h
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

#include "DataTransferDetails.h"

#include "common/openss-gui-config.h"

#include <ArgoNavis/CUDA/stringify.hpp>

#include <QString>


namespace ArgoNavis { namespace CUDA {


/**
 * @brief getDataTransferDetailsHeaderList
 * @return - CUDA data transfer details view column headers
 *
 * This function returns the CUDA details view column headers for the data transfers view.
 */
QStringList getDataTransferDetailsHeaderList()
{
    return QStringList() << QStringLiteral("Time")
                         << QStringLiteral("Time Begin")
                         << QStringLiteral("Time End")
                         << QStringLiteral("Call Site")
                         << QStringLiteral("Device")
                         << QStringLiteral("Size")
                         << QStringLiteral("Kind")
                         << QStringLiteral("Source Kind")
                         << QStringLiteral("Destination Kind")
                         << QStringLiteral("Asynchronous");
}

/**
 * @brief getDataTransferDetailsDataList
 * @param time_origin - the time origin for the experiment database
 * @param details - the data transfer details
 * @return - CUDA data transfer details view column data
 *
 * This function returns the CUDA details view column data for the data transfers view.
 */
QVariantList getDataTransferDetailsDataList(const Base::Time &time_origin, const DataTransfer& details)
{
    return QVariantList() << QVariant::fromValue( static_cast<uint64_t>( details.time - time_origin )  / 1000000.0 )
                          << QVariant::fromValue( static_cast<uint64_t>( details.time_begin - time_origin )  / 1000000.0 )
                          << QVariant::fromValue( static_cast<uint64_t>( details.time_end - time_origin )  / 1000000.0 )
                          << QVariant::fromValue( static_cast<uint64_t>( details.call_site ) )
                          << QVariant::fromValue( static_cast<uint64_t>( details.device ) )
                          << QString::fromStdString( CUDA::stringify( ByteCount( details.size ) ) )
                          << QString::fromStdString( CUDA::stringify(details.kind ) )
                          << QString::fromStdString( CUDA::stringify(details.source_kind ) )
                          << QString::fromStdString( CUDA::stringify(details.destination_kind ) )
                          << QVariant::fromValue( details.asynchronous ).toString();
}

} // CUDA
} // ArgoNavis

/*!
   \file KernelExecutionDetails.cpp
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

#include "KernelExecutionDetails.h"

#include "common/openss-gui-config.h"

#include <ArgoNavis/CUDA/stringify.hpp>

#include <QString>


namespace ArgoNavis { namespace CUDA {


/**
 * @brief getKernelExecutionDetailsHeaderList
 * @return - CUDA kernel execution details view column headers
 *
 * This function returns the CUDA details view column headers for the kernel executions view.
 */
QStringList getKernelExecutionDetailsHeaderList()
{
    return QStringList() << QStringLiteral("Type")
                         << QStringLiteral("Time (ms)")
                         << QStringLiteral("Time Begin (ms)")
                         << QStringLiteral("Time End (ms)")
                         << QStringLiteral("Duration (ms)")
                         << QStringLiteral("Call Site")
                         << QStringLiteral("Device")
                         << QStringLiteral("Function")
                         << QStringLiteral("Grid X")
                         << QStringLiteral("Grid Y")
                         << QStringLiteral("Grid Z")
                         << QStringLiteral("Block X")
                         << QStringLiteral("Block Y")
                         << QStringLiteral("Block Z")
                         << QStringLiteral("Registers Per Thread")
                         << QStringLiteral("Cache Preference")
                         << QStringLiteral("Static Shared Memory")
                         << QStringLiteral("Dynamic Shared Memory")
                         << QStringLiteral("Local Memory");
}

/**
 * @brief getKernelExecutionDetailsDataList
 * @param time_origin - the time origin for the experiment database
 * @param details - the kernel execution details
 * @return - CUDA kernel execution details view column data
 *
 * This function returns the CUDA details view column data for the kernel executions view.
 */
QVariantList getKernelExecutionDetailsDataList(const ArgoNavis::Base::Time &time_origin, const KernelExecution &details)
{
    double timeBegin( static_cast<uint64_t>( details.time_begin - time_origin ) / 1000000.0 );
    double timeEnd( static_cast<uint64_t>( details.time_end - time_origin ) / 1000000.0 );
    double duration( timeEnd - timeBegin );
    return QVariantList() << QStringLiteral("Kernel Execution")
                          << QVariant::fromValue( static_cast<uint64_t>( details.time - time_origin ) / 1000000.0 )
                          << QVariant::fromValue( timeBegin )
                          << QVariant::fromValue( timeEnd )
                          << QVariant::fromValue( duration )
                          << QVariant::fromValue( static_cast<uint64_t>( details.call_site ) )
                          << QVariant::fromValue( static_cast<uint64_t>( details.device ) )
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
                          << QString::fromStdString( CUDA::stringify( FunctionName( details.function ) ) )
#else
                          << QString( CUDA::stringify( FunctionName( details.function ) ).c_str() )
#endif
                          << QVariant::fromValue( static_cast<uint32_t>( details.grid.get<0>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.grid.get<1>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.grid.get<2>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.block.get<0>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.block.get<1>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.block.get<2>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.registers_per_thread ) )
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
                          << QString::fromStdString( CUDA::stringify( details.cache_preference ) )
                          << QString::fromStdString( CUDA::stringify( ByteCount( details.static_shared_memory ) ) )
                          << QString::fromStdString( CUDA::stringify( ByteCount( details.dynamic_shared_memory ) ) )
                          << QString::fromStdString( CUDA::stringify( ByteCount( details.local_memory ) ) );
#else
                          << QString( CUDA::stringify( details.cache_preference ).c_str() )
                          << QString( CUDA::stringify( ByteCount( details.static_shared_memory ) ).c_str() )
                          << QString( CUDA::stringify( ByteCount( details.dynamic_shared_memory ) ).c_str() )
                          << QString( CUDA::stringify( ByteCount( details.local_memory ) ).c_str() );
#endif
}


} // CUDA
} // ArgoNavis

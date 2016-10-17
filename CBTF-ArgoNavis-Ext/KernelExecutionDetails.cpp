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
    return QStringList() << QStringLiteral("Time")
                         << QStringLiteral("Time Begin")
                         << QStringLiteral("Time End")
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
    return QVariantList() << QVariant::fromValue( static_cast<uint64_t>( details.time - time_origin ) / 1000000.0 )
                          << QVariant::fromValue( static_cast<uint64_t>( details.time_begin - time_origin ) / 1000000.0 )
                          << QVariant::fromValue( static_cast<uint64_t>( details.time_end - time_origin ) / 1000000.0 )
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

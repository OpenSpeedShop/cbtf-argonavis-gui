#include "KernelExecutionDetails.h"

#include "common/openss-gui-config.h"

#include <ArgoNavis/CUDA/stringify.hpp>

#include <QString>


namespace ArgoNavis { namespace CUDA {


/**
 * @brief demangle
 * @param mangled
 * @return
 * Demangle a C++ function name.
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
                         << QStringLiteral("Cache Preference")
                         << QStringLiteral("Registers Per Thread")
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
                          << QVariant::fromValue( demangle( details.function ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.grid.get<0>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.grid.get<1>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.grid.get<2>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.block.get<0>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.block.get<1>() ) )
                          << QVariant::fromValue( static_cast<uint32_t>( details.block.get<2>() ) )
                          << QString::fromStdString( CUDA::stringify(details.cache_preference) )
                          << QVariant::fromValue( details.registers_per_thread )
                          << QVariant::fromValue( details.static_shared_memory )
                          << QVariant::fromValue( details.dynamic_shared_memory )
                          << QVariant::fromValue( details.local_memory );
}


} // CUDA
} // ArgoNavis


#include "CudaDeviceHelper.h"

namespace ArgoNavis { namespace CUDA {

bool operator==(const CUDA::Device& lhs, const CUDA::Device& rhs)
{
    return ( ( boost::get<0>(lhs.compute_capability) == boost::get<0>(rhs.compute_capability) ) &&
             ( boost::get<1>(lhs.compute_capability) == boost::get<1>(rhs.compute_capability) ) &&
             ( lhs.constant_memory_size == rhs.constant_memory_size ) &&
             ( lhs.core_clock_rate == rhs.core_clock_rate ) &&
             ( lhs.global_memory_bandwidth == rhs.global_memory_bandwidth ) &&
             ( lhs.global_memory_size == rhs.global_memory_size ) &&
             ( lhs.l2_cache_size == rhs.l2_cache_size ) &&
             ( boost::get<0>(lhs.max_block) == boost::get<0>(rhs.max_block) ) &&
             ( boost::get<1>(lhs.max_block) == boost::get<1>(rhs.max_block) ) &&
             ( boost::get<2>(lhs.max_block) == boost::get<2>(rhs.max_block) ) &&
             ( lhs.max_blocks_per_multiprocessor == rhs.max_blocks_per_multiprocessor ) &&
             ( boost::get<0>(lhs.max_grid) == boost::get<0>(rhs.max_grid) ) &&
             ( boost::get<1>(lhs.max_grid) == boost::get<1>(rhs.max_grid) ) &&
             ( boost::get<2>(lhs.max_grid) == boost::get<2>(rhs.max_grid) ) &&
             ( lhs.max_ipc == rhs.max_ipc ) &&
             ( lhs.max_registers_per_block == rhs.max_registers_per_block ) &&
             ( lhs.max_shared_memory_per_block == rhs.max_shared_memory_per_block ) &&
             ( lhs.max_threads_per_block == rhs.max_threads_per_block ) &&
             ( lhs.max_warps_per_multiprocessor == rhs.max_warps_per_multiprocessor ) &&
             ( lhs.memcpy_engines == rhs.memcpy_engines ) &&
             ( lhs.multiprocessors == rhs.multiprocessors ) &&
             ( lhs.name == rhs.name ) &&
             ( lhs.threads_per_warp == rhs.threads_per_warp ) );
}

} // CUDA
} // ArgoNavis

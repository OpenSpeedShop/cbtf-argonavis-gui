#ifndef CUDADEVICEHELPER_H
#define CUDADEVICEHELPER_H

#include <ArgoNavis/CUDA/Device.hpp>

namespace ArgoNavis { namespace CUDA {

bool operator==(const CUDA::Device& lhs, const CUDA::Device& rhs);

} // CUDA
} // ArgoNavis

#endif // CUDADEVICEHELPER_H

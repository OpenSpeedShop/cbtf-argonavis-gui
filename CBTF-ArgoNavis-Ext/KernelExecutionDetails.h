#ifndef KERNELEXECUTIONDETAILS_H
#define KERNELEXECUTIONDETAILS_H

#include <ArgoNavis/Base/Time.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>

#include <QStringList>
#include <QVariant>


namespace ArgoNavis { namespace CUDA {


    QStringList getKernelExecutionDetailsHeaderList();
    QVariantList getKernelExecutionDetailsDataList(const Base::Time& time_origin, const KernelExecution& details);


} // CUDA
} // ArgoNavis

#endif // KERNELEXECUTIONDETAILS_H

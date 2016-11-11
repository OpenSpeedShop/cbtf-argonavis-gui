#ifndef CLUSTERNAMEBUILDER_H
#define CLUSTERNAMEBUILDER_H

#include <QString>

#include "ArgoNavis/Base/ThreadName.hpp"
#include "Thread.hxx"

namespace ArgoNavis { namespace CUDA {

    const QString getUniqueClusterName(const Base::ThreadName& thread);

    const QString getUniqueClusterName(const OpenSpeedShop::Framework::Thread& thread);

} // CUDA
} // ArgoNavis

#endif // CLUSTERNAMEBUILDER_H

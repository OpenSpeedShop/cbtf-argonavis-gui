/*!
   \file ClusterNameBuilder.cpp
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

#include <CBTF-ArgoNavis-Ext/ClusterNameBuilder.h>

#include <QMap>

namespace ArgoNavis { namespace CUDA {

QMap< uint64_t, uint16_t> m_tidmap;

const QString getUniqueClusterName(const Base::ThreadName& thread)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QString clusterName = QString::fromStdString( thread.host() );
#else
    QString clusterName = QString( thread.host().c_str() );
#endif

#ifdef HAS_STRIP_DOMAIN_NAME
    int index = clusterName.indexOf( '.' );
    if ( index > 0 )
        clusterName = clusterName.left( index );
#endif

    // append MPI rank (if any)
    boost::optional<boost::uint32_t> mpiRank = thread.mpi_rank();
    if ( mpiRank ) {
        clusterName += ( "-rank-" + QString::number(mpiRank.get()) );
    }
    else {
        boost::uint64_t pid = thread.pid();
        clusterName += ( "-pid-" + QString::number(pid) );
    }

    boost::optional<uint64_t> tidval = thread.tid();
    if ( tidval ) {
        uint64_t tid = tidval.get();
        uint16_t val;
        if ( m_tidmap.contains( tid ) ) {
            val = m_tidmap[tid];
        }
        else {
            val = m_tidmap[tid] = m_tidmap.size();
        }
        clusterName += ( "-tid-" + QString::number(val) );
    }

    return clusterName;
}

const QString getUniqueClusterName(const OpenSpeedShop::Framework::Thread& thread)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QString clusterName = QString::fromStdString( thread.getHost() );
#else
    QString clusterName = QString( thread.getHost().c_str() );
#endif

#ifdef HAS_STRIP_DOMAIN_NAME
    int index = clusterName.indexOf( '.' );
    if ( index > 0 )
        clusterName = clusterName.left( index );
#endif

    // append MPI rank (if any)
    std::pair<bool, int> mpiRank = thread.getMPIRank();
    if ( mpiRank.first ) {
        clusterName += ( "-rank-" + QString::number(mpiRank.second) );
    }
    else {
        pid_t pid = thread.getProcessId();
        clusterName += ( "-pid-" + QString::number(pid) );
    }

    std::pair< bool, pthread_t> tidval = thread.getPosixThreadId();
    if ( tidval.first ) {
        uint64_t tid = tidval.second;
        uint16_t val;
        if ( m_tidmap.contains( tid ) ) {
            val = m_tidmap[tid];
        }
        else {
            val = m_tidmap[tid] = m_tidmap.size();
        }
        clusterName += ( "-tid-" + QString::number(val) );
    }

    return clusterName;
}

} // CUDA
} // ArgoNavis

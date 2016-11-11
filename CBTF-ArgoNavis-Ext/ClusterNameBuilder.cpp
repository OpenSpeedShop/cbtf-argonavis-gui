
#include <CBTF-ArgoNavis-Ext/ClusterNameBuilder.h>

namespace ArgoNavis { namespace CUDA {

const QString getUniqueClusterName(const Base::ThreadName &thread)
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

    return clusterName;
}

const QString getUniqueClusterName(const OpenSpeedShop::Framework::Thread &thread)
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

    return clusterName;
}

} // CUDA
} // ArgoNavis

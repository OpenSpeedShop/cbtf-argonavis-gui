#include "ViewSortFilterProxyModel.h"

#include <QDateTime>

#include <limits>


namespace ArgoNavis { namespace GUI {


/**
 * @brief ViewSortFilterProxyModel::ViewSortFilterProxyModel
 * @param parent - the parent object
 */
ViewSortFilterProxyModel::ViewSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel( parent )
    , m_lower( std::numeric_limits<double>::min() )
    , m_upper( std::numeric_limits<double>::max() )
{
    setDynamicSortFilter( true );
}

/**
 * @brief ViewSortFilterProxyModel::~ViewSortFilterProxyModel
 */
ViewSortFilterProxyModel::~ViewSortFilterProxyModel()
{

}

/**
 * @brief ViewSortFilterProxyModel::setFilterRange
 * @param lower
 * @param upper
 */
void ViewSortFilterProxyModel::setFilterRange(double lower, double upper)
{
    m_lower = lower;
    m_upper = upper;

    invalidateFilter();
}

/**
 * @brief ViewSortFilterProxyModel::filterAcceptsRow
 * @param source_row
 * @param source_parent
 * @return
 */
bool ViewSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index1 = sourceModel()->index( source_row, 1, source_parent );  // "Time Begin" index
    QVariant value1 = sourceModel()->data( index1 );
    QModelIndex index2 = sourceModel()->index( source_row, 2, source_parent );  // "Time End" index
    QVariant value2 = sourceModel()->data( index2 );

    if ( QVariant::Double == value1.type() && QVariant::Double == value2.type()  ) {
        double timeValue1 = value1.toDouble();  // "Time Begin" value
        double timeValue2 = value2.toDouble();  // "Time End" value
        // keep row if either "Time Begin" or "Time End" value within range defined by 'm_lower' .. ' 'm_upper'
        return ( timeValue1 >= m_lower && timeValue1 <= m_upper || timeValue2 >= m_lower && timeValue2 <= m_upper );
    }

    return true;
}


} // GUI
} // ArgoNavis

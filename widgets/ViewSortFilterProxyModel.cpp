#include "ViewSortFilterProxyModel.h"

#include <QDateTime>

#include <limits>


namespace ArgoNavis { namespace GUI {


/**
 * @brief ViewSortFilterProxyModel::ViewSortFilterProxyModel
 * @param parent - the parent object
 *
 * Constructs a ViewSortFilterProxyModel instance with the given parent.  This is a subclass of QSortFilterProxyModel
 * providing a specialized sorting filter model for the details model/view implementation.
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
 *
 * Destroys this ViewSortFilterProxyModel instance.
 */
ViewSortFilterProxyModel::~ViewSortFilterProxyModel()
{

}

/**
 * @brief ViewSortFilterProxyModel::setFilterRange
 * @param lower - the lower value of the filter range
 * @param upper - the upper value of the filter range
 *
 * This method updates the filter range and updates the proxy model by invalidating the filter.
 */
void ViewSortFilterProxyModel::setFilterRange(double lower, double upper)
{
    m_lower = lower;
    m_upper = upper;

    invalidateFilter();
}

/**
 * @brief ViewSortFilterProxyModel::filterAcceptsRow
 * @param source_row - the row of the item in the model
 * @param source_parent - the model indec of the parent of the item in the model
 * @return - the filter value indicating whether the item is to be accepted (true) or not (false)
 *
 * This method implements a filter to keep the specified row if either "Time Begin" value within range defined by ['m_lower' .. 'm_upper'] OR
 * "Time Begin" is before 'm_lower' but "Time End" is equal to or greater than 'm_lower'.
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
        // keep row if either "Time Begin" value within range defined by ['m_lower' .. 'm_upper'] OR
        // "Time Begin" is before 'm_lower' but "Time End" is equal to or greater than 'm_lower'
        return ( timeValue1 >= m_lower && timeValue1 <= m_upper || timeValue1 < m_lower && timeValue2 >= m_lower );
    }

    return true;
}


} // GUI
} // ArgoNavis

/*!
   \file ViewSortFilterProxyModel.cpp
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2017 Argo Navis Technologies, LLC

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

#include "ViewSortFilterProxyModel.h"

#include <QDateTime>
#include <QStringList>

#include <limits>


namespace ArgoNavis { namespace GUI {


/**
 * @brief ViewSortFilterProxyModel::ViewSortFilterProxyModel
 * @param parent - the parent object
 *
 * Constructs a ViewSortFilterProxyModel instance with the given parent.  This is a subclass of QSortFilterProxyModel
 * providing a specialized sorting filter model for the details model/view implementation.
 */
ViewSortFilterProxyModel::ViewSortFilterProxyModel(const QString &type, QObject *parent)
    : DefaultSortFilterProxyModel( type, parent )
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
 * @brief ViewSortFilterProxyModel::setColumnHeaders
 * @param columnHeaders - the subset of columns to be included in the proxy model
 *
 * This method defines a subset of columns from the source model to use in the proxy model.
 */
void ViewSortFilterProxyModel::setColumnHeaders(const QStringList &columnHeaders)
{
    QAbstractItemModel* model = sourceModel();

    QStringList modelColumnHeaders;

    for (int i=0; i<model->columnCount(); ++i) {
        modelColumnHeaders << sourceModel()->headerData( i, Qt::Horizontal ).toString();
    }

    foreach (const QString& columnName, columnHeaders) {
        int index = modelColumnHeaders.indexOf( columnName );
        if ( -1 != index || "*" == m_type ) {
            m_columns << index;
        }
    }
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
 * @param source_parent - the model index of the parent of the item in the model
 * @return - the filter value indicating whether the item is to be accepted (true) or not (false)
 *
 * The method reimplements QSortFilterProxyModel::filterAcceptsRow.
 *
 * The method implements a filter to keep the specified row if either "Time Begin" value within range defined by ['m_lower' .. 'm_upper'] OR
 * "Time Begin" is before 'm_lower' but "Time End" is equal to or greater than 'm_lower'.
 */
bool ViewSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    bool result( DefaultSortFilterProxyModel::filterAcceptsRow( source_row, source_parent ) );

    QModelIndex indexType = sourceModel()->index( source_row, 0, source_parent );       // "Type" index
    QVariant typeVar = sourceModel()->data( indexType );
    QModelIndex indexTimeBegin = sourceModel()->index( source_row, 2, source_parent );  // "Time Begin" index
    QVariant timeBeginVar = sourceModel()->data( indexTimeBegin );
    QModelIndex indexTimeEnd = sourceModel()->index( source_row, 3, source_parent );    // "Time End" index
    QVariant timeEndVar = sourceModel()->data( indexTimeEnd );

    if ( QVariant::String == typeVar.type() && QVariant::Double == timeBeginVar.type() && QVariant::Double == timeEndVar.type()  ) {
        const QString type = typeVar.toString();           // "Type" value
        const double timeBegin = timeBeginVar.toDouble();  // "Time Begin" value
        const double timeEnd = timeEndVar.toDouble();      // "Time End" value
        // keep row if either "Time Begin" value within range defined by ['m_lower' .. 'm_upper'] OR
        // "Time Begin" is before 'm_lower' but "Time End" is equal to or greater than 'm_lower'
        result &= ( ( m_type == "*" || type.startsWith( m_type ) ) &&
                  ( ( timeBegin >= m_lower && timeBegin <= m_upper ) || ( timeBegin < m_lower && timeEnd >= m_lower ) ) );
    }

    return result;
}

/**
 * @brief ViewSortFilterProxyModel::filterAcceptsColumn
 * @param source_column - column number of source column
 * @param source_parent - parent of source column
 * @return - return whether the specified column should be included in the proxy model
 *
 * The method reimplements QSortFilterProxyModel::filterAcceptsColumn.
 */
bool ViewSortFilterProxyModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED( source_parent )

    return ( ( m_type == "*" || source_column != 0  ) && m_columns.contains( source_column ) );
}


} // GUI
} // ArgoNavis

/*!
   \file DefaultSortFilterProxyModel.cpp
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

#include "DefaultSortFilterProxyModel.h"


namespace ArgoNavis { namespace GUI {


/**
 * @brief DefaultSortFilterProxyModel::DefaultSortFilterProxyModel
 * @param parent - the parent object
 *
 * Constructs a DefaultSortFilterProxyModel instance with the given parent.  This is a subclass of QSortFilterProxyModel
 * providing a specialized sorting filter model for the details model/view implementation.
 */
DefaultSortFilterProxyModel::DefaultSortFilterProxyModel(const QString &type, QObject* parent)
    : QSortFilterProxyModel( parent )
    , m_type( type )
{

}

/**
 * @brief DefaultSortFilterProxyModel::~DefaultSortFilterProxyModel
 *
 * Destroys this DefaultSortFilterProxyModel instance.
 */
void DefaultSortFilterProxyModel::setFilterCriteria(const QList<QPair<QString, QString> > &criteria)
{
    QAbstractItemModel* model = sourceModel();

    QStringList modelColumnHeaders;

    for (int i=0; i<model->columnCount(); ++i) {
        modelColumnHeaders << sourceModel()->headerData( i, Qt::Horizontal ).toString();
    }

    for ( QList< QPair<QString, QString> >::const_iterator iter = criteria.begin(); iter != criteria.end(); iter++ ) {
        const QPair<QString, QString>& item( *iter );
        const QString filterColumnName = item.first;
        if ( modelColumnHeaders.contains( filterColumnName ) ) {
            QRegExp regularExpression( item.second );
            if ( regularExpression.isValid() ) {
                m_filterCriteria << qMakePair( modelColumnHeaders.indexOf( filterColumnName ), regularExpression );
            }
        }
    }
}

/**
 * @brief DefaultSortFilterProxyModel::filterAcceptsRow
 * @param source_row - the row of the item in the model
 * @param source_parent - the model index of the parent of the item in the model
 * @return - the filter value indicating whether the item is to be accepted (true) or not (false)
 *
 * The method reimplements QSortFilterProxyModel::filterAcceptsRow.
 *
 * This method implements a filter to keep the specified row only if the filter criteria matches the source row contents.
 * If no filter criteria was specified by DefaultSortFilterProxyModel::setFilterCriteria(), then the source row is accepted.
 */
bool DefaultSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    bool keepRow( true );

    for ( QList< QPair<int, QRegExp> >::const_iterator iter = m_filterCriteria.begin(); iter != m_filterCriteria.end(); iter++ ) {
        const QPair<int, QRegExp>& item( *iter );

        const QModelIndex sourceModelIndex = sourceModel()->index( source_row, item.first, source_parent );
        const QString sourceColumnContents = sourceModel()->data( sourceModelIndex ).toString();

        const QRegExp filterRegExp = item.second;
        keepRow &= filterRegExp.exactMatch( sourceColumnContents );
    }

    return keepRow;
}


} // GUI
} // ArgoNavis

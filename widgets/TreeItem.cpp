/*!
   \file TreeItem.cpp
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

#include "TreeItem.h"


namespace ArgoNavis { namespace GUI {


/*!
 * \brief TreeItem::TreeItem
 * \param data - column data for the new tree item
 * \param parent - the parent tree item
 *
 * Create a new tree item in the tree hierarchy having the parent and column data specified.
 */
TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent, bool checkable, bool checked, bool enabled)
    : QObject( parent )
    , m_itemData( data )
    , m_checked( checked )
    , m_checkable( checkable )
    , m_enabled( enabled )
{

}

/*!
 * \brief TreeItem::~TreeItem
 *
 * Destroys the tree item.
 */
TreeItem::~TreeItem()
{
    qDeleteAll( m_childItems );
}

/*!
 * \brief TreeItem::appendChild
 * \param item - attach a new child tree item
 *
 * Add a new tree item as a child in the tree hierarchy.
 */
void TreeItem::appendChild(TreeItem *item)
{
    m_childItems.append( item );
}

/**
 * @brief TreeItem::removeChild
 * @param child - remove the child tree item
 */
void TreeItem::removeChild(TreeItem *child)
{
    m_childItems.removeAll( child );
    delete child;
}

/*!
 * \brief TreeItem::child
 * \param row - the child index (row) desired
 * \return - the child tree item at specified index (row)
 *
 * Returns the child tree item at the specified index (row).
 */
TreeItem *TreeItem::child(int row)
{
    return m_childItems.value( row );
}

/*!
 * \brief TreeItem::childCount
 * \return - the number of children
 *
 * Returns the number of children for this tree item.
 */
int TreeItem::childCount() const
{
    return m_childItems.count();
}

/*!
 * \brief TreeItem::row
 * \return - the row index for this tree item in the parent tree item.  Return zero if this tree item is the root.
 */
int TreeItem::row() const
{
    TreeItem* parentItem = qobject_cast< TreeItem* >( parent() );
    if ( parentItem )
        return parentItem->m_childItems.indexOf( const_cast< TreeItem* >( this ) );

    return 0;
}

/*!
 * \brief TreeItem::columnCount
 * \return - the number of columns for this tree item
 *
 * Return the number of columns for this tree item.
 */
int TreeItem::columnCount() const
{
    return m_itemData.count();
}

/*!
 * \brief TreeItem::data
 * \param column - the column index
 * \return - the data at the specified column index
 *
 * Returns the data at the specified column index.
 */
QVariant TreeItem::data(int column) const
{
    return m_itemData.value( column );
}

/*!
 * \brief TreeItem::parentItem
 * \return - the parent tree item.  Return Q_NULLPTR is this is the root.
 */
TreeItem *TreeItem::parentItem()
{
    TreeItem* parentItem = qobject_cast< TreeItem* >( parent() );
    return parentItem;
}

/**
 * @brief TreeItem::setData
 * @param column - the column index
 * @param data - the column data
 *
 * Set the data for the column at the specified index.
 */
void TreeItem::setData(int column, const QVariant &data)
{
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
    if ( column > m_itemData.size() )
        m_itemData.reserve( column );
#endif
    m_itemData.insert( column, data );
}


} // GUI
} // ArgoNavis

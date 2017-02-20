/*!
   \file TreeModel.cpp
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

#include "TreeModel.h"

#include "TreeItem.h"


namespace ArgoNavis { namespace GUI {


/*!
 * \brief TreeModel::TreeModel
 * \param root - create a tree model having the specified root tree item
 * \param parent - the QObject parent that owns this tree model instance (if any)
 *
 * Constructs a new tree model and returns a pointer to the object.
 */
TreeModel::TreeModel(TreeItem* root, QObject *parent)
    : QAbstractItemModel( parent )
{
    rootItem = root;
}

/*!
 * \brief TreeModel::~TreeModel
 *
 * Destroys the tree model instance.
 */
TreeModel::~TreeModel()
{
    delete rootItem;
}

/*!
 * \brief TreeModel::index
 * \param row - the row index
 * \param column - the column index
 * \param parent - the parent model index
 * \return - the model index of the item specified
 *
 * Overrides QAbstractItemModel::index method.
 *
 * Returns the index of the item in the model specified by the given row, column and parent index.
 */
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if ( ! hasIndex(row, column, parent) )
        return QModelIndex();

    TreeItem *parentItem = getItem( parent );

    TreeItem *childItem = parentItem->child( row );
    if ( childItem )
        return createIndex( row, column, childItem );
    else
        return QModelIndex();
}

/*!
 * \brief TreeModel::parent
 * \param index - the index referencing the model item of interest
 * \return - the parent of the referenced model item
 *
 * Overrides QAbstractItemModel::parent method.
 *
 * Returns the parent of the model item with the given index.
 * If the item has no parent, an invalid QModelIndex is returned.
 */
QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if ( ! index.isValid() )
        return QModelIndex();

    TreeItem *childItem = static_cast< TreeItem* >( index.internalPointer() );

    if ( childItem == NULL )
        return QModelIndex();

    TreeItem *parentItem = childItem->parentItem();

    if ( parentItem == NULL || parentItem == rootItem )
        return QModelIndex();

    return createIndex( parentItem->row(), 0, parentItem );
}

/*!
 * \brief TreeModel::rowCount
 * \param parent - the model index of the model item of interest
 * \return - the number of rows under the referenced parent model item
 *
 * Overrides QAbstractItemModel::rowCount method.
 *
 * Returns the number of rows under the given parent
 * When the parent is valid it means that rowCount is returning the number of children of parent.
 */
int TreeModel::rowCount(const QModelIndex &parent) const
{
    if ( parent.column() > 0 )
        return 0;

    TreeItem *parentItem = getItem( parent );

    return parentItem->childCount();
}

/*!
 * \brief TreeModel::columnCount
 * \param parent - the model index of the model item of interest
 * \return - the number of columns for the children of the given parent
 *
 * Overrides QAbstractItemModel::columnCount method.
 *
 * Returns the number of columns for the children of the given parent.
 */
int TreeModel::columnCount(const QModelIndex &parent) const
{
    return getItem( parent )->columnCount();
}

/**
 * @brief TreeModel::setData
 * @param index - the index location to set
 * @param value - the value to set
 * @param role - the role type
 * @return - returns true if successful; otherwise returns false
 *
 * Overrides QAbstractItemModel::setData method.
 *
 * Sets the role data for the item at index to value.  The dataChanged() signal is emitted if the data was successfully set.
 */
bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    TreeItem *item = getItem( index );

    if ( role == CheckableRole ) {
        bool checked = value.toBool();
        item->setCheckable( checked );
        emit dataChanged( index, index );
        return true;
    }

    if ( role == Qt::CheckStateRole ) {
        bool checked = value.toBool();
        item->setChecked( checked );
        emit dataChanged( index, index );
        // modify children similarily
        for ( int i=0; i<item->childCount(); i++ ) {
            QModelIndex childIndex = TreeModel::index( i, 0, index );
            TreeItem* child = getItem( childIndex );
            // cause change and emit dataChanged signal only when really changed
            if ( child && child->isChecked() != checked ) {
                child->setChecked( checked );
                emit dataChanged( childIndex, childIndex );
            }
        }
        // modify parent to unchecked if one of it's children changes to unchecked
        if ( ! checked ) {
            QModelIndex parentIndex = index.parent();
            TreeItem *parentItem = getItem( parentIndex );
            // cause change and emit dataChanged signal only when really changed
            if ( parentItem && parentItem->isChecked() != checked ) {
                parentItem->setChecked( checked );
                emit dataChanged( parentIndex, parentIndex );
            }
        }
        return true;
    }

    if ( role != Qt::EditRole )
        return false;

    item->setData( index.column(), value );

    emit dataChanged( index, index );

    return true;
}

/**
 * @brief TreeModel::insertRows
 * @param row - the row index to insert the new rows before
 * @param count - the number of rows to insert
 * @param parent - the parent item of inserted rows
 * @return - whether row insertion was successful
 *
 * Overrides QAbstractItemModel::insertRows method.
 *
 * Inserts count rows into the model before the given row.
 * Items in the new row will be children of the item represented by the parent model index.
 */
bool TreeModel::insertRows(int row, int count, const QModelIndex &parent)
{
    QModelIndex index = createIndex( row, 0 );

    if ( ! index.isValid() )
        return false;

    QList< QVariant > data;

    TreeItem* parentItem = getItem( parent );

    beginInsertRows( parent, row, row+count-1 );
    for ( int i=0; i<count; ++i ) {
        TreeItem* item = new TreeItem( data, parentItem );
        parentItem->appendChild( item );
    }
    endInsertRows();

    return true;
}

/**
 * @brief TreeModel::removeRows
 * @param row - the row index to begin removing
 * @param count - the number of rows to remove
 * @param parent - the parent item of the removed rows
 * @return - whether row removals successful
 *
 * Removes count rows starting with the given row under parent parent from the model.
 * Returns true if the rows were successfully removed; otherwise returns false.
 */
bool TreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    int lastRow( row+count-1 );

    TreeItem* parentItem;
    if ( ! parent.isValid() )
        parentItem = rootItem;
    else
        parentItem = static_cast< TreeItem* >( parent.internalPointer() );

    beginRemoveRows( parent, row, lastRow );
    for ( int i=row; i<=lastRow; ++i ) {
        TreeItem* item = parentItem->child( i );
        parentItem->removeChild( item );
    }
    endRemoveRows();

    return true;
}

/**
 * @brief TreeModel::roleNames
 * @return hash map of user role names
*/
QHash<int, QByteArray> TreeModel::roleNames() const
{
    return QAbstractItemModel::roleNames();
}

/**
 * @brief TreeModel::getItem
 * @param index - model index of item
 * @return the TreeItem instance referenced by index
 */
TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if ( index.isValid() ) {
        TreeItem* item = static_cast< TreeItem* >( index.internalPointer() );
        if ( item )
            return item;
    }

    return rootItem;
}

/*!
 * \brief TreeModel::data
 * \param index - the tree item desired
 * \param role - the role type
 * \return - the column data for the tree item referenced by the model index row
 *
 * Overrides QAbstractItemModel::data method.
 *
 * Returns the data stored under the given role for the item referred to by the index.
 * Returns invalid QVariant if this is an invalid model index or unsupported role.
 */
QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if ( ! index.isValid() )
        return QVariant();

    TreeItem *item = static_cast< TreeItem* >( index.internalPointer() );

    if ( ! item )
        return QVariant();

    if ( role == Qt::CheckStateRole && item->isCheckable() && 0 == index.column() ) {
        return static_cast< int >( item->isChecked() ? Qt::Checked : Qt::Unchecked );
    }

    if ( role == CheckableRole && 0 == index.column() ) {
        return item->isCheckable();
    }

    if ( role != Qt::DisplayRole && role != Qt::EditRole )
        return QVariant();

    return item->data( index.column() );
}

/*!
 * \brief TreeModel::flags
 * \param index
 * \return
 *
 * Overrides QAbstractItemModel::flags method,
 *
 * Returns the item flags for the given index.
 */
Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags( Qt::NoItemFlags );

    if ( index.isValid() ) {
        TreeItem *item = getItem( index );
        if ( item ) {
            flags = QAbstractItemModel::flags( index );
            if ( item->isCheckable() ) {
                flags |= Qt::ItemIsUserCheckable;
                flags |= Qt::ItemIsEditable;
            }
            if ( item->isEnabled() ) {
                flags |= Qt::ItemIsEnabled;
            }
        }
    }

    return flags;
}

/*!
 * \brief TreeModel::headerData
 * \param section - indicates the header section
 * \param orientation - indicates the header desired - the horizontal or vertical header
 * \param role - indicates the role type
 * \return - return the data for the given role and section in the referenced header
 *
 * Overrides QAbstractItemModel::headerData method.
 *
 * Returns the data for the given role and section in the header with the specified orientation.
 */
QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        return rootItem->data( section );

    return QVariant();
}


} // GUI
} // ArgoNavis

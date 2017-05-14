/*!
   \file ShowDeviceDetailsDialog.cpp
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

#include "ShowDeviceDetailsDialog.h"
#include "ui_ShowDeviceDetailsDialog.h"

#include "TreeModel.h"
#include "TreeItem.h"


namespace ArgoNavis { namespace GUI {


/**
 * @brief ShowDeviceDetailsDialog::ShowDeviceDetailsDialog
 * @param parent - the parent widget
 *
 * Constructs an show device details dialog instance of the given parent.
 */
ShowDeviceDetailsDialog::ShowDeviceDetailsDialog(QWidget *parent)
    : QDialog( parent )
    , ui( new Ui::ShowDeviceDetailsDialog )
    , m_lastDevice( -1 )
{
    ui->setupUi( this );

    qRegisterMetaType< NameValueList >("NameValueList");

    // Create the data model
    createModel();

    // Configure tree view
    ui->treeView->resizeColumnToContents( 0 );
    ui->treeView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    ui->treeView->setSelectionMode( QAbstractItemView::NoSelection );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    ui->treeView->setSizeAdjustPolicy( QAbstractItemView::AdjustToContents );
#endif
    ui->treeView->setStyleSheet("QTreeView {"
                                "   font: 14px;"
                                "}"
                                " "
                                "QTreeView::branch:has-children:!has-siblings:closed,"
                                "QTreeView::branch:closed:has-children:has-siblings {"
                                "        border-image: none;"
                                "        image: url(:/images/branch-closed);"
                                "}"
                                " "
                                "QTreeView::branch:open:has-children:!has-siblings,"
                                "QTreeView::branch:open:has-children:has-siblings  {"
                                "        border-image: none;"
                                "        image: url(:/images/branch-open);"
                                "}");
}

/**
 * @brief ShowDeviceDetailsDialog::~ShowDeviceDetailsDialog
 *
 * Destroys the instance.
 */
ShowDeviceDetailsDialog::~ShowDeviceDetailsDialog()
{
    delete ui->treeView->model();

    delete ui;
}

/**
 * @brief ShowDeviceDetailsDialog::clearAllDevices
 *
 * Remove all devices from the data model.
 */
void ShowDeviceDetailsDialog::clearAllDevices()
{
    delete ui->treeView->model();

    createModel();
}

/**
 * @brief ShowDeviceDetailsDialog::createModel
 *
 * Create data model.
 */
void ShowDeviceDetailsDialog::createModel()
{
    QList<QVariant> rootData;
    rootData << "Devices";
    m_root = new TreeItem( rootData );

    TreeModel* model = new TreeModel( m_root );

    ui->treeView->setModel( model );
}

/**
 * @brief ShowDeviceDetailsDialog::exec
 * @return - the standard button code pressed
 *
 * Overrides QDialog::exec() method to show the dialog and start the event loop.  Only the device choosen by
 * the user is expanded in the tree view.
 */
int ShowDeviceDetailsDialog::exec()
{
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( action ) {
        QVariant data = action->data();

        const int device = data.toInt();

        if ( m_lastDevice != -1 ) {
            ui->treeView->setRowHidden( m_lastDevice, QModelIndex(), true );
        }

        QAbstractItemModel* model = ui->treeView->model();

        if ( model ) {
            const QModelIndex rootIndex = model->index( device, 0 );
            const QModelIndex attributeItemIndex = model->index( 0, 0, rootIndex );

            // expand the sub-tree associated with the specified device
            ui->treeView->setRowHidden( device, QModelIndex(), false );
            ui->treeView->collapseAll();
            ui->treeView->expand( rootIndex );
            ui->treeView->expand( attributeItemIndex );
            m_lastDevice = device;
        }
    }

    return QDialog::exec();
}

/**
 * @brief ShowDeviceDetailsDialog::handleAddDevice
 * @param deviceNumber - the device number to add
 * @param attributes - the list of name/value pairs associated to device attributes
 * @param maximumLimits - the list of name/value pairs associated to device maximum limits
 *
 * This method adds device information to the data model.
 */
void ShowDeviceDetailsDialog::handleAddDevice(const int deviceNumber, const NameValueList &attributes, const NameValueList &maximumLimits)
{
    // get current model row count which is the index of the new device to be added
    QAbstractItemModel* model = ui->treeView->model();

    if ( ! model )
        return;

    const int deviceCount = model->rowCount();

    // create device item and add as child of the root item
    TreeItem* deviceNameItem = new TreeItem( QList< QVariant>() << QString("Device: %1").arg(deviceNumber), m_root );
    m_root->appendChild( deviceNameItem );

    // create attribute item and add as child of the device item
    TreeItem* attributeItem = new TreeItem( QList< QVariant >() << "Attributes", deviceNameItem );
    deviceNameItem->appendChild( attributeItem );

    // add each attribute name/value pair to the attribute section
    foreach( NameValuePair nameValue, attributes ) {
        // create new attribute name/value item and add as child of the attribute item
        TreeItem* item = new TreeItem( QList< QVariant >() << QString("%1: %2").arg(nameValue.first).arg(nameValue.second), attributeItem );
        attributeItem->appendChild( item );
    }

    // create limits item and add as child of the device item
    TreeItem* limitsItem = new TreeItem( QList< QVariant >() << "Maximum Limits", deviceNameItem );
    deviceNameItem->appendChild( limitsItem );

    // add each maximum limits name/value pair to the maximum limits section
    foreach( NameValuePair nameValue, maximumLimits ) {
        // create new maximum limits name/value item and add as child of the maximum limits item
        TreeItem* item = new TreeItem( QList< QVariant >() << QString("%1: %2").arg(nameValue.first).arg(nameValue.second), limitsItem );
        limitsItem->appendChild( item );
    }

    // hide the new device index
    ui->treeView->setRowHidden( deviceCount, QModelIndex(), true );
}


} // GUI
} // ArgoNavis

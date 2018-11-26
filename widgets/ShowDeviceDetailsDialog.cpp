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

#include <QMutexLocker>
#include <QAction>

#include <limits>


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
    , m_lastDevice( std::numeric_limits< std::size_t >::max() )
{
    ui->setupUi( this );

    qRegisterMetaType< NameValueList >("NameValueList");
}

/**
 * @brief ShowDeviceDetailsDialog::~ShowDeviceDetailsDialog
 *
 * Destroys the instance.
 */
ShowDeviceDetailsDialog::~ShowDeviceDetailsDialog()
{
    clearAllDevices();

    delete ui;
}

/**
 * @brief ShowDeviceDetailsDialog::clearAllDevices
 *
 * Remove all devices from the data model.
 */
void ShowDeviceDetailsDialog::clearAllDevices()
{
    QMutexLocker guard( &m_mutex );

    m_attributes.clear();
    m_limits.clear();
    m_deviceMap.clear();

    m_lastDevice = std::numeric_limits< std::size_t >::max();
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

    // determine which action was the signal sender
    if ( ! action )
        return QDialog::Rejected;

    // get action data containing device number
    QVariant data = action->data();

    // get device number
    const std::size_t device = data.toInt();

    if ( m_lastDevice != device ) {
        QMutexLocker guard( &m_mutex );

        // check whether device map has value for device
        if ( device >= m_deviceMap.size() )
            return QDialog::Rejected;

        // get defined device number
        const std::size_t definedDevice = m_deviceMap[ device ];

        // check whether attributes and limits vectors have values for defined device
        if ( definedDevice >= m_attributes.size() || definedDevice >= m_limits.size() )
            return QDialog::Rejected;

        // class state valid at this point for specified 'device' and 'definedDevice' values

        // clear tree view
        ui->treeView->clear();

        // set header title to indicate device number
        ui->treeView->setHeaderLabel( QString("Device: %1").arg(device) );

        const NameValueList& attributes = m_attributes[ definedDevice ];
        const NameValueList& limits = m_limits[ definedDevice ];

        QTreeWidgetItem* attributeItem = new QTreeWidgetItem( (QTreeWidget*)0, QStringList(QStringLiteral("Attributes")) );
        for (int i = 0; i < attributes.size(); ++i)
            attributeItem->addChild( new QTreeWidgetItem( QStringList(QString("%1: %2").arg(attributes[i].first).arg(attributes[i].second)) ) );

        QTreeWidgetItem* limitItem = new QTreeWidgetItem( (QTreeWidget*)0, QStringList(QStringLiteral("Maximum Limits")) );
        for (int i = 0; i < limits.size(); ++i)
            limitItem->addChild( new QTreeWidgetItem( QStringList(QString("%1: %2").arg(limits[i].first).arg(limits[i].second)) ) );

        QList<QTreeWidgetItem * > items;
        items << attributeItem << limitItem;
        ui->treeView->insertTopLevelItems( 0, items );

        ui->treeView->expandAll();

        m_lastDevice = device;
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
void ShowDeviceDetailsDialog::handleAddDevice(const quint32 deviceNumber, const quint32 definedDeviceNumber, const NameValueList& attributes, const NameValueList& maximumLimits)
{
    QMutexLocker guard( &m_mutex );

    if ( ! attributes.isEmpty() ) {
        if ( m_attributes.size() <= definedDeviceNumber )
            m_attributes.resize( definedDeviceNumber+1 );
        m_attributes[ definedDeviceNumber ] = attributes;
    }

    if ( ! maximumLimits.isEmpty() ) {
        if ( m_limits.size() <= definedDeviceNumber )
            m_limits.resize( definedDeviceNumber+1 );
        m_limits[ definedDeviceNumber ] = maximumLimits;
    }

    if ( m_deviceMap.size() <= deviceNumber )
        m_deviceMap.resize( deviceNumber+1, -1 );
    m_deviceMap[ deviceNumber ] = definedDeviceNumber;
}


} // GUI
} // ArgoNavis

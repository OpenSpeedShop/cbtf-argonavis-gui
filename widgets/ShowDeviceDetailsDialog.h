/*!
   \file ShowDeviceDetailsDialog.h
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

#ifndef SHOWDEVICEDETAILSDIALOG_H
#define SHOWDEVICEDETAILSDIALOG_H

#include <QDialog>

#include "common/openss-gui-config.h"
#include "CBTF-ArgoNavis-Ext/NameValueDefines.h"

namespace Ui {
class ShowDeviceDetailsDialog;
}


namespace ArgoNavis { namespace GUI {


// [ Forward Declarations ]
class TreeModel;
class TreeItem;


class ShowDeviceDetailsDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ShowDeviceDetailsDialog(QWidget *parent = 0);
    ~ShowDeviceDetailsDialog();

    void clearAllDevices();

public slots:

    int exec();

private:

    void createModel();

private slots:

    void handleAddDevice(const int deviceNumber, const NameValueList& attributes, const NameValueList& maximumLimits);

private:

    Ui::ShowDeviceDetailsDialog *ui;

    TreeItem* m_root;

    int m_lastDevice;

};


} // GUI
} // ArgoNavis

#endif // SHOWDEVICEDETAILSDIALOG_H

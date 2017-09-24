/*!
   \file MetricViewFilterDialog.cpp
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

#include "MetricViewFilterDialog.h"

#include "ui_MetricViewFilterDialog.h"


namespace ArgoNavis { namespace GUI {


/**
 * @brief MetricViewFilterDialog::MetricViewFilterDialog
 * @param parent - the parent widget
 *
 * Constructs an MetricViewFilterDialog instance of the given parent.
 */
MetricViewFilterDialog::MetricViewFilterDialog(QWidget *parent)
    : QDialog( parent )
    , ui( new Ui::MetricViewFilterDialog )
{
    ui->setupUi( this );
}

/**
 * @brief MetricViewFilterDialog::~MetricViewFilterDialog
 *
 * Destroys the MetricViewFilterDialog instance.
 */
MetricViewFilterDialog::~MetricViewFilterDialog()
{
    delete ui;
}

/**
 * @brief MetricViewFilterDialog::setColumns
 * @param columnList - list of currently displayed column headers
 *
 * The method sets the list of columns available in the column selection combo-box.
 */
void MetricViewFilterDialog::setColumns(const QStringList &columnList)
{
    // clear the current items from the combo-box
    ui->comboBox_SelectColumn->clear();

    // set the new list of combo-box items
    ui->comboBox_SelectColumn->addItems( columnList );
}


} // GUI
} // ArgoNavis

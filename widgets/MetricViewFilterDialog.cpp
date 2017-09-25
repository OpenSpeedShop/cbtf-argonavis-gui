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

#include <QMenu>
#include <QContextMenuEvent>
#include <QDebug>


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

    // create context-menu actions
    m_deleteFilterItem = new QAction( tr("&Delete Selected Filter(s)"), this );
    m_deleteFilterItem->setStatusTip( tr("Deletes any selected rows in the table") );

    m_deleteAllFilterItems = new QAction( tr("&Clear All Filters"), this );
    m_deleteAllFilterItems->setStatusTip( tr("Clears all filters current defined and shown in the table") );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( m_deleteFilterItem, &QAction::triggered, this, &MetricViewFilterDialog::handleDeleteFilterItem );
    connect( m_deleteAllFilterItems, &QAction::triggered, this, &MetricViewFilterDialog::handleDeleteAllFilterItems );
#else
    connect( m_deleteFilterItem, SIGNAL(triggered(bool)), this, SLOT(handleDeleteFilterItem()) );
    connect( m_deleteAllFilterItems, SIGNAL(triggered(bool)), this, SLOT(handleDeleteAllFilterItems()) );
#endif

    connect( ui->pushButton_Clear, SIGNAL(pressed()), this, SLOT(handleClearPressed()) );
    connect( ui->pushButton_Accept, SIGNAL(pressed()), this, SLOT(handleAcceptPressed()) );

    QPushButton* applyButton = ui->buttonBox_MetricViewFilterDialog->button( QDialogButtonBox::Apply );
    if ( applyButton ) {
        connect( applyButton, SIGNAL(pressed()), this, SLOT(handleApplyPressed()) );
    }

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

#ifndef QT_NO_CONTEXTMENU
/**
 * @brief MetricViewFilterDialog::contextMenuEvent
 * @param event - the context-menu event details
 *
 * This is the handler to receive context-menu events for the dialog.
 */
void MetricViewFilterDialog::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu( this );

    menu.addAction( m_deleteFilterItem );
    menu.addAction( m_deleteAllFilterItems );

    menu.exec( event->globalPos() );
}
#endif // QT_NO_CONTEXTMENU

/**
 * @brief MetricViewFilterDialog::handleClearPressed
 *
 * This is the handler for the QPushButton::clicked() signal for the "Clear" button.
 */
void MetricViewFilterDialog::handleClearPressed()
{
    ui->lineEdit_FilterText->clear();
    ui->comboBox_SelectColumn->setCurrentIndex( 0 );
}

/**
 * @brief MetricViewFilterDialog::handleAcceptPressed
 *
 * This is the handler for the QPushButton::clicked() signal for the "Accept" button.
 */
void MetricViewFilterDialog::handleAcceptPressed()
{
    // get row index for new entry
    const int rowIndex = ui->tableWidget_DefinedFilters->rowCount();

    // create empty row at bottom of table
    ui->tableWidget_DefinedFilters->insertRow( rowIndex );

    QTableWidgetItem* columnItem = new QTableWidgetItem( ui->comboBox_SelectColumn->currentText() );
    ui->tableWidget_DefinedFilters->setItem( rowIndex, 0, columnItem );

    QTableWidgetItem* filterExpressionItem = new QTableWidgetItem( ui->lineEdit_FilterText->text() );
    ui->tableWidget_DefinedFilters->setItem( rowIndex, 1, filterExpressionItem );

    handleClearPressed();
}

/**
 * @brief MetricViewFilterDialog::handleDeleteFilterItem
 *
 * This handles the context-menu item to delete the user selected rows from the table.
 */
void MetricViewFilterDialog::handleDeleteFilterItem()
{
    const QList<QTableWidgetItem*> selectedItems = ui->tableWidget_DefinedFilters->selectedItems();

    for ( int i = selectedItems.size()-1; i>=0; --i ) {
        const QTableWidgetItem* item( selectedItems.at(i) );
        ui->tableWidget_DefinedFilters->removeRow( item->row() );
    }
}

/**
 * @brief MetricViewFilterDialog::handleDeleteAllFilterItems
 *
 * This handles the context-menu item to clear all rows from the table.
 */
void MetricViewFilterDialog::handleDeleteAllFilterItems()
{
    for( int row = ui->tableWidget_DefinedFilters->rowCount()-1; row >= 0; --row ) {
        ui->tableWidget_DefinedFilters->removeRow( row );
    }
}

/**
 * @brief MetricViewFilterDialog::handleApplyPressed
 *
 * This is the handler when the user presses the "Apply" button to apply
 * the defined filters to any proxy models.
 */
void MetricViewFilterDialog::handleApplyPressed()
{
    QList< QPair<QString, QString> > filterList;

    for( int row = 0; row < ui->tableWidget_DefinedFilters->rowCount(); ++row ) {

        const QTableWidgetItem* columnItem( ui->tableWidget_DefinedFilters->item( row, 0 ) );
        const QTableWidgetItem* filterItem( ui->tableWidget_DefinedFilters->item( row, 1 ) );

        filterList << qMakePair( columnItem->text(), filterItem->text() );
    }

    emit applyFilters( filterList );

    QDialog::accept();
}


} // GUI
} // ArgoNavis

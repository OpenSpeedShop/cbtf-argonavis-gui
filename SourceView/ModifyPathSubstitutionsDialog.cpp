/*!
   \file ModifyPathSubstitutionDialog.cpp
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

#include "ModifyPathSubstitutionsDialog.h"

#include "ui_ModifyPathSubstitutionsDialog.h"

#include <QFileDialog>
#include <QMenu>
#include <QContextMenuEvent>


namespace ArgoNavis { namespace GUI {


/**
 * @brief ModifyPathSubstitutionsDialog::ModifyPathSubstitutionsDialog
 * @param parent - specify parent of the ModifyPathSubstitutionsDialog instance
 */
ModifyPathSubstitutionsDialog::ModifyPathSubstitutionsDialog(QWidget *parent)
    : QDialog( parent )
    , ui( new Ui::ModifyPathSubstitutionsDialog )
    , m_selectFilePath( Q_NULLPTR )
{
    ui->setupUi( this );

    setMinimumSize( QSize(690, 485) );

    // create context-menu actions
    m_selectFilePath = new QAction( tr("&Select File"), this );

    if ( m_selectFilePath ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( m_selectFilePath, &QAction::triggered, this, &ModifyPathSubstitutionsDialog::handleSelectFilePath );
#else
        connect( m_selectFilePath, SIGNAL(triggered(bool)), this, SLOT(handleSelectFilePath()) );
#endif
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( ui->tableWidget, &QTableWidget::cellChanged, this, &ModifyPathSubstitutionsDialog::handleCellChanged );
#else
    connect( ui->tableWidget, SIGNAL(cellChanged(int,int)), this, SLOT(handleCellChanged(int,int)) );
#endif
}

/**
 * @brief ModifyPathSubstitutionDialog::~ModifyPathSubstitutionDialog
 *
 * Destroys the ModifyPathSubstitutionDialog instance.
 */
ModifyPathSubstitutionsDialog::~ModifyPathSubstitutionsDialog()
{
    delete ui;

    delete m_selectFilePath;
}

/**
 * @brief ModifyPathSubstitutionsDialog::extractFilenameAndLine
 * @param text - text containing defining location information
 * @param filename - the filename obtained from the defining location information
 * @param lineNumber - the line number obtained from the defining location information
 *
 * Extracts the filename and line number from the data in a particular cell of the metric table view.
 */
void ModifyPathSubstitutionsDialog::extractFilenameAndLine(const QString& text, QString& filename, int& lineNumber)
{
    QString definingLocation;
    lineNumber = -1;
    int startParenIdx = text.lastIndexOf( '(' );
    if ( -1 != startParenIdx ) {
        definingLocation = text.mid( startParenIdx + 1 );
    }
    else {
        definingLocation = text;
    }
    int sepIdx = definingLocation.lastIndexOf( ',' );
    if ( -1 != sepIdx ) {
        filename = definingLocation.left( sepIdx );
        QString lineNumberStr = definingLocation.mid( sepIdx + 1 );
        int endParenIdx = lineNumberStr.lastIndexOf( ')' );
        if ( -1 != endParenIdx ) {
            lineNumberStr.chop( lineNumberStr.length() - endParenIdx );
        }
        lineNumber = lineNumberStr.toInt();
    }
}

/**
 * @brief ModifyPathSubstitutionsDialog::resizeEvent
 * @param e - resize event info
 *
 * This reimplements QDialog::resizeEvent to provide specific handling of the QResizeEvent in
 * the Modify Path Substitution dialog.
 */
void ModifyPathSubstitutionsDialog::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED( e )

    int tableWidth = ui->tableWidget->contentsRect().width();

    ui->tableWidget->setColumnWidth( 0, tableWidth / 2 );
    ui->tableWidget->updateGeometry();
}

/**
 * @brief ModifyPathSubstitutionsDialog::accept
 *
 * Overrides QDialog::accept() to notify external consumers of changes to the path substitutions information.
 * Then calls QDialog::accept().
 */
void ModifyPathSubstitutionsDialog::accept()
{
    QList<int> rowsToRemove;

    QList<int> list = m_modifiedRows.toList();

    for ( int i=0; i<list.size(); i++ ) {
        int index = list.at( i );
        bool success(false);
        QTableWidgetItem* item1 = ui->tableWidget->item( index, 0 );
        if ( item1 ) {
            QString originalPath( item1->text() );
            QTableWidgetItem* item2 = ui->tableWidget->item( index, 1 );
            if ( item2 ) {
                QString newPath( item2->text() );
                if ( ! originalPath.isEmpty() && ! newPath.isEmpty() ) {
                    emit signalAddPathSubstitution( index, originalPath, newPath );
                    success = true;
                }
            }
        }
        if ( ! success ) {
            rowsToRemove << index;
        }
    }

    foreach( int row, rowsToRemove ) {
        ui->tableWidget->removeRow( row );
    }

    m_modifiedRows.clear();

    QDialog::accept();
}

/**
 * @brief ModifyPathSubstitutionsDialog::reject
 *
 * Overrides the QDialog::reject() method and cancels any pending changes.
 * Then calls QDialog::reject().
 */
void ModifyPathSubstitutionsDialog::reject()
{
    m_modifiedRows.clear();

    // remove the last entry with the pre-populated item in the 'Original Path' column
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->removeRow( row-1 );

    QDialog::reject();
}

#ifndef QT_NO_CONTEXTMENU
/**
 * @brief ModifyPathSubstitutionsDialog::contextMenuEvent
 * @param event - the context-menu event details
 *
 * This is the handler to receive context-menu events for the widget.
 */
void ModifyPathSubstitutionsDialog::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu( this );

    menu.addAction( m_selectFilePath );

    menu.exec( event->globalPos() );
}
#endif // QT_NO_CONTEXTMENU

/**
 * @brief ModifyPathSubstitutionsDialog::exec
 * @return - the standard button code pressed
 *
 * Overrides QDialog::exec() method to pre-populate path substitutions dialog with the original path
 * in accordance with the item selected.
 */
int ModifyPathSubstitutionsDialog::exec()
{
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( action ) {
        QVariant data = action->data();

        int row = ui->tableWidget->rowCount();
        ui->tableWidget->setRowCount( row + 1 );

        QTableWidgetItem* item = new QTableWidgetItem( data.toString() );
        ui->tableWidget->setItem( row, 0, item );
    }

    return QDialog::exec();
}

/**
 * @brief ModifyPathSubstitutionsDialog::handleCellChanged
 * @param row - the row changed
 * @param column - the column changed
 *
 * Handler invoked when a QTreeView cell has changed.
 */
void ModifyPathSubstitutionsDialog::handleCellChanged(int row, int column)
{
    Q_UNUSED( column )

    m_modifiedRows.insert( row );
}

/**
 * @brief ModifyPathSubstitutionsDialog::handleSelectFilePath
 *
 * This handles the "Select File" action trigger and opens the
 */
void ModifyPathSubstitutionsDialog::handleSelectFilePath()
{
    QFileDialog* dialog = new QFileDialog( this, tr("Select Directory For File") );

    if ( ! dialog )
        return;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( this, &ModifyPathSubstitutionsDialog::finished, dialog, &QDialog::done );
    connect( dialog, &QFileDialog::fileSelected, this, &ModifyPathSubstitutionsDialog::handleFileSelected );
#else
    connect( this, SIGNAL(finished(int)), dialog, SLOT(done(int)) );
    connect( this, &ModifyPathSubstitutionsDialog::finished, this, &ModifyPathSubstitutionsDialog::handleFileSelected );
#endif

    dialog->setAttribute( Qt::WA_DeleteOnClose, true );
    dialog->show();
}

/**
 * @brief ModifyPathSubstitutionsDialog::handleFileSelected
 * @param file - the name of the selected file
 *
 * This method takes the absolute file path and extracts the directory path and sets the "New Path"
 * table item for the current row with the directory path.
 */
void ModifyPathSubstitutionsDialog::handleFileSelected(const QString &file)
{
    const int row = ui->tableWidget->rowCount() - 1;

    const QFileInfo fileInfo( file );

    QTableWidgetItem* item = new QTableWidgetItem( fileInfo.path() );

    ui->tableWidget->setItem( row, 1, item );
}


} // GUI
} // ArgoNavis

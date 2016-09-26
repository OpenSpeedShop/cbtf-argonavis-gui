/*!
   \file ModifyPathSubstitutionDialog.h
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

#ifndef MODIFYPATHSUBSTITUTIONSDIALOG_H
#define MODIFYPATHSUBSTITUTIONSDIALOG_H

#include <QDialog>
#include <QSet>

#include "common/openss-gui-config.h"

// Forward Declarations

namespace Ui {
class ModifyPathSubstitutionsDialog;
}


namespace ArgoNavis { namespace GUI {


class ModifyPathSubstitutionsDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ModifyPathSubstitutionsDialog(QWidget *parent = 0);
    virtual ~ModifyPathSubstitutionsDialog();

signals:

    void signalAddPathSubstitution(int index, const QString& oldPath, const QString& newPath);

protected:

    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
    void accept() Q_DECL_OVERRIDE;
    void reject() Q_DECL_OVERRIDE;

protected slots:

    int exec() Q_DECL_OVERRIDE;

private slots:

    void handleCellChanged(int row, int column);

private:

    Ui::ModifyPathSubstitutionsDialog *ui;

    QSet< int > m_modifiedRows;

};


} // GUI
} // ArgoNavis

#endif // MODIFYPATHSUBSTITUTIONSDIALOG_H

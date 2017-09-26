/*!
   \file MetricViewFilterDialog.h
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

#ifndef METRICVIEWFILTERDIALOG_H
#define METRICVIEWFILTERDIALOG_H

#include <QDialog>

#include <QList>

#include "common/openss-gui-config.h"

namespace Ui {
class MetricViewFilterDialog;
}

#ifndef QT_NO_CONTEXTMENU
class QContextMenuEvent;
#endif
class QTableWidgetItem;

namespace ArgoNavis { namespace GUI {


class FilterExpressionValidator;


class MetricViewFilterDialog : public QDialog
{
    Q_OBJECT

public:

    explicit MetricViewFilterDialog(QWidget *parent = 0);
    ~MetricViewFilterDialog();

    void setColumns(const QStringList &columnList);

signals:

    void applyFilters(const QList<QPair<QString, QString>>& filters, bool applyNow);

protected:

    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

#ifndef QT_NO_CONTEXTMENU
protected slots:

    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
#endif // QT_NO_CONTEXTMENU

private slots:

    void handleClearPressed();
    void handleAcceptPressed();
    void handleDeleteFilterItem();
    void handleDeleteAllFilterItems();
    void handleApplyPressed();
    void handleOkPressed();
    void handleCancelPressed();
    void handleValidateFilterExpression(const QString &text);

private:

    void processDefinedFilters(bool applyNow);

private:

    Ui::MetricViewFilterDialog *ui;

    QAction* m_deleteFilterItem;
    QAction* m_deleteAllFilterItems;

    typedef struct {
        int numRows;
        int numColumns;
        QList< QTableWidgetItem* > items;
        void reset() {
            numRows = numColumns = 0;
            items.clear();
        }
    } FilterSnapshotInfo;

    FilterSnapshotInfo m_snapshot;

    FilterExpressionValidator* m_validator;

};


} // GUI
} // ArgoNavis

#endif // METRICVIEWFILTERDIALOG_H

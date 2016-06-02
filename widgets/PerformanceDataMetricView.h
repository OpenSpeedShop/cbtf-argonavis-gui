/*!
   \file PerformanceDataTableView.h
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

#ifndef PERFORMANCEDATAMETRICVIEW_H
#define PERFORMANCEDATAMETRICVIEW_H

#include <QWidget>
#include <QTreeView>

// [ Forward Declarations ]

namespace Ui {
class PerformanceDataMetricView;
}

class QStandardItemModel;
class QSortFilterProxyModel;
class QVBoxLayout;
class QStackedLayout;


namespace ArgoNavis { namespace GUI {


/*!
 * \brief The PerformanceDataMetricView class
 */

class PerformanceDataMetricView : public QWidget
{
    Q_OBJECT

public:

    explicit PerformanceDataMetricView(QWidget *parent = 0);
    virtual ~PerformanceDataMetricView();

    void showBlankView();

    void deleteAllModelsViews();

public slots:

    void handleInitModel(const QString& metricView, const QStringList& metrics);
    void handleAddData(const QString &metricView, const QVariantList& data);

private slots:

    void handleMetricViewChanged(const QString &metricView);

private:

    Ui::PerformanceDataMetricView *ui;

    QMap< QString, QStandardItemModel* > m_models;          // map metric to model
    QMap< QString, QSortFilterProxyModel* > m_proxyModels;  // map metric to model
    QMap< QString, QTreeView* > m_views;                    // map metric to view
    QStackedLayout* m_viewStack;                            // vertical layout holding current view

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAMETRICVIEW_H

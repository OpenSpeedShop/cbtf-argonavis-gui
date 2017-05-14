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
#include <QMutex>
#include <QMap>
#include <QStandardItemModel>

#include "CBTF-ArgoNavis-Ext/NameValueDefines.h"

// [ Forward Declarations ]

namespace Ui {
class PerformanceDataMetricView;
}

class QSortFilterProxyModel;
class QVBoxLayout;
class QStackedLayout;
class QAction;


namespace ArgoNavis { namespace GUI {


class ModifyPathSubstitutionsDialog;
class ShowDeviceDetailsDialog;


/*!
 * \brief The PerformanceDataMetricView class
 */

class PerformanceDataMetricView : public QWidget
{
    Q_OBJECT

    typedef enum { MENU_TYPE_UNDEFINED, DEFINE_PATH_MAPPINGS, SHOW_DEVICE_DETAILS } DetailsMenuTypes;

public:

    explicit PerformanceDataMetricView(QWidget *parent = 0);
    virtual ~PerformanceDataMetricView();

    void showBlankView();

    void deleteAllModelsViews();

    typedef enum { DETAILS_MODE = 1, METRIC_MODE = 2, CALLTREE_MODE = 4 } ModeType;

    Q_DECLARE_FLAGS( ModeTypes, ModeType )

    void setAvailableMetricModes(const ModeTypes& modes);

signals:

    void signalAddDevice(const int deviceNumber, const NameValueList& attributes, const NameValueList& maximumLimits);
    void signalRequestMetricView(const QString& clusterName, const QString& metricName, const QString& viewName);
    void signalRequestDetailView(const QString& clusterName, const QString& detailName);
    void signalClearSourceView();
    void signalDisplaySourceFileLineNumber(const QString& filename, int lineNumber);
    void signalAddPathSubstitution(int index, const QString& oldPath, const QString& newPath);

public slots:

    void handleInitModel(const QString& clusterName, const QString& metricName, const QString& viewName, const QStringList& metrics);
    void handleInitModelView(const QString& clusterName, const QString& metricName, const QString& viewName, const QString& attachedMetricViewName, const QStringList& metrics);
    void handleAddData(const QString& clusterName, const QString &metricName, const QString& viewName, const QVariantList& data, const QStringList& columnHeaders);
    void handleRangeChanged(const QString& clusterName, const QString& metricName, const QString& viewName, double lower, double upper);

private slots:

    void handleViewModeChanged(const QString &text);
    void handleMetricViewChanged(const QString &text);
    void handleRequestMetricViewComplete(const QString& clusterName, const QString& metricName, const QString& viewName, double lower, double upper);
    void showContextMenu(const DetailsMenuTypes menuType, const QVariant& index, const QPoint& globalPos);
    void handleTableViewItemClicked(const QModelIndex& index);
    void handleCustomContextMenuRequested(const QPoint& pos);

private:

    void extractFilenameAndLine(const QString& text, QString& filename, int& lineNumber);
    void processTableViewItemClicked(QTreeView* view, const QModelIndex& index);
    void processCustomContextMenuRequested(QTreeView* view, const QPoint &pos);

private:

    Ui::PerformanceDataMetricView *ui;

    static QString s_functionTitle;
    static QString s_deviceTitle;

    QString m_clusterName;                                  // cluster name associated to metric views
    QMutex m_mutex;                                         // mutex for the following QMap objects
    QMap< QString, QStandardItemModel* > m_models;          // map metric to model
    QMap< QString, QSortFilterProxyModel* > m_proxyModels;  // map metric to model
    QMap< QString, QTreeView* > m_views;                    // map metric to view

    QStackedLayout* m_viewStack;                            // vertical layout holding current view

    ModeType m_mode;

    QStandardItemModel m_metricViewModel;                   // snapshot of view combobox model for metric mode
    QStandardItemModel m_detailsViewModel;                  // initialized view combobox model for details mode
    QStandardItemModel m_calltreeViewModel;                 // empty combobox model for calltree mode

    ModifyPathSubstitutionsDialog* m_modifyPathsDialog;
    ShowDeviceDetailsDialog* m_deviceDetailsDialog;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAMETRICVIEW_H

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
class MetricViewFilterDialog;
class DerivedMetricInformationDialog;


/*!
 * \brief The PerformanceDataMetricView class
 */

class PerformanceDataMetricView : public QWidget
{
    Q_OBJECT

    typedef enum { MENU_TYPE_UNDEFINED,
                   DEFAULT_CONTEXT_MENU,
                   DEFINE_PATH_MAPPINGS,
                   SHOW_DEVICE_DETAILS,
                   SHOW_DERIVED_METRICS
                 } DetailsMenuTypes;

public:

    explicit PerformanceDataMetricView(QWidget *parent = 0);
    virtual ~PerformanceDataMetricView();

    void showBlankView();

    void deleteAllModelsViews();

    typedef enum { DETAILS_MODE = 1,
                   METRIC_MODE = 2,
                   DERIVED_METRIC_MODE = 4,
                   CALLTREE_MODE = 8,
                   COMPARE_MODE = 16,
                   COMPARE_BY_RANK_MODE = 32,
                   COMPARE_BY_HOST_MODE = 64,
                   COMPARE_BY_PROCESS_MODE = 128,
                   LOAD_BALANCE_MODE = 256,
                   TRACE_MODE = 512
                 } ModeType;

    Q_DECLARE_FLAGS( ModeTypes, ModeType )

    void setAvailableMetricModes(const ModeTypes& modes);

    static QString getMetricModeName(const ModeType mode);

    static QString getMetricViewName(const QString &modeName, const QString &metricName, const QString &viewName);

signals:

    void signalAddDevice(const quint32 deviceNumber, const quint32 definedDeviceNumber, const NameValueList& attributes, const NameValueList& maximumLimits);
    void signalRequestDerivedMetricView(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName);
    void signalRequestMetricView(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName);
    void signalRequestLoadBalanceView(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName);
    void signalRequestCompareView(const QString& clusteringCriteriaName, const QString& compareMode, const QString& metricName, const QString& viewName);
    void signalRequestCalltreeView(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName);
    void signalRequestTraceView(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName);
    void signalRequestDetailView(const QString& clusteringCriteriaName, const QString& detailName);
    void signalClearSourceView();
    void signalDisplaySourceFileLineNumber(const QString& filename, int lineNumber);
    void signalTraceItemSelected(const QString& definingLocation, double timeBegin, double timeEnd, int rank);
    void signalAddPathSubstitution(int index, const QString& oldPath, const QString& newPath);
    void signalMetricViewChanged(const QString& metricViewName);

public slots:

    void handleInitModel(const QString& clusteringCriteriaName, const QString& modeName, const QString& metricName, const QString& viewName, const QStringList& metrics);
    void handleInitModelView(const QString& clusteringCriteriaName, const QString& modeName, const QString& metricName, const QString& viewName, const QString& attachedMetricViewName, const QStringList& metrics);
    void handleAddData(const QString& clusteringCriteriaName, const QString& modeName, const QString &metricName, const QString& viewName, const QVariantList& data, const QStringList& columnHeaders);
    void handleRangeChanged(const QString& clusteringCriteriaName, const QString &modeName, const QString& metricName, const QString& viewName, double lower, double upper);
    void handleRequestViewUpdate(bool clearExistingViews);

private slots:

    void handleViewModeChanged(const QString &text);
    void handleMetricViewChanged(const QString &text);
    void handleRequestMetricViewComplete(const QString& clusteringCriteriaName, const QString& modeName, const QString& metricName, const QString& viewName, double lower, double upper);
    void showContextMenu(const DetailsMenuTypes menuType, const QVariant& index, const QPoint& globalPos);
    void handleTableViewItemClicked(const QModelIndex& index);
    void handleCustomContextMenuRequested(const QPoint& pos);
    void handleApplyClearFilters();
    void handleApplyFilter(const QList<QPair<QString,QString> >& filters, bool applyNow);

private:

    void applyFilterToCurrentView(const QList<QPair<QString, QString> > &filters);
    void processTableViewItemClicked(const QAbstractItemModel *model, const QModelIndex &index);
    void processTableViewItemClicked(QTreeView* view, const QModelIndex& index);
    void processCustomContextMenuRequested(QTreeView* view, const QPoint &pos);
    void clearExistingModelsAndViews(const QString &metricViewName, bool deleteModel = true, bool deleteView = false);
    bool deleteModelsAndViews();
    void resetUI();

    QString getMetricViewName() const;

private:

    Ui::PerformanceDataMetricView *ui;

    static QString s_functionTitle;
    static QString s_deviceTitle;

    static QString s_metricModeName;
    static QString s_derivedMetricModeName;
    static QString s_detailsModeName;
    static QString s_calltreeModeName;
    static QString s_compareModeName;
    static QString s_compareByRankModeName;
    static QString s_compareByHostModeName;
    static QString s_compareByProcessModeName;
    static QString s_loadBalanceModeName;
    static QString s_traceModeName;

    static QString s_functionViewName;
    static QString s_statementsViewName;
    static QString s_linkedObjectsViewName;
    static QString s_loopsViewName;

    static QString s_allEventsDetailsName;
    static QString s_dataTransfersDetailsName;
    static QString s_kernelExecutionsDetailsName;

    static QString s_noneName;

    static QString s_APPLY_FILTERS_STR;
    static QString s_CLEAR_FILTERS_STR;

    QString m_clusteringCritieriaName;                      // clustering criteria name associated to metric views
    QMutex m_mutex;                                         // mutex for the following QMap objects
    QMap< QString, QStandardItemModel* > m_models;          // map metric to model
    QMap< QString, QSortFilterProxyModel* > m_proxyModels;  // map metric to model
    QMap< QString, QTreeView* > m_views;                    // map metric to view

    QList< QPair< QString, QString > > m_currentFilter;     // currently available user-defined metric view filters

    QStackedLayout* m_viewStack;                            // vertical layout holding current view

    ModeType m_mode;

    QStandardItemModel m_dummyModel;                        // dummy model

    // models for view combobox
    QStandardItemModel m_metricViewModel;                   // snapshot of view combobox model for metric mode
    QStandardItemModel m_derivedMetricViewModel;            // snapshot of view combobox model for derived metric mode
    QStandardItemModel m_loadBalanceViewModel;              // snapshot of view combobox model for load balance mode
    QStandardItemModel m_detailsViewModel;                  // initialized view combobox model for details mode
    QStandardItemModel m_calltreeViewModel;                 // empty combobox model for calltree mode
    QStandardItemModel m_traceViewModel;                    // empty combobox model for trace mode
    QStandardItemModel m_compareViewModel;                  // snapshot of view combobox model for compare mode
    // models for metric combobox
    QStandardItemModel m_metricModeMetricModel;             // snapshot of metric combobox model for metric mode
    QStandardItemModel m_traceModeMetricModel;              // snapshot of metric combobox model for trace mode

    ModifyPathSubstitutionsDialog* m_modifyPathsDialog;
    ShowDeviceDetailsDialog* m_deviceDetailsDialog;
    MetricViewFilterDialog* m_metricViewFilterDialog;

};


} // GUI
} // ArgoNavis

#endif // PERFORMANCEDATAMETRICVIEW_H

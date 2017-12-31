#ifndef DERIVEDMETRICINFORMATIONDIALOG_H
#define DERIVEDMETRICINFORMATIONDIALOG_H

#include <QDialog>

#include <QSignalMapper>

#include "common/openss-gui-config.h"


namespace Ui {
class DerivedMetricInformationDialog;
}

#ifndef QT_NO_CONTEXTMENU
class QContextMenuEvent;
#endif


namespace ArgoNavis { namespace GUI {


class ConfigureUserDerivedMetricsDialog;


class DerivedMetricInformationDialog : public QDialog
{
    Q_OBJECT

public:

    explicit DerivedMetricInformationDialog(QWidget *parent = 0);
    ~DerivedMetricInformationDialog();

protected:

    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#ifndef QT_NO_CONTEXTMENU
protected slots:

    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
#endif // QT_NO_CONTEXTMENU
#endif

private slots:

    void handleNewDerivedMetricDefined(const QString& name, const QString& formula, bool enabled);
    void handleCheckboxClicked(QWidget* widget);
    void handleLoadUserDefinedMetric();
    void handleSaveUserDefinedMetric();

private:

    Ui::DerivedMetricInformationDialog *ui;

    ConfigureUserDerivedMetricsDialog* m_configureUserDerivedMetricsDialog;

    QSignalMapper* m_mapper;

    int m_userDefinedStartIndex;

    QString m_directoryPath;

};


} // GUI
} // ArgoNavis

#endif // DERIVEDMETRICINFORMATIONDIALOG_H

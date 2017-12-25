#ifndef DERIVEDMETRICINFORMATIONDIALOG_H
#define DERIVEDMETRICINFORMATIONDIALOG_H

#include <QDialog>

#include "common/openss-gui-config.h"


namespace Ui {
class DerivedMetricInformationDialog;
}


namespace ArgoNavis { namespace GUI {


class DerivedMetricInformationDialog : public QDialog
{
    Q_OBJECT

public:

    explicit DerivedMetricInformationDialog(QWidget *parent = 0);
    ~DerivedMetricInformationDialog();

protected:

    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private:

    Ui::DerivedMetricInformationDialog *ui;

};


} // GUI
} // ArgoNavis

#endif // DERIVEDMETRICINFORMATIONDIALOG_H

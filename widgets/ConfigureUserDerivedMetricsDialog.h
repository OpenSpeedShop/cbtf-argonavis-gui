#ifndef CONFIGUREUSERDERIVEDMETRICSDIALOG_H
#define CONFIGUREUSERDERIVEDMETRICSDIALOG_H

#include <QDialog>

namespace Ui {
class ConfigureUserDerivedMetricsDialog;
}


namespace ArgoNavis { namespace GUI {


class ConfigureUserDerivedMetricsDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ConfigureUserDerivedMetricsDialog(QWidget *parent = 0);
    ~ConfigureUserDerivedMetricsDialog();

signals:

    void signalNewDerivedMetricDefined(const QString& name, const QString& formula, bool enabled);

private slots:

    void handleApplyButtonClicked();
    void handleCancelButtonClicked();
    void handleOkButtonClicked();

private:

    void reset();

private:

    Ui::ConfigureUserDerivedMetricsDialog *ui;

};


} // GUI
} // ArgoNavis

#endif // CONFIGUREUSERDERIVEDMETRICSDIALOG_H

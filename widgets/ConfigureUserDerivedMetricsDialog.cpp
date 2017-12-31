#include "ConfigureUserDerivedMetricsDialog.h"
#include "ui_ConfigureUserDerivedMetricsDialog.h"

#include <QPushButton>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QRegularExpression>
#else
#include <QRegExp>
#endif


namespace ArgoNavis { namespace GUI {

/**
 * @brief ConfigureUserDerivedMetricsDialog::ConfigureUserDerivedMetricsDialog
 * @param parent - specify parent of the ConfigureUserDerivedMetricsDialog instance
 *
 * Constructs a ConfigureUserDerivedMetricsDialog which is a child of parent.  If parent is 0, the ConfigureUserDerivedMetricsDialog becomes a window.
 */
ConfigureUserDerivedMetricsDialog::ConfigureUserDerivedMetricsDialog(QWidget *parent)
    : QDialog( parent )
    , ui( new Ui::ConfigureUserDerivedMetricsDialog )
{
    ui->setupUi( this );

    QPushButton* applyButton = ui->buttonBox_DefineDerivedMetric->button( QDialogButtonBox::Apply );
    QPushButton* cancelButton = ui->buttonBox_DefineDerivedMetric->button( QDialogButtonBox::Cancel );
    QPushButton* okButton = ui->buttonBox_DefineDerivedMetric->button( QDialogButtonBox::Ok );

    connect( applyButton, SIGNAL(clicked(bool)), this, SLOT(handleApplyButtonClicked()) );
    connect( cancelButton, SIGNAL(clicked(bool)), this, SLOT(handleCancelButtonClicked()) );
    connect( okButton, SIGNAL(clicked(bool)), this, SLOT(handleOkButtonClicked()) );
}

/**
 * @brief ConfigureUserDerivedMetricsDialog::~ConfigureUserDerivedMetricsDialog
 *
 * The ConfigureUserDerivedMetricsDialog destructor.
 */
ConfigureUserDerivedMetricsDialog::~ConfigureUserDerivedMetricsDialog()
{
    delete ui;
}

/**
 * @brief ConfigureUserDerivedMetricsDialog::handleApplyButtonClicked
 *
 * This handles the 'Apply' button being clicked.  Performs some simple entry simplification and validation and emits
 * the 'signalNewDerivedMetricDefined' signal if validation successful. The dialog state is reset.
 */
void ConfigureUserDerivedMetricsDialog::handleApplyButtonClicked()
{
    if ( ( ! ui->lineEdit_NameDescription->text().isEmpty() ) && ( ! ui->lineEdit_Formula->text().isEmpty() ) ) {
        const QString formula = ui->lineEdit_Formula->text().simplified();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        const bool result = formula.contains( QRegularExpression("[^\\sA-Za-z0-9_/*-+()]") );
#else
        const bool result = formula.contains( QRegExp("[^\\sA-Za-z0-9_/*-+()]") );
#endif
        if ( ! result ) {
            const QString name = ui->lineEdit_NameDescription->text().simplified();
            emit signalNewDerivedMetricDefined( name, formula, ui->checkBox_Enabled->isChecked() );
        }
    }

    reset();
}

/**
 * @brief ConfigureUserDerivedMetricsDialog::handleCancelButtonClicked
 *
 * This handles the 'Cancel' button being clicked.  Resets the dialog state and calls QDialog::reject().
 */
void ConfigureUserDerivedMetricsDialog::handleCancelButtonClicked()
{
    reset();

    QDialog::reject();
}

/**
 * @brief ConfigureUserDerivedMetricsDialog::handleOkButtonClicked
 *
 * This handles the 'Ok' button being clicked.  Calls the 'Apply' button handler and calls QDialog::accept().
 */
void ConfigureUserDerivedMetricsDialog::handleOkButtonClicked()
{
    handleApplyButtonClicked();

    QDialog::accept();
}

/**
 * @brief ConfigureUserDerivedMetricsDialog::reset
 *
 * This resets the dialog state - ie clears all entry fields.
 */
void ConfigureUserDerivedMetricsDialog::reset()
{
    ui->lineEdit_NameDescription->clear();
    ui->lineEdit_Formula->clear();
    ui->checkBox_Enabled->setChecked( false );
}


} // GUI
} // ArgoNavis

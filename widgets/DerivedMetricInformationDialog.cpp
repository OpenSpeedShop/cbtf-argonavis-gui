#include "DerivedMetricInformationDialog.h"
#include "ui_DerivedMetricInformationDialog.h"

#include "managers/DerivedMetricsSolver.h"

#include <QTableWidgetItem>
#include <QCheckBox>


namespace ArgoNavis { namespace GUI {


/**
 * @brief DerivedMetricInformationDialog::DerivedMetricInformationDialog
 * @param parent - specify parent of the DerivedMetricInformationDialog instance
 *
 * Constructs a DerivedMetricInformationDialog which is a child of parent.  If parent is 0, the  DerivedMetricInformationDialog becomes a window.
 */
DerivedMetricInformationDialog::DerivedMetricInformationDialog(QWidget *parent)
    : QDialog( parent )
    , ui( new Ui::DerivedMetricInformationDialog )
    , m_mapper( nullptr )
{
    ui->setupUi( this );
}

/**
 * @brief DerivedMetricInformationDialog::~DerivedMetricInformationDialog
 *
 * Destroys this DerivedMetricInformationDialog instance.
 */
DerivedMetricInformationDialog::~DerivedMetricInformationDialog()
{
    delete ui;
}

/**
 * @brief DerivedMetricInformationDialog::showEvent
 * @param event - the show event instance
 *
 * This method re-implements the QDialog::showEvent() method.  This implementation
 * initializes the table widget with the data for each derived metric definition.
 * Since the number of derived metrics is assumed to be a low number and can be limited
 * as needed, this implementation is the easiest for now.
 */
void DerivedMetricInformationDialog::showEvent(QShowEvent *event)
{
    Q_UNUSED( event );

    // clear current contents
    for (int i=ui->tableWidget->rowCount(); i>=0; --i) {
        ui->tableWidget->removeRow( i );
    }

    delete m_mapper;

    m_mapper = new QSignalMapper;

    // construct derived metric solver on stack
    DerivedMetricsSolver solver;

    // get the derived metric data
    const QVector<QVariantList> data = solver.getDerivedMetricData();

    int rowCount( 0 );

    // foreach derived metric contained in the data vector
    foreach ( const QVariantList& list, data ) {
        if ( 3 == list.size() ) {
            // add new row to table
            ui->tableWidget->insertRow( rowCount );

            // insert "name/description" and "formula" column items
            ui->tableWidget->setItem( rowCount, 0, new QTableWidgetItem(list[0].toString()) );
            ui->tableWidget->setItem( rowCount, 1, new QTableWidgetItem(list[1].toString()) );

            // construct and initialize checkbox and insert as last column item
            QCheckBox* checkbox = new QCheckBox;
            checkbox->setObjectName( list[0].toString() );
            checkbox->setChecked( list[2].toBool() );

            m_mapper->setMapping( checkbox, checkbox );
            connect( checkbox, SIGNAL(clicked(bool)), m_mapper, SLOT(map()) );

            ui->tableWidget->setCellWidget( rowCount, 2, checkbox );

            ++rowCount;
        }
    }

    connect( m_mapper, SIGNAL(mapped(QWidget*)), this, SLOT(handleCheckboxClicked(QWidget*)) );
}

/**
 * @brief DerivedMetricInformationDialog::handleCheckboxClicked
 * @param widget - a QCheckBox instance
 *
 * This method handles QSignalMapper::mapped(QWidget*) signal.  First the QWidget instance
 * is casted to a QCheckBox pointer.  Then a DerivedMetricsSolver is contructed and used
 * to set the enabled state of the corresponding derived metric (key equals the 'objectName' property).
 */
void DerivedMetricInformationDialog::handleCheckboxClicked(QWidget *widget)
{
    QCheckBox* checkbox = qobject_cast<QCheckBox *>( widget );

    if ( checkbox ) {
        // construct derived metric solver on stack
        DerivedMetricsSolver solver;

        solver.setEnabled( checkbox->objectName(), checkbox->isChecked() );
    }
}


} // GUI
} // ArgoNavis

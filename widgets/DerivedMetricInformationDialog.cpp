#include "DerivedMetricInformationDialog.h"
#include "ui_DerivedMetricInformationDialog.h"

#include "managers/DerivedMetricsSolver.h"
#include "widgets/ConfigureUserDerivedMetricsDialog.h"
#include "widgets/DerivedMetricInformation.h"

#include <QTableWidgetItem>
#include <QCheckBox>
#include <QMenu>
#include <QContextMenuEvent>
#include <QFileDialog>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QJsonDocument>
#include <QJsonArray>
#endif


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
    , m_userDefinedStartIndex( -1 )
{
    ui->setupUi( this );

    // create 'Configure User Derived Metrics' dialog
    m_configureUserDerivedMetricsDialog = new ConfigureUserDerivedMetricsDialog( this );

    connect( m_configureUserDerivedMetricsDialog, SIGNAL(signalNewDerivedMetricDefined(QString,QString,bool)),
             this, SLOT(handleNewDerivedMetricDefined(QString,QString,bool)) );

    // connect 'Configure User Defined' button to 'Configure User Derived Metrics' dialog
    connect( ui->pushButton_ConfigureUserDefined, SIGNAL(clicked(bool)), m_configureUserDerivedMetricsDialog, SLOT(exec()) );
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
    for (int i=ui->tableWidget_DerivedMetricDefinitions->rowCount(); i>=0; --i) {
        ui->tableWidget_DerivedMetricDefinitions->removeRow( i );
    }

    delete m_mapper;

    m_mapper = new QSignalMapper;

    // construct derived metric solver on stack
    const DerivedMetricsSolver* solver = DerivedMetricsSolver::instance();

    // get the derived metric data
    const QVector<QVariantList> data = solver->getDerivedMetricData();

    int rowCount( 0 );

    // foreach derived metric contained in the data vector
    foreach ( const QVariantList& list, data ) {
        if ( 3 == list.size() ) {
            // add new row to table
            ui->tableWidget_DerivedMetricDefinitions->insertRow( rowCount );

            // insert "name/description" and "formula" column items
            ui->tableWidget_DerivedMetricDefinitions->setItem( rowCount, 0, new QTableWidgetItem(list[0].toString()) );
            ui->tableWidget_DerivedMetricDefinitions->setItem( rowCount, 1, new QTableWidgetItem(list[1].toString()) );

            // construct and initialize checkbox and insert as last column item
            QCheckBox* checkbox = new QCheckBox;
            checkbox->setObjectName( list[0].toString() );
            checkbox->setChecked( list[2].toBool() );

            m_mapper->setMapping( checkbox, checkbox );
            connect( checkbox, SIGNAL(clicked(bool)), m_mapper, SLOT(map()) );

            ui->tableWidget_DerivedMetricDefinitions->setCellWidget( rowCount, 2, checkbox );

            ++rowCount;
        }
    }

    connect( m_mapper, SIGNAL(mapped(QWidget*)), this, SLOT(handleCheckboxClicked(QWidget*)) );
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#ifndef QT_NO_CONTEXTMENU
/**
 * @brief DerivedMetricInformationDialog::contextMenuEvent
 * @param event - the context-menu event details
 *
 * This is the handler to receive context-menu events for the widget.
 */
void DerivedMetricInformationDialog::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu( this );

    menu.addAction( tr("Load User Derived Metrics"), this, SLOT(handleLoadUserDefinedMetric()) );
    menu.addAction( tr("Save User Derived Metrics"), this, SLOT(handleSaveUserDefinedMetric()) );

    menu.exec( event->globalPos() );
}
#endif // QT_NO_CONTEXTMENU
#endif

/**
 * @brief DerivedMetricInformationDialog::handleNewDerivedMetricDefined
 * @param name - the name/description of the new derived metric
 * @param formula - the formula for the derived metric
 * @param enabled - whether the derived metric is currently enabled
 *
 * This handler is invoked when the ConfigureUserDerivedMetricsDialog::signalNewDerivedMetricDefined signal is emitted.  It attempts
 * to add the new derived metric to the DerivedMetricsSolver singleton instance database.
 */
void DerivedMetricInformationDialog::handleNewDerivedMetricDefined(const QString &name, const QString &formula, bool enabled)
{
    // construct derived metric solver on stack
    DerivedMetricsSolver* solver = DerivedMetricsSolver::instance();

    if ( solver ) {
        int startingRowCount = ui->tableWidget_DerivedMetricDefinitions->rowCount();
        if ( solver->insert( name, formula, enabled ) ) {
            QApplication::postEvent( this, new QShowEvent() );

            if ( -1 == m_userDefinedStartIndex ) {
                m_userDefinedStartIndex = startingRowCount;
            }
        }
    }
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
        DerivedMetricsSolver* solver = DerivedMetricsSolver::instance();

        solver->setEnabled( checkbox->objectName(), checkbox->isChecked() );
    }
}

/**
 * @brief DerivedMetricInformationDialog::handleLoadUserDefinedMetric
 *
 * This method handles the 'Load User Derived Metrics' context-menu item triggered action.
 */
void DerivedMetricInformationDialog::handleLoadUserDefinedMetric()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    const QString directoryPath = m_directoryPath.isEmpty() ? QApplication::applicationDirPath() : m_directoryPath;

    const QString fileName = QFileDialog::getOpenFileName( this, tr("Open User Derived Metrics XML File"), directoryPath, tr("XML Files (*.xml)") );

    // construct derived metric solver on stack
    DerivedMetricsSolver* solver = DerivedMetricsSolver::instance();

    QFile file( fileName );

    if ( file.open( QFile::ReadOnly ) ) {
        const QByteArray contents = file.readAll();

        QJsonDocument doc( QJsonDocument::fromJson( contents ) );

        if ( doc.isObject() ) {
            QJsonObject root = doc.object();

            if ( root.contains("metrics") && root["metrics"].isArray() ) {
                QJsonArray array = root["metrics"].toArray();

                int rowCount( ui->tableWidget_DerivedMetricDefinitions->rowCount() );

                int startingRowCount = rowCount;

                for ( int i=0; i<array.size(); ++i ) {
                    DerivedMetricInformation info;
                    info.read( array[i].toObject() );

                    if ( ! solver->insert( info.nameDescription(), info.formula(), info.enabled() ) )
                        continue;

                    // add new row to table
                    ui->tableWidget_DerivedMetricDefinitions->insertRow( rowCount );

                    // insert "name/description" and "formula" column items
                    ui->tableWidget_DerivedMetricDefinitions->setItem( rowCount, 0, new QTableWidgetItem( info.nameDescription() ) );
                    ui->tableWidget_DerivedMetricDefinitions->setItem( rowCount, 1, new QTableWidgetItem( info.formula() ) );

                    // construct and initialize checkbox and insert as last column item
                    QCheckBox* checkbox = new QCheckBox;
                    checkbox->setObjectName( info.nameDescription() );
                    checkbox->setChecked( info.enabled() );

                    m_mapper->setMapping( checkbox, checkbox );
                    connect( checkbox, SIGNAL(clicked(bool)), m_mapper, SLOT(map()) );

                    ui->tableWidget_DerivedMetricDefinitions->setCellWidget( rowCount, 2, checkbox );

                    ++rowCount;
                }

                if ( startingRowCount != rowCount && -1 == m_userDefinedStartIndex ) {
                    m_userDefinedStartIndex = startingRowCount;
                }
            }
        }
    }
#endif
}

/**
 * @brief DerivedMetricInformationDialog::handleSaveUserDefinedMetric
 *
 * This method handles the 'Save User Derived Metrics' context-menu item triggered action.
 */
void DerivedMetricInformationDialog::handleSaveUserDefinedMetric()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    if ( -1 == m_userDefinedStartIndex )
        return;

    const QString directoryPath = m_directoryPath.isEmpty() ? QApplication::applicationDirPath() : m_directoryPath;

    const QString fileName = QFileDialog::getSaveFileName( this, tr("Save User Derived Metrics XML File"), directoryPath, tr("XML Files (*.xml)") );

    QFile file( fileName );

    if ( file.open( QIODevice::WriteOnly|QIODevice::Truncate ) ) {

        // construct derived metric solver on stack
        DerivedMetricsSolver* solver = DerivedMetricsSolver::instance();

        QJsonArray array;

        for ( int i=m_userDefinedStartIndex; i<ui->tableWidget_DerivedMetricDefinitions->rowCount(); ++i ) {
            QString name;
            QString formula;
            bool enabled;

            if ( solver->getUserDefined( i-m_userDefinedStartIndex, name, formula, enabled ) ) {
                DerivedMetricInformation info( name, formula, enabled );

                QJsonObject element;
                info.write( element );

                array.append( element );
            }
        }

        QJsonObject root;

        root["metrics"] = array;

        QJsonDocument doc( root );

        file.write( doc.toJson() );
    }
#endif
}


} // GUI
} // ArgoNavis

/*!
   \file PerformanceDataGraphView.cpp
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

#include "PerformanceDataGraphView.h"
#include "ui_PerformanceDataGraphView.h"

#include "managers/PerformanceDataManager.h"
#include "QCustomPlot/CustomPlot.h"

#include <ui_PerformanceDataGraphView.h>

#include <QVector>

#include <cmath>
#include <random>


namespace ArgoNavis { namespace GUI {


// golden ratio conjugate value
const double GOLDEN_RATIO_CONJUGATE = 0.618033988749895;

// initialize random number generator using uniform distribution
static std::mt19937 m_mt( 2560000 );  // use constant seed whose initial sequence of values seemed to generate good colors for small rank counts
static std::uniform_real_distribution<double> m_dis( 0.0, 1.0 );

const int X_AXIS_LEGEND_ITEM_MAX_LENGTH = 200;

/**
 * @brief PerformanceDataGraphView::INIT_Y_AXIS_GRAPH_LABELS
 * @return - initialized map of metric name to y-axis graph label
 */
QMap< QString, QPair< QString, bool > > INIT_Y_AXIS_GRAPH_LABELS()
{
   QMap< QString, QPair< QString, bool > > map;

   map.insert( QStringLiteral("highwater_inclusive_details"), qMakePair( QStringLiteral("Memory Allocation (bytes)"), true ) );
   map.insert( QStringLiteral("leaked_inclusive_details"), qMakePair( QStringLiteral("Memory Allocation (bytes)"), true ) );
   map.insert( QStringLiteral("exclusive_time"), qMakePair( QStringLiteral("Exclusive Time (msec)"), false ) );
   map.insert( QStringLiteral("inclusive_time"), qMakePair( QStringLiteral("Inclusive Time (msec)"), false ) );

   return map;
}

// map of metric names to QPair where 'first' is the y-axis label and 'second' is a flag in which 'true' indicates a line graph (otherwise bar graph)
static QMap< QString, QPair< QString, bool > > s_Y_AXIS_GRAPH_LABELS = INIT_Y_AXIS_GRAPH_LABELS();


/**
 * @brief PerformanceDataGraphView::PerformanceDataGraphView
 * @param parent - specify parent of the PerformanceDataGraphView instance
 *
 * Constructs a widget which is a child of parent.  If parent is 0, the new widget becomes a window.  If parent is another widget,
 * this widget becomes a child window inside parent. The new widget is deleted when its parent is deleted.
 */
PerformanceDataGraphView::PerformanceDataGraphView(QWidget *parent)
    : QWidget( parent )
    , ui( new Ui::PerformanceDataGraphView )
{
    ui->setupUi( this );

    // set document mode on tab widget
    ui->graphView->setDocumentMode( true );

#if defined(HAS_QCUSTOMPLOT_V2)
    connect( ui->graphView, SIGNAL(currentChanged(int)), this, SLOT(handleTabChanged(int)) );
#endif

    // connect performance data manager signals to performance data view slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, static_cast<void(PerformanceDataManager::*)(const QString &metricName, const QString &eventName, const QString &name, double data)>(&PerformanceDataManager::addGraphItem),
                 this, static_cast<void(PerformanceDataGraphView::*)(const QString &metricName, const QString &eventName, const QString &name, double data)>(&PerformanceDataGraphView::handleAddGraphItem) );
        connect( dataMgr, static_cast<void(PerformanceDataManager::*)(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName, double eventTime, double eventData, int rankOrThread)>(&PerformanceDataManager::addGraphItem),
                 this, static_cast<void(PerformanceDataGraphView::*)(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName, double eventTime, double eventData, int rankOrThread)>(&PerformanceDataGraphView::handleAddGraphItem) );
        connect( dataMgr, &PerformanceDataManager::createGraphItems, this, &PerformanceDataGraphView::handleInitGraphView );
        connect( dataMgr, &PerformanceDataManager::requestMetricViewComplete, this, &PerformanceDataGraphView::handleRequestMetricViewComplete, Qt::QueuedConnection );
        connect( dataMgr, &PerformanceDataManager::signalGraphMinAvgMaxRanks, this, &PerformanceDataGraphView::handleGraphMinAvgMaxRanks, Qt::QueuedConnection );
#else
        connect( dataMgr, SIGNAL(addGraphItem(QString,QString,QString,double,double,int)),
                 this, SLOT(handleAddGraphItem(QString,QString,QString,double,double,int)) );
        connect( dataMgr, SIGNAL(addGraphItem(QString,QString,QString,double)),
                 this, SLOT(handleAddGraphItem(QString,QString,QString,double)) );
        connect( dataMgr, SIGNAL(createGraphItems(QString,QString,QString,QStringList,QStringList)),
                 this, SLOT(handleInitGraphView(QString,QString,QString,QStringList,QStringList)) );
        connect( dataMgr, SIGNAL(requestMetricViewComplete(QString,QString,QString,QString,double,double)),
                 this, SLOT(handleRequestMetricViewComplete(QString,QString,QString,QString,double,double)) );
        connect( dataMgr, SIGNAL(signalGraphMinAvgMaxRanks(QString,int,int,int)),
                 this, SLOT(handleGraphMinAvgMaxRanks(QString,int,int,int)) );
#endif
    }
}

/**
 * @brief PerformanceDataGraphView::~PerformanceDataGraphView
 *
 * Destroys the PerformanceDataGraphView instance.
 */
PerformanceDataGraphView::~PerformanceDataGraphView()
{
    delete ui;
}

/**
 * @brief PerformanceDataGraphView::unloadExperimentDataFromView
 * @param experimentName - the name of the experiment to remove from view
 *
 * Removes the given experiment data from view.
 */
void PerformanceDataGraphView::unloadExperimentDataFromView(const QString &experimentName)
{
    Q_UNUSED( experimentName )  // for now until view supports more than one experiment

    foreach ( const MetricGroup& group, m_metricGroup ) {
        group.graph->clearGraphs();

        ui->graphView->removeTab( 0 );

        delete group.graph;
    }

    m_metricGroup.clear();
}

/**
 * @brief PerformanceDataGraphView::initPlotView
 * @param clusteringCriteriaName - the name of the metric group
 * @param metricNameTitle - the displayed metric name (for graph title on tab widget)
 * @param metricName - the metric name
 * @param handleGraphRangeChanges - flag indicating whether x-axis graph ranges changes should be handled
 * @return - the new QCustomPlot instance
 *
 * Creates a QCustomPlot instance and initializes the desired style properties for the axes of the metric graphs.
 */
CustomPlot *PerformanceDataGraphView::initPlotView(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName, bool handleGraphRangeChanges)
{
    CustomPlot* graphView = new CustomPlot( this );

    if ( ! graphView )
        return Q_NULLPTR;

    // add QCustomPlot instance to tab widget
    ui->graphView->addTab( graphView, metricNameTitle );

    graphView->setObjectName( metricName );

#ifdef QCUSTOMPLOT_USE_OPENGL
    graphView->setOpenGl( true );
    qDebug() << Q_FUNC_INFO << "openGl()=" << graphView->openGl();
#endif

    graphView->setNoAntialiasingOnDrag( true );
    graphView->setAutoAddPlottableToLegend( false );
    graphView->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom  | QCP::iSelectPlottables | QCP::iSelectLegend );

    // connect slot that ties some axis selections together (especially opposite axes):
    connect( graphView, SIGNAL(selectionChangedByUser()), this, SLOT(handleSelectionChanged()) );

    // get axis rect for this metric
    QCPAxisRect *axisRect = graphView->axisRect();

    if ( ! axisRect )
        return Q_NULLPTR;

    // legend is located at the top left of axis rect
    axisRect->insetLayout()->setInsetAlignment( 0, Qt::AlignLeft|Qt::AlignTop );

    // initialize legend
    QCPLegend *legend( graphView->legend );
    legend->setVisible( true );
    legend->setFont( QFont( "Helvetica", 8 ) );
    legend->setBrush( QColor(180, 180, 180, 180) );
    legend->setBorderPen( Qt::NoPen );
    legend->setSelectedIconBorderPen( Qt::NoPen );
    // set selected font to be the same as the regular font
    legend->setSelectedFont( legend->font() );
    // set selected legend item text color to red
    legend->setSelectedTextColor( Qt::red );

    QLinearGradient plotGradient;
    plotGradient.setStart( 0, 0 );
    plotGradient.setFinalStop( 0, 350 );
    plotGradient.setColorAt( 0, QColor(100, 100, 100) );
    plotGradient.setColorAt( 1, QColor(80, 80, 80) );
    graphView->setBackground( plotGradient );

    QLinearGradient axisRectGradient;
    axisRectGradient.setStart( 0, 0 );
    axisRectGradient.setFinalStop( 0, 350 );
    axisRectGradient.setColorAt( 0, QColor(100, 100, 100) );
    axisRectGradient.setColorAt( 1, QColor(60, 60, 60) );
    axisRect->setBackground( axisRectGradient );

    // set fixed height on graph
    setFixedHeight( 400 + X_AXIS_LEGEND_ITEM_MAX_LENGTH );

    // get x and y axes
    QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
    QCPAxis* yAxis = axisRect->axis( QCPAxis::atLeft );

    // specify which axes can be dragged or zoomed - for these graph views just the x-axis
    axisRect->setRangeDragAxes( xAxis, NULL );
    axisRect->setRangeZoomAxes( xAxis, NULL );

    // prepare x axis
    if ( xAxis ) {
#if defined(HAS_QCUSTOMPLOT_V2)
        // create ticker
        QSharedPointer< QCPAxisTickerText > ticker( new QCPAxisTickerText );
        xAxis->setTicker( ticker );
#else
        xAxis->setAutoTicks( false );
        xAxis->setAutoTickLabels( false );
        xAxis->setAutoTickStep( false );
        xAxis->setAutoSubTicks( false );
#endif
        QFont font;
        font.setFamily( "arial" );
        font.setBold( true );
        font.setPixelSize( 14 );
        xAxis->setTickLabelFont( font );
        xAxis->setPadding( 20 ); // a bit more space to the bottom border

        // set some pens, brushes and backgrounds
        xAxis->setBasePen( QPen(Qt::white, 1) );
        xAxis->setTickPen( QPen(Qt::white, 1) );
        xAxis->setSubTickPen( QPen(Qt::white, 1) );
        xAxis->setTickLabelColor( Qt::white );
        xAxis->grid()->setPen( QPen(QColor(140, 140, 140), 1, Qt::DotLine) );
        xAxis->grid()->setSubGridPen( QPen(QColor(80, 80, 80), 1, Qt::DotLine) );
        xAxis->grid()->setSubGridVisible( true );
        xAxis->grid()->setZeroLinePen( Qt::NoPen );

        // set associated metric group property
        xAxis->setProperty( "associatedMetricGroup", clusteringCriteriaName );
        // set associated cluster name property
        xAxis->setProperty( "associatedClusterName", metricNameTitle );
        // set metric name property
        xAxis->setProperty( "metricName", metricName );

        // X axis always visible
        xAxis->setVisible( true );

        // set X axis graph lower range
        // NOTE: If not defined here, the full range needs to be specified again after loading and processing
        // experiment data for default view.  This is accomplished via the 'handleRequestMetricViewComplete' signal.
        xAxis->setRangeLower( -1.0 );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        // connect slot to handle value axis range changes and regenerate the tick marks appropriately
        connect( xAxis, static_cast<void(QCPAxis::*)(const QCPRange &newRange)>(&QCPAxis::rangeChanged),
                 this, &PerformanceDataGraphView::handleAxisRangeChange );
#else
        connect( xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(handleAxisRangeChange(QCPRange)) );
#endif
    }

    // prepare y axis
    if ( yAxis ) {
        QFont font;
        font.setFamily( "arial" );
        font.setBold( true );
        font.setPixelSize( 12 );

        yAxis->setLabelFont( font );
        yAxis->setLabelColor( Qt::white );

        // set some pens, brushes and backgrounds
        yAxis->setBasePen( QPen(Qt::white, 1) );
        yAxis->setTickPen( QPen(Qt::white, 1) );
        yAxis->setSubTickPen( QPen(Qt::white, 1) );
        yAxis->setTickLabelColor( Qt::white );
        yAxis->grid()->setPen( QPen(QColor(140, 140, 140), 1, Qt::DotLine) );
        yAxis->grid()->setSubGridPen( QPen(QColor(80, 80, 80), 1, Qt::DotLine) );
        yAxis->grid()->setSubGridVisible( true );
        yAxis->grid()->setZeroLinePen( Qt::NoPen );

        yAxis->setVisible( true );
        yAxis->setPadding( 5 ); // a bit more space to the left border

#if defined(HAS_QCUSTOMPLOT_V2)
        yAxis->setTickLabels( true );
        yAxis->setTicks( true );
        // create ticker
        QSharedPointer< QCPAxisTickerText > ticker( new QCPAxisTickerText );
        yAxis->setTicker( ticker );
#else
        yAxis->setAutoTicks( false );
        yAxis->setAutoTickLabels( false );
        yAxis->setAutoTickStep( false );
        yAxis->setAutoTickCount( 10 );
#endif

        // set the y-axis label based on the metric specified
        if ( s_Y_AXIS_GRAPH_LABELS.contains( metricName ) ) {
            yAxis->setLabel( s_Y_AXIS_GRAPH_LABELS[ metricName ].first );
        }
    }

    return graphView;
}

/**
 * @brief PerformanceDataGraphView::initGraph
 * @param plot - the QCustomPlot instance
 * @param rankOrThread - the rank or thread id associated with the graph instance to create
 * @return - the new graph instance (QCPGraph)
 *
 * This method create a new graph instance (QCPGraph) for the specified rank or thread id,
 * sets an unique color for the graph line and returns the graph instance.
 */
QCPGraph* PerformanceDataGraphView::initGraph(CustomPlot* plot, int rankOrThread)
{
    QCPGraph* graph = plot->addGraph();

    if ( graph ) {
        // set graph name to rank #
        graph->setName( QString("Rank %1").arg( rankOrThread ) );
        // set plot colors for new graph
        graph->setPen( QPen( goldenRatioColor(), 2.0 ) );
#if defined(HAS_QCUSTOMPLOT_V2)
        graph->setSelectable( QCP::stWhole );
        QCPSelectionDecorator *decorator = new QCPSelectionDecorator;
        decorator->setPen( QPen( Qt::red, 2.5 ) );
        graph->setSelectionDecorator( decorator );
#else
        // set graph selected color to red
        graph->setSelectedPen( QPen( Qt::red, 4.0 ) );
#endif
        // add graph to legend
        if ( plot->graphCount() < 4 || rankOrThread == 0 ) {
            graph->addToLegend();
        }
    }

    return graph;
}

/**
 * @brief PerformanceDataGraphView::goldenRatioColor
 * @return - color generated using golden ratio
 *
 * Generate color using golden ratio.
 * Reference: "https://en.wikipedia.org/wiki/Golden_ratio" and
 *            "https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically".
 */
QColor PerformanceDataGraphView::goldenRatioColor() const
{
    double iptr;  // for integral portion (ignored)

    double h = std::modf( m_dis( m_mt ) + GOLDEN_RATIO_CONJUGATE, &iptr );

    return QColor::fromHsvF( h, 0.3, 0.99 );
}

/**
 * @brief PerformanceDataGraphView::normalizedName
 * @param name - the name of the event item (the full defined location)
 * @param plot - the QCustomPlot instance
 * @return - the name of the function without any parameter or return type information or source-code location information
 *
 * This function generates a shortened name from the full defined location name.
 */
QString PerformanceDataGraphView::normalizedName(const QString& name, const QCustomPlot *plot) const
{
    const int pindex = name.indexOf( QRegExp("[<(]") );
    const int sindex = name.lastIndexOf( ' ', pindex-2 ) + 1;

    const QString functionName = name.mid( sindex, pindex );

    QFontMetrics fontMetric( plot->font() );

    return fontMetric.elidedText( functionName, Qt::ElideRight, X_AXIS_LEGEND_ITEM_MAX_LENGTH );
}

/**
 * @brief PerformanceDataGraphView::handleSelectionChanged
 *
 * Process plottable or legend item selection changes.  If a plottable is selected then corresponding
 * legend is selected.  If the legend isn't present, it is added.  If a legend item is selected.
 * the corresponding plottable is selected.
 */
void PerformanceDataGraphView::handleSelectionChanged()
{
    // get the sender instance - should be a QCustomPlot instance
    QCustomPlot* graphView = qobject_cast< QCustomPlot* >( sender() );

    if ( ! graphView )
        return;

    const QString metricName = graphView->objectName();

    if ( ! m_metricGroup.contains( metricName ) )
        return;

    MetricGroup& metricGroup = m_metricGroup[ metricName ];

    const int graphCount = graphView->plottableCount();

    // keep the number of additional items added to the legend to be just one, so we just added a legend
    // item and there was one already added, then remove the previous one added.  Previous will be removed
    // also if the user cleared selection.
    if ( metricGroup.legendItemAdded ) {
        graphView->legend->removeAt( graphView->legend->itemCount()-1 );
        metricGroup.legendItemAdded = false;
    }

    // synchronize selection of graphs with selection of corresponding legend items
    for ( int i=0; i<graphCount; ++i ) {
        QCPAbstractPlottable* graph = graphView->plottable( i );
        if ( graph ) {
            QCPPlottableLegendItem *item = graphView->legend->itemWithPlottable( graph );
            // if graph selected then set corresponding legend item
            if ( graph->selected() ) {
                // is selected graph not included in the legend?  if not then add it
                if ( ! item ) {
                    metricGroup.legendItemAdded = graph->addToLegend();
                    if ( metricGroup.legendItemAdded ) {
                        item = graphView->legend->itemWithPlottable( graph );
                    }
                }
                if ( item ) {
                    item->setSelected( true );
                }
                break; // exit once the one selection was processed
            }
            // if legend item selected then set corresponding graph
            else if ( item && item->selected() ) {
#if defined(HAS_QCUSTOMPLOT_V2)
                QCPDataRange range;
                range.setBegin( 0 );
                range.setEnd( graphView->axisRect()->axis( QCPAxis::atBottom )->range().upper );
                QCPDataSelection selection( range );
                graph->setSelection( selection );
#else
                graph->setSelected( true );
#endif
                break; // exit once the one selection was processed
            }
        }
    }
}

/**
 * @brief PerformanceDataGraphView::handleInitGraphView
 * @param clusteringCriteriaName - the name of the metric group
 * @param metricNameTitle - the displayed metric name (for graph title on tab widget)
 * @param metricName - the metric name
 * @param eventNames - the list of event names (ie PAPI event names)
 * @param items - the list of items (ie function names)
 *
 * This function initializes the metric group information applicable to graph views with bar graphs.
 */
void PerformanceDataGraphView::handleInitGraphView(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName, const QStringList &eventNames, const QStringList &items)
{
    CustomPlot* graphView = initPlotView( clusteringCriteriaName, metricNameTitle, metricName, false );

    if ( ! graphView )
        return;

    if ( m_metricGroup.insert( metricName, MetricGroup( graphView ) ) == m_metricGroup.end() )
        return;

    MetricGroup& metricGroup = m_metricGroup[ metricName ];

    foreach ( const QString& item, items ) {
        metricGroup.items << normalizedName( item, graphView );
    }

    QVector< double > mTickVector( metricGroup.items.size() );
    QVector< QString > mTickLabelVector( metricGroup.items.size() );

#ifdef HAS_STACKED_BAR_GRAPHS
    const double FACTOR = 1.0;
#else
    const double FACTOR = 10.0;
#endif

    for ( int i=0; i<metricGroup.items.size(); ++i ) {
        mTickVector[i] = i * FACTOR;
        mTickLabelVector[i] = metricGroup.items[ i ];
    }

    graphView->xAxis->setRangeUpper( FACTOR * metricGroup.items.size() + 1 );

    metricGroup.xGraphRange = graphView->xAxis->range();

#if defined(HAS_QCUSTOMPLOT_V2)
    // get the ticker shared pointer
    QSharedPointer< QCPAxisTicker > axisTicker = graphView->xAxis->ticker();

    // cast to QCPAxisTickerFixed instance
    QCPAxisTickerText* ticker = dynamic_cast< QCPAxisTickerText* >( axisTicker.data() );

    if ( ticker ) {
        ticker->setTicks( mTickVector, mTickLabelVector );
    }
#else
    graphView->xAxis->setTickVector( mTickVector );
    graphView->xAxis->setTickVectorLabels( mTickLabelVector );
    graphView->xAxis->setTickLabelRotation( 60 );
#endif

#ifdef HAS_STACKED_BAR_GRAPHS
    QCPBars* lastBar( Q_NULLPTR );
#else
    QCPBarsGroup* group = new QCPBarsGroup( graphView );
#endif

    foreach ( const QString& eventName, eventNames ) {
        // create empty bar chart objects
        QCPBars *bar = new QCPBars( graphView->xAxis, graphView->yAxis );
        if ( bar ) {
            bar->setAntialiased( false );
            // set names and colors
            bar->setName( eventName );
            const QColor color = goldenRatioColor();
            bar->setPen( QPen( color ) );
            bar->setBrush( color );
            bar->setWidth( 1.0 );
#ifdef HAS_STACKED_BAR_GRAPHS
            if ( lastBar ) {
                bar->moveBelow( lastBar );
            }
            // save instance pointer so next bar can be stacked below
            lastBar = bar;
#else
            bar->setBarsGroup( group );
#endif
            bar->addToLegend();
            metricGroup.bars.insert( eventName, bar );
        }
    }
}

/**
 * @brief PerformanceDataGraphView::handleAxisRangeChange
 * @param requestedRange - the range of this axis due to change
 *
 * Handle changes to x axis ranges.  Make sure any range change requests made, mainly from user axis range drag and zoom actions,
 * are keep within the valid range of the metric group.  The tick locations and label vectors are computed also.
 */
void PerformanceDataGraphView::handleAxisRangeChange(const QCPRange &requestedRange)
{
    // get the sender axis
    QCPAxis* xAxis = qobject_cast< QCPAxis* >( sender() );

    if ( Q_NULLPTR == xAxis )
        return;

    // get data range of the metric group
    const QString metricName = xAxis->property( "metricName" ).toString();

    QMutexLocker guard( &m_mutex );

    if ( ! m_metricGroup.contains( metricName ) )
        return;

    const QCPRange dataRange = m_metricGroup[ metricName ].xGraphRange;

    // clamp requested range to dataRange.lower .. dataRange.upper
    double lower = qMax( requestedRange.lower, dataRange.lower );
    double upper = qMin( requestedRange.upper, dataRange.upper );

    const double minXspread = 2.0;

    const bool isLineGraph = s_Y_AXIS_GRAPH_LABELS.contains( metricName ) && s_Y_AXIS_GRAPH_LABELS[ metricName ].second;

    // only maintain x-axis lower/upper range spread for line graphs
    if ( isLineGraph ) {
        // but may need to adjust lower or upper bound to maintain minimum graph range
        if ( upper - lower < minXspread ) {
            if ( upper - minXspread > dataRange.lower )
                lower = upper - minXspread;
            else
                upper = lower + minXspread;
        }
    }

    // temporarily block signals for X Axis
    xAxis->blockSignals( true );

    xAxis->setRange( lower, upper );

    // only regenerate ticks for line graphs
    if ( isLineGraph ) {
        QCPRange newRange = xAxis->range();

        // Generate tick positions according to linear scaling:
        double mTickStep = newRange.size() / (double)( 10.0 + 1e-10 ); // mAutoTickCount ticks on average,
        double magnitudeFactor = qPow( 10.0, qFloor( qLn(mTickStep) / qLn(10.0) ) ); // get magnitude fact
        double tickStepMantissa = mTickStep / magnitudeFactor;

        mTickStep = qCeil( tickStepMantissa ) * magnitudeFactor;
        mTickStep = qMax( 1.0, mTickStep );

        // Generate tick positions according to mTickStep:
        qint64 firstStep = floor( newRange.lower / mTickStep ); // do not use qFloor here, or we'll lose 6
        qint64 lastStep = qMin( dataRange.upper, ceil( newRange.upper / mTickStep ) ); // do not use qCeil

        int tickcount = qMax( 0LL, lastStep - firstStep + 1 );

        QVector< double > mTickVector;
        QVector< QString > mTickLabelVector;

        mTickVector.resize( tickcount );
        mTickLabelVector.resize( tickcount );

        for ( int i=0; i<tickcount; ++i ) {
            double tickValue = ( firstStep + i ) * mTickStep;
            mTickVector[i] = tickValue;
            double tickLabelValue = tickValue;
#if defined(USE_DISCRETE_SAMPLES)
            tickLabelValue *= 10.0;
#endif
            mTickLabelVector[i] = QString::number( tickLabelValue, 'f', 0 );
        }

#if defined(HAS_QCUSTOMPLOT_V2)
        // get the ticker shared pointer
        QSharedPointer< QCPAxisTicker > axisTicker = xAxis->ticker();

        // cast to QCPAxisTickerFixed instance
        QCPAxisTickerText* ticker = dynamic_cast< QCPAxisTickerText* >( axisTicker.data() );

        if ( ticker ) {
            ticker->setTicks( mTickVector, mTickLabelVector );
        }
#else
        xAxis->setSubTickCount( qMax( 1.0, (qreal) qCeil( tickStepMantissa ) ) - 1 );

        xAxis->setTickVector( mTickVector );
        xAxis->setTickVectorLabels( mTickLabelVector );
#endif
    }

    // unblock signals for X Axis
    xAxis->blockSignals( false );
}

/**
 * @brief PerformanceDataGraphView::handleAddGraphItem
 * @param clusteringCriteriaName - the clustering criteria name
 * @param metricNameTitle - the displayed metric name (for graph title on tab widget)
 * @param metricName - the metric name
 * @param eventTime - the time of the event
 * @param eventData - the metric data for the event
 * @param rankOrThread - the rank or thread id in which the trace event occurred
 *
 * This method handles adding a graph item to the graph (if it hasn't been created yet) and then adds the data to the graph.
 */
void PerformanceDataGraphView::handleAddGraphItem(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName, double eventTime, double eventData, int rankOrThread)
{
    Q_UNUSED( rankOrThread );

    CustomPlot* graphView( Q_NULLPTR);

    {
        QMutexLocker guard( &m_mutex );

        if ( ! m_metricGroup.contains( metricName ) ) {

            graphView = initPlotView( clusteringCriteriaName, metricNameTitle, metricName );

            m_metricGroup.insert( metricName, MetricGroup( graphView ) );
        }
        else {
            // get graph index for cluster
            graphView = m_metricGroup[ metricName ].graph;
        }

        MetricGroup& metricGroup = m_metricGroup[ metricName ];

        if ( metricGroup.yGraphRange.upper < eventData ) {
            metricGroup.yGraphRange.upper = eventData;
        }

        QCPGraph* graph( Q_NULLPTR );

        if ( metricGroup.subgraphs.contains( rankOrThread ) ) {
            graph = metricGroup.subgraphs[ rankOrThread ];
        }
        else {
            graph = initGraph( graphView, rankOrThread );

            metricGroup.subgraphs.insert( rankOrThread, graph );
        }
        if ( graph ) {
            // pass data points to graphs
            graph->addData( eventTime, eventData );
        }
    }
}

/**
 * @brief PerformanceDataGraphView::handleAddGraphItem
 * @param metricName - the metric name
 * @param eventName - the name of the event
 * @param name - the name of the individually graphed item along x-axis
 * @param data - the item's data for the event
 *
 * This method handles adding the graph data for the specified event and item.
 */
void PerformanceDataGraphView::handleAddGraphItem(const QString &metricName, const QString &eventName, const QString &name, double data)
{
    QMutexLocker guard( &m_mutex );

    if ( ! m_metricGroup.contains( metricName ) )
        return;

    MetricGroup& metricGroup = m_metricGroup[ metricName ];

    if ( metricGroup.yGraphRange.upper < data ) {
        metricGroup.yGraphRange.upper = data;
    }

    const QString elidedName = normalizedName( name, metricGroup.graph );

    if ( ! metricGroup.bars.contains( eventName ) || ! metricGroup.items.contains( elidedName ) )
        return;

    QCPBars* bar = metricGroup.bars[ eventName ];

    if ( bar ) {
#ifdef HAS_STACKED_BAR_GRAPHS
        const double key = (double) metricGroup.items.indexOf( elidedName );
#else
        const double key = (double) metricGroup.items.indexOf( elidedName ) * 10;
#endif

        bar->addData( key, data );
    }
}

/**
 * @brief PerformanceDataGraphView::handleGraphMinAvgMaxRanks
 * @param metricName - the metric name
 * @param rankWithMinValue - rank containing smallest metric value for graph
 * @param rankClosestToAvgValue - rank closest to averagec metric value for graph
 * @param rankWithMaxValue - rank containing largest metric value for graph
 *
 * This slot handles the PerformanceDataManager::signalGraphMinAvgMaxRanks signal.  If this signal is emitted only the unique set of ranks
 * identified by the parameters (rank with minimum value, rank closest to the average value and the rank with maximum value) as well as
 * rank 0 are shown in the graph.  All other graphs that may have been added for other ranks are removed.  The legend will show all these
 * ranks.
 */
void PerformanceDataGraphView::handleGraphMinAvgMaxRanks(const QString &metricName, int rankWithMinValue, int rankClosestToAvgValue, int rankWithMaxValue)
{
    if ( metricName.isEmpty() )
        return;

    QMutexLocker guard( &m_mutex );

    if ( ! m_metricGroup.contains( metricName ) )
        return;

    MetricGroup& metricGroup = m_metricGroup[ metricName ];

    QCustomPlot* graphView = metricGroup.graph;

    if ( graphView ) {

        QSet< int > desiredRankSet;
        desiredRankSet << 0 << rankWithMinValue << rankClosestToAvgValue << rankWithMaxValue;

        for ( int i=graphView->graphCount(); i>=0; --i ) {
            if ( metricGroup.subgraphs.contains( i ) ) {
                QCPGraph* graph = metricGroup.subgraphs[ i ];
                if ( ! desiredRankSet.contains( i ) ) {
                    graph->removeFromLegend();
                    graphView->removeGraph( graph );
                }
                else {
                    // change name of graph
                    if ( i == rankWithMinValue ) {
                        graph->setName( QString("Rank %1 (Min)").arg( i ) );
                    }
                    else if ( i == rankClosestToAvgValue ) {
                        graph->setName( QString("Rank %1 (Avg)").arg( i ) );
                    }
                    else if ( i == rankWithMaxValue ) {
                        graph->setName( QString("Rank %1 (Max)").arg( i ) );
                    }
                    // add legend items only for the desired rank set
                    graph->addToLegend();
                }
            }
        }

        // force graph replot
#if defined(HAS_QCUSTOMPLOT_V2)
        graphView->replot( QCustomPlot::rpQueuedReplot );
#else
        graphView->replot();
#endif
    }
}

/**
 * @brief PerformanceDataGraphView::handleRequestMetricViewComplete
 * @param clusteringCriteriaName - the name of the cluster criteria
 * @param modeName - the mode name
 * @param metricName - name of metric view for which to add data to model
 * @param viewName - name of the view for which to add data to model
 * @param lower - lower value of range to actually view
 * @param upper - upper value of range to actually view
 *
 * Once a signal 'requestMetricViewComplete' is emitted, this handler of the signal will insure the plot is updated.
 */
void PerformanceDataGraphView::handleRequestMetricViewComplete(const QString &clusteringCriteriaName, const QString &modeName, const QString &metricName, const QString &viewName, double lower, double upper)
{
    if ( clusteringCriteriaName.isEmpty() || modeName.isEmpty() || viewName.isEmpty() )
        return;

    const QCPRange range( lower, upper );

    CustomPlot* graphView( Q_NULLPTR );

    {
        QMutexLocker guard( &m_mutex );

        if ( ! m_metricGroup.contains( metricName ) )
            return;

        MetricGroup& metricGroup = m_metricGroup[ metricName ];

        graphView = metricGroup.graph;

        // only update xGraphRange here if the x-axis is the experiment time such as in the line graphs
        if ( s_Y_AXIS_GRAPH_LABELS.contains( metricName ) && s_Y_AXIS_GRAPH_LABELS[ metricName ].second ) {
            metricGroup.xGraphRange = range;
        }
    }

    if ( ! graphView )
        return;

    // create axis rect for this metric
    QCPAxisRect *axisRect = graphView->axisRect();

    if ( axisRect ) {
        // set the y-axis graph range
        QCPAxis* yAxis = axisRect->axis( QCPAxis::atLeft );
        if ( yAxis ) {
            QMutexLocker guard( &m_mutex );

            MetricGroup& metricGroup = m_metricGroup[ metricName ];

            // determine the largest y-axis value
            double largestYAxisValue;

#ifdef HAS_STACKED_BAR_GRAPHS
            if ( metricGroup.bars.size() == 0 ) {
                // for line graphs, the maximum has been maintained in the 'yGraphRange' variable
                largestYAxisValue = metricGroup.yGraphRange.upper;
            }
            else if ( metricGroup.bars.begin().value()->data()->size() > 0 ) {
                // for bar graphs, compute the total value for all stacked bars
                std::vector< double > totals( metricGroup.bars.begin().value()->data()->size(), 0.0 );
                for ( QMap< QString, QCPBars* >::iterator biter = metricGroup.bars.begin(); biter != metricGroup.bars.end(); biter++ ) {
                    QCPBars* map( biter.value() );
                    for ( QCPBarDataMap::iterator iter=map->data()->begin(); iter != map->data()->end(); iter++ ) {
                        QCPBarData barData( *iter );
                        totals[ (int) barData.key ] += barData.value;
                    }
                }

                // the largest y-axis value is the largest value of all stacked bars
                largestYAxisValue = *( std::max_element( totals.begin(), totals.end() ) );
            }
#else
            // for line graphs, the maximum has been maintained in the 'yGraphRange' variable
            largestYAxisValue = metricGroup.yGraphRange.upper;
#endif

            const int NUM_SIGNIFICANT_DIGITS = qCeil( std::log10( largestYAxisValue ) );
            const double MAX_YAXIS_VALUE = qCeil( largestYAxisValue / qPow( 10.0, NUM_SIGNIFICANT_DIGITS - 1 ) ) * qPow( 10.0, NUM_SIGNIFICANT_DIGITS - 1 );

            yAxis->setRange( QCPRange( 0.0, MAX_YAXIS_VALUE ) );
            yAxis->setTicks( true );

            QVector<double> tickValues;
            QVector<QString> tickLabels;

            for (int i=0; i<=10; ++i) {
                const double value( i * std::round( MAX_YAXIS_VALUE / 10.0 ) );
                tickValues << value;
                tickLabels << QString::number( value, 'f', 0 );
            }

#if defined(HAS_QCUSTOMPLOT_V2)
            // get the ticker shared pointer
            QSharedPointer< QCPAxisTicker > axisTicker = yAxis->ticker();

            // cast to QCPAxisTickerFixed instance
            QCPAxisTickerText* ticker = dynamic_cast< QCPAxisTickerText* >( axisTicker.data() );

            if ( ticker ) {
                ticker->setTicks( tickValues, tickLabels );
            }
#else
            yAxis->setTickVector( tickValues );
            yAxis->setTickVectorLabels( tickLabels );
#endif
        }
        if ( QStringLiteral("Trace") == modeName && QStringLiteral("All Events") == viewName ) {
            // set the x-axis graph range
            QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
            if ( xAxis ) {
                xAxis->setRange( range );
            }
        }
        // force graph replot
#if defined(HAS_QCUSTOMPLOT_V2)
        graphView->replot( QCustomPlot::rpQueuedReplot );
#else
        graphView->replot();
#endif
    }
}

#if defined(HAS_QCUSTOMPLOT_V2)
/**
 * @brief PerformanceDataGraphView::handleTabChanged
 * @param index - new tab index
 */
void PerformanceDataGraphView::handleTabChanged(int index)
{
    Q_UNUSED( index );

    QCustomPlot* plot = qobject_cast< QCustomPlot* >( ui->graphView->currentWidget() );

    if ( plot ) {
        const QString metricName = plot->objectName();
        if ( m_metricGroup.contains( metricName ) ) {
            m_metricGroup[ metricName ].graph->replot( QCustomPlot::rpImmediateRefresh );
        }
    }
}
#endif


} // GUI
} // ArgoNavis

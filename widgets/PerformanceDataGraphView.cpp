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


namespace ArgoNavis { namespace GUI {


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

    // connect performance data manager signals to performance data view slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, &PerformanceDataManager::addGraphItem, this, &PerformanceDataGraphView::handleAddGraphItem, Qt::QueuedConnection );
        connect( this, &PerformanceDataGraphView::graphRangeChanged, dataMgr, &PerformanceDataManager::graphRangeChanged );
        connect( dataMgr, &PerformanceDataManager::requestMetricViewComplete, this, &PerformanceDataGraphView::handleRequestMetricViewComplete, Qt::QueuedConnection );
#else
        connect( dataMgr, SIGNAL(addGraphItem(QString,QString,QString,double,double,int)),
                 this, SLOT(handleAddGraphItem(QString,QString,QString,double,double,int)) );
        connect( this, SIGNAL(graphRangeChanged(QString,QString,double,double,QSize)),
                 dataMgr, SIGNAL(graphRangeChanged(QString,QString,double,double,QSize)) );
        connect( dataMgr, SIGNAL(requestMetricViewComplete(QString,QString,QString,QString,double,double)),
                 this, SLOT(handleRequestMetricViewComplete(QString,QString,QString,QString,double,double)) );
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
 * @return - the new QCustomPlot instance
 *
 * Creates a QCustomPlot instance and initializes the desired style properties for the axes of the metric graphs.
 */
CustomPlot *PerformanceDataGraphView::initPlotView(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName)
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
    graphView->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom );

    QCPGraph* graph = graphView->addGraph();

    if ( ! graph )
        return Q_NULLPTR;

    // set plot colors for new graph
    graphView->graph( 0 )->setBrush( QColor(10, 100, 50, 70) );
    graphView->graph( 0 )->setPen( QPen(QColor(10, 140, 70, 160)) );

    QLinearGradient plotGradient;
    plotGradient.setStart( 0, 0 );
    plotGradient.setFinalStop( 0, 350 );
    plotGradient.setColorAt( 0, QColor(100, 100, 100) );
    plotGradient.setColorAt( 1, QColor(80, 80, 80) );
    graphView->setBackground( plotGradient );

    // get axis rect for this metric
    QCPAxisRect *axisRect = graphView->axisRect();

    QLinearGradient axisRectGradient;
    axisRectGradient.setStart( 0, 0 );
    axisRectGradient.setFinalStop( 0, 350 );
    axisRectGradient.setColorAt( 0, QColor(100, 100, 100) );
    axisRectGradient.setColorAt( 1, QColor(60, 60, 60) );
    axisRect->setBackground( axisRectGradient );

    // get x and y axes
    QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
    QCPAxis* yAxis = axisRect->axis( QCPAxis::atLeft );

    // prepare x axis
    if ( xAxis ) {
#if defined(HAS_QCUSTOMPLOT_V2)
        // create ticker
        QSharedPointer< QCPAxisTickerFixed > fixedTicker( new QCPAxisTickerFixed );
        fixedTicker->setScaleStrategy( QCPAxisTickerFixed::ssMultiples );
        xAxis->setTicker( fixedTicker );
#else
        xAxis->setAutoTicks( false );
        xAxis->setAutoTickLabels( false );
        xAxis->setAutoTickStep( false );
        xAxis->setAutoSubTicks( false );
#endif
        QFont font;
        font.setFamily( "arial" );
        font.setBold( true );
        font.setPixelSize( 12 );
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

        axisRect->setRangeDragAxes( xAxis, NULL );
        axisRect->setRangeZoomAxes( xAxis, NULL );

        // set associated metric group property
        xAxis->setProperty( "associatedMetricGroup", clusteringCriteriaName );
        // set associated cluster name property
        xAxis->setProperty( "associatedClusterName", metricNameTitle );
        // set metric name property
        xAxis->setProperty( "metricName", metricName );

        // X axis always visible
        xAxis->setVisible( true );

        // set X axis graph lower range
        // NOTE: the full range needs to be specified again after loading and processing experiment data
        // for default view.  This is accomplished via the 'handleRequestMetricViewComplete' signal.
        xAxis->setRangeLower( 0.0 );

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

        setFixedHeight( QWIDGETSIZE_MAX );

#if defined(HAS_QCUSTOMPLOT_V2)
        yAxis->setTickLabels( false );
        yAxis->setTicks( false );
        // create ticker
        QSharedPointer< QCPAxisTickerText > ticker( new QCPAxisTickerText );
        ticker->setTickCount( 1 );
        yAxis->setTicker( ticker );
#else
        yAxis->setAutoTicks( false );
        yAxis->setAutoTickLabels( false );
        yAxis->setAutoTickStep( false );
        yAxis->setAutoTickCount( 10 );
#endif

        yAxis->setLabel( QStringLiteral("Memory Allocation (bytes)") );
    }

    return graphView;
}

/**
 * @brief PerformanceDataTimelineView::PerformanceDataGraphView
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
    const QString clusteringCriteriaName = xAxis->property( "associatedMetricGroup" ).toString();
    const QString clusterName = xAxis->property( "associatedClusterName" ).toString();
    const QString metricName = xAxis->property( "metricName" ).toString();

    QMutexLocker guard( &m_mutex );

    if ( ! m_metricGroup.contains( metricName ) )
        return;

    const QCPRange dataRange = m_metricGroup[ metricName ].xGraphRange;

    const QSize size = xAxis->axisRect()->size();

    // return if the size has zero height or width
    if ( 0 == size.width() || 0 == size.height() )
        return;

    const double minXspread = 2.0;

    // clamp requested range to dataRange.lower .. dataRange.upper
    double lower = qMax( requestedRange.lower, dataRange.lower );
    double upper = qMin( requestedRange.upper, dataRange.upper );

    // but may need to adjust lower or upper bound to maintain minimum graph range
    if ( upper - lower < minXspread ) {
        if ( upper - minXspread > dataRange.lower )
            lower = upper - minXspread;
        else
            upper = lower + minXspread;
    }

    // temporarily block signals for X Axis
    xAxis->blockSignals( true );

    xAxis->setRange( lower, upper );

    emit graphRangeChanged( clusteringCriteriaName, clusterName, lower, upper, size );

    QCPRange newRange = xAxis->range();

    // Generate tick positions according to linear scaling:
    double mTickStep = newRange.size() / (double)( 10.0 + 1e-10 ); // mAutoTickCount ticks on average, the small addition is to prevent jitter on exact integers
    double magnitudeFactor = qPow( 10.0, qFloor( qLn(mTickStep) / qLn(10.0) ) ); // get magnitude factor e.g. 0.01, 1, 10, 1000 etc.
    double tickStepMantissa = mTickStep / magnitudeFactor;

    mTickStep = qCeil( tickStepMantissa ) * magnitudeFactor;
    mTickStep = qMax( 1.0, mTickStep );

    // Generate tick positions according to mTickStep:
    qint64 firstStep = floor( newRange.lower / mTickStep ); // do not use qFloor here, or we'll lose 64 bit precision
    qint64 lastStep = qMin( dataRange.upper, ceil( newRange.upper / mTickStep ) ); // do not use qCeil here, or we'll lose 64 bit precision

    int tickcount = qMax( 0LL, lastStep - firstStep + 1 );

#ifdef HAS_QCUSTOMPLOT_V2
    // get the ticker shared pointer
    QSharedPointer< QCPAxisTicker > axisTicker = xAxis->ticker();

    // cast to QCPAxisTickerFixed instance
    QCPAxisTickerFixed* ticker = dynamic_cast< QCPAxisTickerFixed* >( axisTicker.data() );

    ticker->setTickCount( tickcount );
    ticker->setTickStep( mTickStep );
#else
    xAxis->setSubTickCount( qMax( 1.0, (qreal) qCeil( tickStepMantissa ) ) - 1 );

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

    xAxis->setTickVector( mTickVector );
    xAxis->setTickVectorLabels( mTickLabelVector );
#endif

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
 * This method handles adding a trace event to the axis rect for the trace graph.
 */
void PerformanceDataGraphView::handleAddGraphItem(const QString &clusteringCriteriaName, const QString &metricNameTitle, const QString &metricName, double eventTime, double eventData, int rankOrThread)
{
    Q_UNUSED( rankOrThread );

    CustomPlot* graphView( Q_NULLPTR);

    {
        QMutexLocker guard( &m_mutex );

        if ( ! m_metricGroup.contains( metricName ) )
        {
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
    }

    if ( graphView ) {
        // pass data points to graphs
        graphView->graph( 0 )->addData( eventTime, eventData );
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
    Q_UNUSED( lower );
    Q_UNUSED( upper );

    if ( clusteringCriteriaName.isEmpty() || modeName.isEmpty() || viewName.isEmpty() )
        return;

    if ( ! ( QStringLiteral("Trace") == modeName && QStringLiteral("All Events") == viewName ) )
        return;

    const QCPRange range( lower, upper );

    CustomPlot* graphView( Q_NULLPTR );

    {
        QMutexLocker guard( &m_mutex );

        if ( ! m_metricGroup.contains( metricName ) )
            return;

        MetricGroup& metricGroup = m_metricGroup[ metricName ];

        graphView = metricGroup.graph;

        metricGroup.xGraphRange = range;
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

            const int NUM_SIGNIFICANT_DIGITS = qCeil( std::log10( metricGroup.yGraphRange.upper ) );
            const double MAX_YAXIS_VALUE = qCeil( metricGroup.yGraphRange.upper / qPow( 10.0, NUM_SIGNIFICANT_DIGITS - 1 ) ) * qPow( 10.0, NUM_SIGNIFICANT_DIGITS - 1 );

            yAxis->setRange( QCPRange( 0.0, MAX_YAXIS_VALUE ) );
            yAxis->setTicks( true );

            QVector<double> tickValues;
            QVector<QString> tickLabels;

            for (int i=0; i<=10; ++i) {
                const double value( i * qRound( MAX_YAXIS_VALUE / 10.0 ) );
                tickValues << value;
                tickLabels << QString::number( value, 'f', 0 );
            }

            yAxis->setTickVector( tickValues );
            yAxis->setTickVectorLabels( tickLabels );
        }
        // set the x-axis graph range
        QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
        if ( xAxis ) {
            xAxis->setRange( range );
        }
        // force graph replot
#if defined(HAS_QCUSTOMPLOT_V2)
        graphView->replot( QCustomPlot::rpQueuedReplot );
#else
        graphView->replot();
#endif
    }
}


} // GUI
} // ArgoNavis

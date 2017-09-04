#include "OSSHighlightItem.h"

#include "OSSTraceItem.h"

#include <QTimer>


namespace ArgoNavis { namespace GUI {


const double HIGHLIGHT_AREA_SIZE = 10.0;


/**
 * @brief OSSHighlightItem::OSSHighlightItem
 * @param axisRect - the associated axis rect
 * @param parentPlot - the parent QCustomPlot instance
 *
 * Constructs an OSSHighlightItem instance which is clipped to the specified axis rect.
 * Each position of this QCPItemRect is associated to the specifed axis rect and
 * X and Y axes of the axis rect.  Each position typeX property is set to use plot
 * coordinates and typeY property is set to use axis rect ratio.
 */
OSSHighlightItem::OSSHighlightItem(QCPAxisRect* axisRect, QCustomPlot *parentPlot)
    : QCPItemRect( parentPlot )
    , m_axisRect( axisRect )
{
    // event belongs to axis rect
    setClipAxisRect( axisRect );

    // highlight item is only visible when setData slot called and automatically hides after 5 seconds
    setVisible( false );
}

/**
 * @brief OSSHighlightItem::~OSSHighlightItem
 *
 * Destroys the OSSHighlightItem instance.
 */
OSSHighlightItem::~OSSHighlightItem()
{

}

/**
 * @brief OSSHighlightItem::setData
 * @param annotation
 * @param timeBegin
 * @param timeEnd
 * @param rank
 */
void OSSHighlightItem::setData(const QString &annotation, double timeBegin, double timeEnd, int rank)
{
    m_annotation = annotation;

    // set selected
    //setSelected( true );

    // set brushes and pens for normal (non-selected) appearance
    setBrush( QColor("#fee270") );

    // set brushes and pens for selected appearance (only highlight border)
    setSelectedBrush( brush() );  // same brush as normal appearance

    // set position types to plot coordinates for X axis and viewport ratio for Y axis
    const QCPItemPosition::PositionType yPosType( -1 == rank ? QCPItemPosition::ptAxisRectRatio : QCPItemPosition::ptPlotCoords );
    foreach( QCPItemPosition* position, positions() ) {
        position->setAxisRect( m_axisRect );
        position->setAxes( m_axisRect->axis( QCPAxis::atBottom ), m_axisRect->axis( QCPAxis::atLeft ) );
        position->setTypeX( QCPItemPosition::ptPlotCoords );
        position->setTypeY( yPosType );
    }

    if ( -1 == rank ) {
        topLeft->setCoords( timeBegin, 0.40 );
        bottomRight->setCoords( timeEnd, 0.60 );
    }
    else {
        topLeft->setCoords( timeBegin, (double) rank + OSSTraceItem::s_halfHeight + 0.1 );
        bottomRight->setCoords( timeEnd, (double) rank - OSSTraceItem::s_halfHeight - 0.1 );
    }

    // make visible
    setVisible( true );

    // start timer to hide highlight in 10 seconds
    QTimer::singleShot( 10000, this, SLOT(handleTimeout()) );

    parentPlot()->replot();
}

/**
 * @brief OSSHighlightItem::draw
 * @param painter - the painter used for drawing
 *
 * Reimplements the OSSEventItem::draw method.
 */
void OSSHighlightItem::draw(QCPPainter *painter)
{
#if defined(HAS_QCUSTOMPLOT_V2)
    QPointF p1 = topLeft->pixelPosition();
    QPointF p2 = bottomRight->pixelPosition();
#else
    QPointF p1 = topLeft->pixelPoint();
    QPointF p2 = bottomRight->pixelPoint();
#endif

    if ( p1.toPoint() == p2.toPoint() )
        return;

    QRectF rect = QRectF( p1, p2 ).normalized();

    double clipPad = mainPen().widthF();
    QRectF boundingRect = rect.adjusted( -clipPad, -clipPad, clipPad, clipPad );

    QPainterPath innerPath;
    innerPath.addRoundedRect( boundingRect, 5.0, 5.0 );

    QRectF outerBoundingRect = rect.adjusted( -HIGHLIGHT_AREA_SIZE, -HIGHLIGHT_AREA_SIZE, HIGHLIGHT_AREA_SIZE, HIGHLIGHT_AREA_SIZE );

    QPainterPath outerPath;
    outerPath.addRoundedRect( outerBoundingRect, 5.0, 5.0 );

    QPainterPath path = outerPath.intersected( innerPath );

    if ( boundingRect.intersects( clipRect() ) ) { // only draw if bounding rect of rect item is visible in cliprect
        painter->setPen( mainPen() );
        painter->setBrush( mainBrush() );
        painter->drawPath( path );
    }
}

/**
 * @brief OSSHighlightItem::handleTimeout
 */
void OSSHighlightItem::handleTimeout()
{
    setVisible( false );

    parentPlot()->replot();
}

} // GUI
} // ArgoNavis

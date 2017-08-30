#include "OSSHighlightItem.h"


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
    : OSSEventItem( axisRect, parentPlot )
{

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
 * @brief OSSHighlightItem::draw
 * @param painter - the painter used for drawing
 *
 * Reimplements the OSSEventItem::draw method.
 */
void OSSHighlightItem::draw(QCPPainter *painter)
{
    QPointF p1 = topLeft->pixelPoint();
    QPointF p2 = bottomRight->pixelPoint();

    if ( p1.toPoint() == p2.toPoint() )
        return;

    QRectF rect = QRectF( p1, p2 ).normalized();

    double clipPad = mainPen().widthF();
    QRectF boundingRect = rect.adjusted( -clipPad, -clipPad, clipPad, clipPad );

    QPainterPath innerPath;
    innerPath.addRoundedRect( boundingRect, 5.0, 5.0 );

    QRectF outerBoundingRect = rect.adjusted( HIGHLIGHT_AREA_SIZE, HIGHLIGHT_AREA_SIZE, HIGHLIGHT_AREA_SIZE, HIGHLIGHT_AREA_SIZE );

    QPainterPath outerPath;
    outerPath.addRoundedRect( outerBoundingRect, 5.0, 5.0 );

    QPainterPath path = outerPath.intersected( innerPath );

    if ( boundingRect.intersects( clipRect() ) ) { // only draw if bounding rect of rect item is visible in cliprect
        painter->setPen( mainPen() );
        painter->setBrush( mainBrush() );
        painter->drawPath( path );
    }
}

} // GUI
} // ArgoNavis

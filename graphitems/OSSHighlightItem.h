#ifndef OSSHIGHLIGHTITEM_H
#define OSSHIGHLIGHTITEM_H


#include "OSSEventItem.h"

#include "qcustomplot.h"

#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


class OSSHighlightItem : public OSSEventItem
{
    Q_OBJECT

public:

    explicit OSSHighlightItem(QCPAxisRect* axisRect, QCustomPlot* parentPlot = 0);
    virtual ~OSSHighlightItem();

protected:

    virtual void draw(QCPPainter *painter) Q_DECL_OVERRIDE;

};


} // GUI
} // ArgoNavis

#endif // OSSHIGHLIGHTITEM_H

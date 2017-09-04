#ifndef OSSHIGHLIGHTITEM_H
#define OSSHIGHLIGHTITEM_H


#include "qcustomplot.h"

#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


class OSSHighlightItem : public QCPItemRect
{
    Q_OBJECT

public:

    explicit OSSHighlightItem(QCPAxisRect* axisRect, QCustomPlot* parentPlot = 0);
    virtual ~OSSHighlightItem();

public slots:

    void setData(const QString& annotation, double timeBegin, double timeEnd, int rank = -1);

protected:

    virtual void draw(QCPPainter *painter) Q_DECL_OVERRIDE;

private slots:

    void handleTimeout();

private:

    QCPAxisRect* m_axisRect;

    QString m_annotation;

};


} // GUI
} // ArgoNavis

#endif // OSSHIGHLIGHTITEM_H

/*!
   \file OSSHighlightItem.h
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

#ifndef OSSHIGHLIGHTITEM_H
#define OSSHIGHLIGHTITEM_H


#include "qcustomplot.h"

#include "common/openss-gui-config.h"

#include <QAtomicInt>


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

    // a reference counter for the number of one-shot timers started
    QAtomicInt m_counter;

};


} // GUI
} // ArgoNavis

#endif // OSSHIGHLIGHTITEM_H

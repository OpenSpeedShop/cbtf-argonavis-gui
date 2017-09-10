/*!
   \file OSSMpiFunctionItem.h
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

#ifndef OSSTRACEITEM_H
#define OSSTRACEITEM_H

#include "OSSEventItem.h"
#include "qcustomplot.h"


namespace ArgoNavis { namespace GUI {


class OSSTraceItem : public OSSEventItem
{
public:

    explicit OSSTraceItem(QCPAxisRect* axisRect, QCustomPlot* parentPlot = 0);
    virtual ~OSSTraceItem();

    void setData(const QString& functionName, double timeBegin, double timeEnd, int rank);
    void setBrush(const QString& functionName);

    static double s_halfHeight;

protected:

    virtual void draw(QCPPainter *painter) Q_DECL_OVERRIDE;

private:

    QString m_functionName;

};


} // GUI
} // ArgoNavis

#endif // OSSTRACEITEM_H

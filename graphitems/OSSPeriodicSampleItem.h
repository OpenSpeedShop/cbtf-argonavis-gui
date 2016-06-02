/*!
   \file OSSPeriodicSample.h
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2016 Argo Navis Technologies, LLC

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

#ifndef OSSPERIODICSAMPLEITEM_H
#define OSSPERIODICSAMPLEITEM_H

#include "qcustomplot.h"


namespace ArgoNavis { namespace GUI {


class OSSPeriodicSampleItem : public QCPItemRect
{
    Q_OBJECT

public:

    explicit OSSPeriodicSampleItem(QCPAxisRect* axisRect, QCustomPlot* parentPlot = 0);
    virtual ~OSSPeriodicSampleItem();

    void setData(const double& time_begin,
                 const double& time_end,
                 const double& count);

private:

    // raw period sample data
    double m_time_begin;
    double m_time_end;
    uint64_t m_count;

};


} // GUI
} // ArgoNavis

#endif // OSSPERIODICSAMPLEITEM_H

/*!
   \file OSSKernelExecutionItem.h
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

#ifndef OSSKERNELEXECUTIONITEM_H
#define OSSKERNELEXECUTIONITEM_H

#include "OSSEventItem.h"
#include "qcustomplot.h"

#include <ArgoNavis/Base/Time.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>


namespace ArgoNavis { namespace GUI {


class OSSKernelExecutionItem : public OSSEventItem
{
    Q_OBJECT

public:

    explicit OSSKernelExecutionItem(QCPAxisRect* axisRect, QCustomPlot* parentPlot = 0);
    virtual ~OSSKernelExecutionItem();

    void setData(const Base::Time& time_origin,
                 const CUDA::KernelExecution& details);

protected:

    CUDA::KernelExecution m_details;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
private:

    friend QTextStream& operator<<(QTextStream& os, const OSSKernelExecutionItem& item);
#endif
};


#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
QTextStream& operator<<(QTextStream& os, const OSSKernelExecutionItem& item);
#endif


} // GUI
} // ArgoNavis

#endif // OSSKERNELEXECUTIONITEM_H

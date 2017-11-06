/*!
   \file CustomPlot.h
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

#ifndef CUSTOMPLOT_H
#define CUSTOMPLOT_H

#include "qcustomplot.h"

#include "common/openss-gui-config.h"


class CustomPlot : public QCustomPlot
{
    Q_OBJECT

public:

    explicit CustomPlot(QWidget *parent = 0);

signals:

    void signalXAxisTickLabelSelected(const QString& label);

protected:

    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private slots:

    void handleMousePress(QMouseEvent *event);

};


#endif // CUSTOMPLOT_H

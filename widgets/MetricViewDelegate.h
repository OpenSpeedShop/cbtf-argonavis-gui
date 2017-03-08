/*!
   \file MetricViewDelegate.h
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

#ifndef METRICVIEWDELEGATE_H
#define METRICVIEWDELEGATE_H

#include <QStyledItemDelegate>

#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


class MetricViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:

    explicit MetricViewDelegate(QObject *parent = 0);

protected:

    QString displayText(const QVariant &value, const QLocale &locale) const Q_DECL_OVERRIDE;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;

};


} // GUI
} // ArgoNavis

#endif // METRICVIEWDELEGATE_H

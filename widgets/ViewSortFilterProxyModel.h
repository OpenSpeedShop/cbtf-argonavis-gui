/*!
   \file ViewSortFilterProxyModel.h
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

#ifndef VIEWSORTFILTERPROXYMODEL_H
#define VIEWSORTFILTERPROXYMODEL_H

#include "DefaultSortFilterProxyModel.h"

#include "common/openss-gui-config.h"

#include <QSet>
#include <QString>


namespace ArgoNavis { namespace GUI {


class ViewSortFilterProxyModel : public DefaultSortFilterProxyModel
{
    Q_OBJECT

public:

    explicit ViewSortFilterProxyModel(const QString& type = "*", QObject* parent = Q_NULLPTR);
    virtual ~ViewSortFilterProxyModel();

    void setColumnHeaders(const QStringList &columnHeaders);

    void setFilterRange(double lower, double upper);

protected:

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const Q_DECL_OVERRIDE;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const Q_DECL_OVERRIDE;

private:

    double m_lower;
    double m_upper;

    QSet< int > m_columns;

};


} // GUI
} // ArgoNavis

#endif // VIEWSORTFILTERPROXYMODEL_H

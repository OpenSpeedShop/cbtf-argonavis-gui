/*!
   \file TreeItem.h
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

#ifndef TREEITEM_H
#define TREEITEM_H

#include <QObject>
#include <QList>
#include <QVariant>

#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


/*!
 * \brief The TreeItem class
 */

class TreeItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable NOTIFY checkableChanged)
    Q_PROPERTY(bool checked READ isChecked() WRITE setChecked NOTIFY checkedChanged)
    Q_PROPERTY(bool enabled READ isEnabled() WRITE setEnabled NOTIFY enabledChanged)

    friend class TreeModel;

public:

    explicit TreeItem(const QList<QVariant> &data, TreeItem *parent = 0, bool checkable = false, bool checked = false, bool enabled = true);
    virtual ~TreeItem();

    // child manipulators
    void appendChild(TreeItem *child);
    void removeChild(TreeItem *child);

signals:

    void checkableChanged(bool value);
    void checkedChanged(bool value);
    void enabledChanged(bool value);

protected:

    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    TreeItem *parentItem();
    // check state setter/getter
    void setChecked(bool set){ if ( m_enabled) { m_checked = set; emit checkedChanged( set ); } }
    bool isChecked() const { return m_checked; }
    // checkable state setter/getter
    void setCheckable(bool set) { m_checkable = set; emit checkableChanged( set ); }
    bool isCheckable() const { return m_checkable; }
    // enabled state setter/getter
    void setEnabled(bool set){ m_enabled = set; emit enabledChanged( set ); }
    bool isEnabled() const { return m_enabled; }

    void setData(int column, const QVariant& data);

private:

    explicit TreeItem() Q_DECL_EQ_DELETE;

    QList<TreeItem*> m_childItems;
    QList<QVariant> m_itemData;
    bool m_checked;
    bool m_checkable;
    bool m_enabled;

};


} // GUI
} // ArgoNavis

#endif // TREEITEM_H

/*!
   \file ApplicationOverrideCursorManager.h
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

#ifndef APPLICATIONOVERRIDECURSORMANAGER_H
#define APPLICATIONOVERRIDECURSORMANAGER_H

#include <QObject>
#include <QMutex>
#include <QAtomicPointer>
#include <QStringList>


namespace ArgoNavis { namespace GUI {


class ApplicationOverrideCursorManager : public QObject
{
    Q_OBJECT

public:

    static ApplicationOverrideCursorManager *instance();

    void startWaitingOperation(const QString& name);
    void finishWaitingOperation(const QString& name);

    void destroy();

private:

    explicit ApplicationOverrideCursorManager(QObject *parent = 0);

private:

    static QAtomicPointer< ApplicationOverrideCursorManager > s_instance;

    QStringList m_activeWaitingOperations;

    QMutex m_mutex;

};


} // GUI
} // ArgoNavis

#endif // APPLICATIONOVERRIDECURSORMANAGER_H

/*!
   \file ApplicationOverrideCursorManager.cpp
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

#include "ApplicationOverrideCursorManager.h"

#include "common/openss-gui-config.h"

#include <QApplication>
#include <QDebug>


namespace ArgoNavis { namespace GUI {


QAtomicPointer< ApplicationOverrideCursorManager > ApplicationOverrideCursorManager::s_instance;


/**
 * @brief ApplicationOverrideCursorManager::ApplicationOverrideCursorManager
 * @param parent
 */
ApplicationOverrideCursorManager::ApplicationOverrideCursorManager(QObject *parent)
    : QObject( parent )
{

}

/**
 * @brief ApplicationOverrideCursorManager::instance
 * @return - return a pointer to the singleton instance
 *
 * This method provides a pointer to the singleton instance.
 */
ApplicationOverrideCursorManager *ApplicationOverrideCursorManager::instance()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    ApplicationOverrideCursorManager* inst = s_instance.loadAcquire();
#else
    ApplicationOverrideCursorManager* inst = s_instance;
#endif

    if ( ! inst ) {
        inst = new ApplicationOverrideCursorManager();
        if ( ! s_instance.testAndSetRelease( 0, inst ) ) {
            delete inst;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            inst = s_instance.loadAcquire();
#else
            inst = s_instance;
#endif
        }
    }

    return inst;
}

/**
 * @brief ApplicationOverrideCursorManager::startWaitingOperation
 * @param name
 */
void ApplicationOverrideCursorManager::startWaitingOperation(const QString &name)
{
    QMutexLocker guard( &m_mutex );

    if ( 0 == m_activeWaitingOperations.size() ) {
        QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
    }

    m_activeWaitingOperations << name;

    qDebug() << "ApplicationOverrideCursorManager::startWaitingOperation: name=" << name << " # active=" << m_activeWaitingOperations.size();
}

/**
 * @brief ApplicationOverrideCursorManager::finishWaitingOperation
 * @param name
 */
void ApplicationOverrideCursorManager::finishWaitingOperation(const QString &name)
{
    QMutexLocker guard( &m_mutex );

    m_activeWaitingOperations.removeOne( name );

    if ( 0 == m_activeWaitingOperations.size() ) {
        QApplication::restoreOverrideCursor();
    }

    qDebug() << "ApplicationOverrideCursorManager::finishWaitingOperation: name=" << name << " # active=" << m_activeWaitingOperations.size();
}

/**
 * @brief ApplicationOverrideCursorManager::destroy
 *
 * Static method to destroy the singleton instance.
 */
void ApplicationOverrideCursorManager::destroy()
{
    if ( s_instance )
        delete s_instance;

    s_instance = Q_NULLPTR;
}


} // GUI
} // ArgoNavis

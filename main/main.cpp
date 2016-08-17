/*!
   \file main.cpp
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

#include "MainWindow.h"

#if defined(HAS_DESTROY_SINGLETONS)
#include "managers/PerformanceDataManager.h"
#endif

#include <QApplication>

using namespace ArgoNavis;


int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QApplication::setGraphicsSystem("raster");
#endif

    QApplication a( argc, argv );

    GUI::MainWindow w;
    w.show();

    int status = a.exec();

#if defined(HAS_DESTROY_SINGLETONS)
    GUI::PerformanceDataManager::destroy();
#endif

    return status;
}

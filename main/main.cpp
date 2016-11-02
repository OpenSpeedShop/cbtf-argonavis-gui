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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
#include <QCommandLineParser>
#else
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#endif
#include <QDebug>

using namespace ArgoNavis;


int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QApplication::setGraphicsSystem("raster");
#endif

    QApplication app( argc, argv );

    QCoreApplication::setApplicationName( argv[0] );
    QString versionStr;
    if (APP_BUILD_VERSION == 0)
        versionStr = QString("%1.%2.%3").arg(APP_MAJOR_VERSION).arg(APP_MINOR_VERSION).arg(APP_SUBMINOR_VERSION);
    else
        versionStr = QString("%1.%2.%3 (%4").arg(APP_MAJOR_VERSION).arg(APP_MINOR_VERSION).arg(APP_MINOR_VERSION).arg(APP_BUILD_VERSION);
    QCoreApplication::setApplicationVersion( versionStr );

    const QString descriptionStr = QCoreApplication::translate( "main", "Open|SpeedShop Application Performance Analysis GUI" );
    const QString fileDescriptionStr = QCoreApplication::translate( "main", "The Open|SpeendShop experiment database (.openss) file to load." );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    QCommandLineParser parser;
    parser.setApplicationDescription( descriptionStr );
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption fileOption( QStringList() << "f" << "file", fileDescriptionStr, "file" );
    parser.addOption( fileOption );

    // Process the actual command line arguments given by the user
    parser.process( app );
#else
    const QString versionOptionOutputStr = QCoreApplication::translate( "main", QString("%1: %2").arg(argv[0]).arg(versionStr).toUtf8().data() );
    const QString helpDescriptionStr = QCoreApplication::translate( "main", "Displays this help." );
    const QString versionDescriptionStr = QCoreApplication::translate( "main", "Displays version information." );
    const QString usageOutputStr = QCoreApplication::translate( "main",
                                                                QString("Usage: %1 [options]\n%2\n\n"
                                                                        "Options:\n"
                                                                        " -h, --help     %3\n"
                                                                        " -v, --version  %4\n"
                                                                        " -f <file>      %5\n"
                                                                        ).arg(argv[0]).arg(descriptionStr).arg(helpDescriptionStr).arg(versionDescriptionStr).arg(fileDescriptionStr).toUtf8().data() );

    boost::program_options::options_description kNonPositionalOptions( descriptionStr.toUtf8().data() );
    kNonPositionalOptions.add_options()
        ( "file,f", boost::program_options::value<std::string>(), fileDescriptionStr.toUtf8().data() )
        ( "version,v", versionDescriptionStr.toUtf8().data() )
        ( "help,h", usageOutputStr.toUtf8().data() );

    boost::program_options::variables_map values;

    try {
        boost::program_options::store( boost::program_options::command_line_parser( argc, argv ).options( kNonPositionalOptions ).run(), values );
        boost::program_options::notify( values );
    }
    catch (const std::exception& error) {
        std::cout << usageOutputStr.toUtf8().data();
        return 0;
    }

    if ( values.count("version") > 0 ) {
        std::cout << versionOptionOutputStr.toUtf8().data() << std::endl;
        return 0;
    }

    if ( values.count("help") > 0 ) {
        std::cout << usageOutputStr.toUtf8().data();
        return 0;
    }
#endif

    GUI::MainWindow w;
    QString filenameStr;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    if ( parser.isSet( fileOption ) ) {
        filenameStr = parser.value( fileOption );
    }
#else
    if ( values.count("file") != 0 ) {
        const std::string filename = values["file"].as<std::string>();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
        filenameStr = QString::fromStdString( filename );
#else
        filenameStr = QString( filename.c_str() );
#endif
    }
#endif
    if ( ! filenameStr.isEmpty() ) {
        w.setExperimentDatabase( filenameStr );
    }
    w.show();

    int status = app.exec();

#if defined(HAS_DESTROY_SINGLETONS)
    GUI::PerformanceDataManager::destroy();
#endif

    return status;
}

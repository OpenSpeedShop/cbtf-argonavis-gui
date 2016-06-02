# This file is part of the Open|SpeedShop Graphical User Interface
# Copyright (C) 2010-2013 Argo Navis Technologies, LLC
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

!isEmpty(OPENSS_PRI_INCLUDED):error("OpenSS-CLI.pri already included")
OPENSS_PRI_INCLUDED = 1

###########################
# LIBRARY & INCLUDE PATHS #
###########################

!win32 {
  isEmpty(LIBOPENSS_INC) {
    warning()
    warning("The LIBOPENSS_INC variable must be set to the location of the Open|SpeedShop framework includes in order to " \
            "interface directly with Open|SpeedShop command-line interface (CLI) functionality.")
    warning()

  } else {
    !exists($$quote($${LIBOPENSS_INC}/include/libopenss-cli/Direct.hxx)) {
      warning()
      warning("The plugin depends on headers from the Open|SpeedShop tool framework.  However, the path passed " \
              "to qmake through 'LIBOPENSS_INC=$$LIBOPENSS_INC', doesn't contain the expected top-level header files.")
      error("Header files at $$quote($${LIBOPENSS_INC}/include/) were not found.")
    }

    INCLUDEPATH += $$quote($${LIBOPENSS_INC}/include)
    DEPENDPATH  += $$quote($${LIBOPENSS_INC}/include)

    isEmpty(OPENSS_LIBPATH): OPENSS_LIBPATH = $${OPENSS_PATH}

    OPENSS_LIBFILES += openss-cli
    OPENSS_LIBFILES += openss-framework
    OPENSS_LIBFILES += openss-framework-cbtf
    OPENSS_LIBFILES += openss-framework-symtabapi
    OPENSS_LIBFILES += openss-message
    OPENSS_LIBFILES += openss-queries
    OPENSS_LIBFILES += openss-queries-cuda

    for(OPENSS_LIBFILE, OPENSS_LIBFILES) {
      INCLUDEPATH += $$quote($${LIBOPENSS_INC}/include/lib$${OPENSS_LIBFILE})
      DEPENDPATH  += $$quote($${LIBOPENSS_INC}/include/lib$${OPENSS_LIBFILE})

      OPENSS_DIRECTORIES = $${OPENSS_LIBPATH}
      OPENSS_DIRECTORIES += $${OPENSS_LIBPATH}/lib
      OPENSS_DIRECTORIES += $${OPENSS_LIBPATH}/lib64

      OPENSS_FILENAME = lib$${OPENSS_LIBFILE}.so

      for(OPENSS_DIRECTORY, OPENSS_DIRECTORIES) {
        exists($${OPENSS_DIRECTORY}/$${OPENSS_FILENAME}) {
          LIBS += -L$$quote($${OPENSS_DIRECTORY}) -l$${OPENSS_LIBFILE}
          unset(OPENSS_DIRECTORIES)
          break()
        }
      }

      !isEmpty(OPENSS_DIRECTORIES) {
        for(OPENSS_DIRECTORY, OPENSS_DIRECTORIES) {
          warning("$${OPENSS_DIRECTORY}/$${OPENSS_FILENAME} was not found")
        }
        error("Please ensure that you have already built the Open|SpeedShop tool framework.")
      }
    }
  }
}

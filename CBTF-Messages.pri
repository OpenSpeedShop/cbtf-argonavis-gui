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

!isEmpty(CBTF_MESSAGES_PRI_INCLUDED):error("CBTF-Messages.pri already included")
CBTF_MESSAGES_PRI_INCLUDED = 1

###########################
# LIBRARY ONLY PATHS      #
###########################

!win32 {
  isEmpty(CBTF_MESSAGES_PATH) {
    warning()
    warning("The CBTF_MESSAGES_PATH variable must be set to the location of the CBTF Messages framework in order to " \
            "build the Open|SpeedShop GUI.")
    warning()

  } else {

    INCLUDEPATH += $$quote($${CBTF_MESSAGES_PATH}/include)
    DEPENDPATH  += $$quote($${CBTF_MESSAGES_PATH}/include)

    isEmpty(CBTF_MESSAGES_LIBPATH): CBTF_MESSAGES_LIBPATH = $${CBTF_MESSAGES_PATH}

    CBTF_MESSAGES_LIBFILES = cbtf cbtf-core cbtf-mrnet cbtf-core-mrnet cbtf-messages-base cbtf-messages-collector cbtf-messages-events cbtf-messages-instrumentation cbtf-messages-perfdata  cbtf-messages-symtab  cbtf-messages-thread cbtf-messages-cuda cbtf-xml

    for(CBTF_MESSAGES_LIBFILE, CBTF_MESSAGES_LIBFILES) {
      CBTF_MESSAGES_FILENAME = lib$${CBTF_MESSAGES_LIBFILE}.so

      CBTF_MESSAGES_DIRECTORIES = $${CBTF_MESSAGES_LIBPATH}
      CBTF_MESSAGES_DIRECTORIES += $${CBTF_MESSAGES_LIBPATH}/lib
      CBTF_MESSAGES_DIRECTORIES += $${CBTF_MESSAGES_LIBPATH}/lib64

      for(CBTF_MESSAGES_DIRECTORY, CBTF_MESSAGES_DIRECTORIES) {
        exists($${CBTF_MESSAGES_DIRECTORY}/$${CBTF_MESSAGES_FILENAME}) {
           LIBS += -L$$quote($${CBTF_MESSAGES_DIRECTORY}) -l$${CBTF_MESSAGES_LIBFILE}
           unset(CBTF_MESSAGES_DIRECTORIES)
           break()
        }
      }

      !isEmpty(CBTF_MESSAGES_DIRECTORIES) {
        for(CBTF_MESSAGES_DIRECTORY, CBTF_MESSAGES_DIRECTORIES) {
          warning("$${CBTF_MESSAGES_DIRECTORY}/$${CBTF_MESSAGES_FILENAME} was not found")
         }
         error("Please ensure that you have already built the CBTF Messages Framwework.")
      }
    }
  }
}

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

    #CBTF_INCLUDEPATH += $$quote($${CBTF_MESSAGES_PATH}/include)
    #CBTF_DEPENDPATH  += $$quote($${CBTF_MESSAGES_PATH}/include)
    #CBTFK_INCLUDEPATH += $$quote($${CBTF_KRELL_MESSAGES_PATH}/include)
    #CBTFK_DEPENDPATH  += $$quote($${CBTF_KRELL_MESSAGES_PATH}/include)
    #CBTFARGO_INCLUDEPATH += $$quote($${CBTF_ARGONAVIS_MESSAGES_PATH}/include)
    #CBTFARGO_DEPENDPATH  += $$quote($${CBTF_ARGONAVIS_MESSAGES_PATH}/include)

    isEmpty(CBTF_MESSAGES_LIBPATH): CBTF_MESSAGES_LIBPATH = $${CBTF_MESSAGES_PATH}
    isEmpty(CBTF_KRELL_MESSAGES_LIBPATH): CBTF_KRELL_MESSAGES_LIBPATH = $${CBTF_KRELL_MESSAGES_PATH}
    isEmpty(CBTF_ARGONAVIS_MESSAGES_LIBPATH): CBTF_ARGONAVIS_MESSAGES_LIBPATH = $${CBTF_ARGONAVIS_MESSAGES_PATH}

    CBTF_MESSAGES_LIBFILES = cbtf
    CBTF_MESSAGES_LIBFILES += cbtf-mrnet
    CBTF_MESSAGES_LIBFILES += cbtf-xml

    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-core
    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-core-mrnet
    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-messages-base
    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-messages-collector
    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-messages-events
    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-messages-instrumentation
    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-messages-perfdata
    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-messages-symtab
    CBTF_KRELL_MESSAGES_LIBFILES += cbtf-messages-thread
    CBTF_ARGONAVIS_MESSAGES_LIBFILES += cbtf-messages-cuda

    for(CBTF_MESSAGES_LIBFILE, CBTF_MESSAGES_LIBFILES) {
      CBTF_MESSAGES_FILENAME = lib$${CBTF_MESSAGES_LIBFILE}.so

      CBTF_MESSAGES_DIRECTORIES = $${CBTF_MESSAGES_LIBPATH}
      CBTF_MESSAGES_DIRECTORIES += $${CBTF_MESSAGES_LIBPATH}/lib
      CBTF_MESSAGES_DIRECTORIES += $${CBTF_MESSAGES_LIBPATH}/lib64
      CBTF_KRELL_MESSAGES_DIRECTORIES = $${CBTF_KRELL_MESSAGES_LIBPATH}
      CBTF_KRELL_MESSAGES_DIRECTORIES += $${CBTF_KRELL_MESSAGES_LIBPATH}/lib
      CBTF_KRELL_MESSAGES_DIRECTORIES += $${CBTF_KRELL_MESSAGES_LIBPATH}/lib64
      CBTF_ARGONAVIS_MESSAGES_DIRECTORIES = $${CBTF_ARGONAVIS_MESSAGES_LIBPATH}
      CBTF_ARGONAVIS_MESSAGES_DIRECTORIES += $${CBTF_ARGONAVIS_MESSAGES_LIBPATH}/lib
      CBTF_ARGONAVIS_MESSAGES_DIRECTORIES += $${CBTF_ARGONAVIS_MESSAGES_LIBPATH}/lib64

      for(CBTF_MESSAGES_DIRECTORY, CBTF_MESSAGES_DIRECTORIES) {
        exists($${CBTF_MESSAGES_DIRECTORY}/$${CBTF_MESSAGES_FILENAME}) {
           LIBS += -L$$quote($${CBTF_MESSAGES_DIRECTORY}) -l$${CBTF_MESSAGES_LIBFILE}
           LIBS += -Wl,-rpath $$quote($${CBTF_MESSAGES_DIRECTORY})
           unset(CBTF_MESSAGES_DIRECTORIES)
           break()
        }
      }
      for(CBTF_KRELL_MESSAGES_DIRECTORY, CBTF_MESSAGES_DIRECTORIES) {
        exists($${CBTF_KRELL_MESSAGES_DIRECTORY}/$${CBTF_MESSAGES_FILENAME}) {
           LIBS += -L$$quote($${CBTF_KRELL_MESSAGES_DIRECTORY}) -l$${CBTF_KRELL_MESSAGES_LIBFILE}
           LIBS += -Wl,-rpath $$quote($${CBTF_KRELL_MESSAGES_DIRECTORY})
           unset(CBTF_MESSAGES_DIRECTORIES)
           break()
        }
      }
      for(CBTF_ARGONAVIS_MESSAGES_DIRECTORY, CBTF_MESSAGES_DIRECTORIES) {
        exists($${CBTF_ARGONAVIS_MESSAGES_DIRECTORY}/$${CBTF_MESSAGES_FILENAME}) {
           LIBS += -L$$quote($${CBTF_ARGONAVIS_MESSAGES_DIRECTORY}) -l$${CBTF_ARGONAVIS_MESSAGES_LIBFILE}
           LIBS += -Wl,-rpath $$quote($${CBTF_ARGONAVIS_MESSAGES_DIRECTORY})
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

#   Copyright (C) 2015,2020 MariaDB Corporation AB
#  
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not see <http://www.gnu.org/licenses>
#  or write to the Free Software Foundation, Inc., 
#  51 Franklin St., Fifth Floor, Boston, MA 02110, USA 

# FindDM.cmake
# 
# Cmake script to look for driver manager includes and libraries on platforms others than Windows
# We expect that the driver manager is UnixODBC

IF(WITH_IODBC)
  SET(ODBC_CONFIG_EXEC iodbc-config)
  SET(ODBC_CONFIG_INCLUDES --cflags)
  SET(ODBC_CONFIG_LIBS --libs)
  SET(ODBC_LIBS iodbc)
  SET(ODBC_INSTLIBS iodbcinst)
ELSE() #UnixODBC
  SET(ODBC_CONFIG_EXEC odbc_config)
  SET(ODBC_CONFIG_INCLUDES --include-prefix)
  SET(ODBC_CONFIG_LIBS --lib-prefix)
  SET(ODBC_LIBS odbc)
  SET(ODBC_INSTLIBS odbcinst)
ENDIF()

IF(ODBC_LIB_DIR AND ODBC_INCLUDE_DIR)
  MESSAGE(STATUS "Using preset values for DM dirs") 
ELSE()
  FIND_PROGRAM(ODBC_CONFIG ${ODBC_CONFIG_EXEC}
               PATH
               /usr/bin
               ${DM_DIR}
               "${DM_DIR}/bin"
               )

  IF(ODBC_CONFIG)
    MESSAGE(STATUS "Found ${ODBC_CONFIG_EXEC}: ${ODBC_CONFIG}")
    EXECUTE_PROCESS(COMMAND ${ODBC_CONFIG} ${ODBC_CONFIG_INCLUDES} 
                    OUTPUT_VARIABLE result)
    STRING(REPLACE "\n" "" ODBC_INCLUDE_DIR ${result})
    EXECUTE_PROCESS(COMMAND ${ODBC_CONFIG} ${ODBC_CONFIG_LIBS} 
                    OUTPUT_VARIABLE result)
    STRING(REPLACE "\n" "" ODBC_LIB_DIR ${result})

    IF(WITH_IODBC)
      STRING(REPLACE "-I" "" ODBC_INCLUDE_DIR ${ODBC_INCLUDE_DIR})
      STRING(REPLACE "-L" "" ODBC_LIB_DIR ${ODBC_LIB_DIR})
      STRING(REGEX REPLACE " +-liodbc -liodbcinst" "" ODBC_LIB_DIR ${ODBC_LIB_DIR})
    ENDIF()
  ELSE()
    MESSAGE(STATUS "${ODBC_CONFIG_EXEC} is not found ")
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_SEARCH_MODULE(ODBC REQUIRED ${ODBC_LIBS})
    PKG_SEARCH_MODULE(ODBCINST REQUIRED ${ODBC_INSTLIBS})

    IF(ODBC_FOUND)
      SET(ODBC_INCLUDE_DIR "${ODBC_INCLUDE_DIRS}")
      MESSAGE(STATUS "Found ODBC Driver Manager includes: ${ODBC_INCLUDE_DIR}")
    ENDIF()
    # Try to find DM libraries, giving precedence to special variables
    SET(ODBC_LIB_DIR "${ODBC_LIBRARY_DIRS}")
    SET(ODBCINST_LIB_DIR "${ODBCINST_LIBRARY_DIRS}")
  ENDIF()
ENDIF()

IF(ODBC_FOUND AND ODBCINST_FOUND)
  MESSAGE(STATUS "Found ODBC Driver Manager libraries: ${ODBC_LIB_DIR} ${ODBCINST_LIB_DIR}")
  SET(DM_FOUND TRUE)
ENDIF()

#   Copyright (C) 2015 MariaDB Corporation AB
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

FIND_PROGRAM(ODBC_CONFIG odbc_config
             PATH
             /usr/bin
             ${DM_DIR}
             )

IF(ODBC_CONFIG)
  MESSAGE(STATUS "Found odbc_config: ${ODBC_CONFIG}")
  EXECUTE_PROCESS(COMMAND ${ODBC_CONFIG} --include-prefix 
                  OUTPUT_VARIABLE result)
  STRING(REPLACE "\n" "" ODBC_INCLUDE_DIR ${result})
  EXECUTE_PROCESS(COMMAND ${ODBC_CONFIG} --lib-prefix 
                  OUTPUT_VARIABLE result)
  STRING(REPLACE "\n" "" ODBC_LIB_DIR ${result})
ELSE()
  # Try to find the include directory, giving precedence to special variables
  SET(LIB_PATHS /usr/local /usr)

  IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    SET(LIB_PATHS "${LIB_PATHS}" "/usr/lib/x86_64-linux-gnu")

    IF(EXISTS "/usr/lib64/")
      SET(LIB_SUFFIX "lib64" "x86_64-linux-gnu")
    ELSE()
      SET(LIB_SUFFIX "lib" "x86_64-linux-gnu")
    ENDIF()
 
  ELSE()
    SET(LIB_PATHS "${LIB_PATHS}" "/usr/local/lib/i386-linux-gnu" "/usr/lib/i386-linux-gnu" "/usr/local/lib/i686-linux-gnu" "/usr/lib/i686-linux-gnu")
    SET(LIB_SUFFIX "lib" "i386-linux-gnu" "i686-linux-gnu")
  ENDIF()

  FIND_PATH(ODBC_INCLUDE_DIR sql.h
      HINTS ${DM_INCLUDE_DIR}
            ${DM_DIR}
            ENV DM_INCLUDE_DIR
            ENV DM_DIR
      PATHS /usr/local
            /usr
      PATH_SUFFIXES include
      NO_DEFAULT_PATH
      DOC "Driver Manager Includes")
  # Giving chance to cmake_(environment)path
  FIND_PATH(ODBC_INCLUDE_DIR sql.h
      DOC "Driver Manager Includes")

  IF(ODBC_INCLUDE_DIR)
    MESSAGE(STATUS "Found ODBC Driver Manager includes: ${ODBC_INCLUDE_DIR}")
  ENDIF()

  # Try to find DM libraries, giving precedence to special variables
  FIND_PATH(ODBC_LIB_DIR libodbc.so
      HINTS ${DM_LIB_DIR}
            ${DM_DIR}
            ENV DM_LIB_DIR
            ENV DM_DIR
      PATHS ${LIB_PATHS}
      PATH_SUFFIXES ${LIB_SUFFIX} 
      NO_DEFAULT_PATH
      DOC "Driver Manager Libraries")
  FIND_PATH(ODBC_LIB_DIR libodbc.so
      DOC "Driver Manager Libraries")
ENDIF()

IF(ODBC_LIB_DIR AND ODBC_INCLUDE_DIR)
  MESSAGE(STATUS "Found ODBC Driver Manager libraries: ${ODBC_LIB_DIR}")
  SET(DM_FOUND TRUE)
ENDIF()

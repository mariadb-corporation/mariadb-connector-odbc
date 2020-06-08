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
    # Try to find the include directory, giving precedence to special variables
    SET(LIB_PATHS /usr/local /usr /usr/local/Cellar/libiodbc/3.52.12)

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
              /usr/local/Cellar/libiodbc/3.52.12
        PATH_SUFFIXES include include/iodbc
        NO_DEFAULT_PATH
        DOC "Driver Manager Includes")
    # Giving chance to cmake_(environment)path
    FIND_PATH(ODBC_INCLUDE_DIR sql.h
        DOC "Driver Manager Includes")

    IF(ODBC_INCLUDE_DIR)
      MESSAGE(STATUS "Found ODBC Driver Manager includes: ${ODBC_INCLUDE_DIR}")
    ENDIF()
    # Try to find DM libraries, giving precedence to special variables
    FIND_PATH(ODBC_LIB_DIR "lib${ODBC_LIBS}.so"
        HINTS ${DM_LIB_DIR}
              ${DM_DIR}
              ENV DM_LIB_DIR
              ENV DM_DIR
        PATHS ${LIB_PATHS}
        PATH_SUFFIXES ${LIB_SUFFIX} 
        NO_DEFAULT_PATH
        DOC "Driver Manager Libraries")
    FIND_PATH(ODBC_LIB_DIR "lib${ODBC_LIBS}.so"
        DOC "Driver Manager Libraries")
    FIND_PATH(ODBCINST_LIB_DIR "lib${ODBC_INSTLIBS}.so"
        HINTS ${DM_LIB_DIR}
              ${DM_DIR}
              ENV DM_LIB_DIR
              ENV DM_DIR
        PATHS ${LIB_PATHS}
        PATH_SUFFIXES ${LIB_SUFFIX} 
        NO_DEFAULT_PATH
        DOC "Driver Manager Libraries")
    FIND_PATH(ODBCINST_LIB_DIR "lib${ODBC_INSTLIBS}.so"
        DOC "Driver Manager Libraries")
  ENDIF()
ENDIF()

IF(ODBC_LIB_DIR AND ODBC_INCLUDE_DIR)
  MESSAGE(STATUS "Found ODBC Driver Manager libraries: ${ODBC_LIB_DIR} ${ODBCINST_LIB_DIR}")
  SET(DM_FOUND TRUE)
ENDIF()

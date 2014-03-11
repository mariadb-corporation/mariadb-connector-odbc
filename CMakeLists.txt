# ************************************************************************************
#   Copyright (C) 2013 SkySQL AB
#   
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Library General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#   
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Library General Public License for more details.
#   
#   You should have received a copy of the GNU Library General Public
#   License along with this library; if not see <http://www.gnu.org/licenses>
#   or write to the Free Software Foundation, Inc., 
#   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
# *************************************************************************************/
PROJECT(mariadb_odbc C)

cmake_minimum_required(VERSION 2.8)

SET (MARIADB_ODBC_SOURCES odbc_3_api.c
                          ma_error.c
                          ma_error.h
                          ma_connection.c
                          ma_connection.h
                          ma_helper.c
                          ma_helper.h
                          ma_debug.c
                          ma_debug.h
                          ma_dll.c
                          ma_dsn.c
                          ma_dsn.h
                          ma_driver.c
                          ma_driver.h
                          ma_info.c
                          ma_info.h
                          ma_environment.c
                          ma_environment.h
                          ma_parse.c
                          ma_parse.h
                          ma_statement.c
                          ma_statement.h
                          ma_desc.c
                          ma_desc.h
                          ma_string.c
                          ma_string.h
                          ma_odbc.h
                          ma_result.c
                          ma_result.h)

SET(DSN_DIALOG_FILES ${CMAKE_SOURCE_DIR}/dsn/odbc_dsn.c
                     ${CMAKE_SOURCE_DIR}/dsn/odbc_dsn.rc
                     ${CMAKE_SOURCE_DIR}/dsn/resource.h
                     ma_dsn.c
		     ma_driver.c
                     ma_string.c
                     ma_desc.c
                     ma_error.c
                     ma_helper.c
                     )

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DMAODBC_DEBUG")

IF(WIN32)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/win)
  SET(ODBC_LIBS odbc32)
  SET(ODBC_INSTLIBS odbccp32)
ELSE()

ENDIF()
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/FindMariaDB.cmake)
IF(MARIADB_FOUND)
  INCLUDE_DIRECTORIES(${MARIADB_INCLUDE_DIR})
  SET(MARIADB_LIB ${MARIADB_LIBRARY_DIR}/debug/mariadbclient.lib)
ELSE()
  MESSAGE(FATAL_ERROR "MariaDB not found")
ENDIF()

IF(WIN32)
  OPTION(WITH_SIGNCODE "Digitally sign excutables and libraries" OFF)
  OPTION(WITH_REVNO "Add revision number in msi packages" OFF)
ENDIF()


SET(MARIADB_ODBC_VERSION_MAJOR 1)
SET(MARIADB_ODBC_VERSION_MINOR 0)
SET(MARIADB_ODBC_VERSION_PATCH 1)

# We need to determine the last parameter for SQLColAttribute:
# Older UnixODBC version expect SQLPOINTER while Windows expects SQLLEN *
TRY_COMPILE(COMPILE_OK ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/cmake/sqlcolattribute.c)
MESSAGE(STATUS "Checking if SQLColAttribute expects SQLPOINTER ${COMPILE_OK}")
IF(COMPILE_OK)
  ADD_DEFINITIONS(-DSQLCOLATTRIB_SQLPOINTER)
ELSE()
  ADD_DEFINITIONS(-DSQLCOLATTRIB_SQLLEN_PTR)
ENDIF()

IF(WIN32)
  IF (MSVC)
    SET(CONFIG_TYPES "DEBUG" "RELEASE" "RELWITHDEBINFO" "MINSIZEREL")
    FOREACH(BUILD_TYPE ${CONFIG_TYPES})
      FOREACH(COMPILER CXX C)
        SET(COMPILER_FLAGS "${CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE}}")
        IF (NOT COMPILER_FLAGS STREQUAL "")
          STRING(REPLACE "/MD" "/MT" COMPILER_FLAGS ${COMPILER_FLAGS})
          IF (BUILD_TYPE STREQUAL "Debug")
            SET(COMPILER_FLAGS "${COMPILER_FLAGS} /RTC1 /RTCc")
            STRING(REPLACE "/Zi" "/ZI" COMPILER_FLAGS ${COMPILER_FLAGS})
          ENDIF()
          MESSAGE (STATUS "CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE}= ${COMPILER_FLAGS}") 
          SET(CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE} ${COMPILER_FLAGS} CACHE 
               STRING "overwritten by mariadb-odbc" FORCE)
        ENDIF()
      ENDFOREACH()
    ENDFOREACH()
  ENDIF()
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

SET(MARIADB_ODBC_VERSION "01.00.2000")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/ma_odbc_version.h.in
               ${CMAKE_SOURCE_DIR}/ma_odbc_version.h)

SET(LIBRARY_NAME "maodbca")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/mariadb-odbc-driver.def.in
               ${CMAKE_SOURCE_DIR}/mariadb-odbc-driver-asc.def)

ADD_LIBRARY(maodbca SHARED ${MARIADB_ODBC_SOURCES} ${CMAKE_SOURCE_DIR}/mariadb-odbc-driver-asc.def maodbca.rc)
SET_TARGET_PROPERTIES(maodbca PROPERTIES LANGUAGE C)
TARGET_LINK_LIBRARIES(maodbca ${MARIADB_LIB} ${ODBC_INSTLIBS} ws2_32)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/win)                                      
ADD_LIBRARY(maodbcs SHARED ${DSN_DIALOG_FILES} ${CMAKE_SOURCE_DIR}/dsn/mariadb_odbc_setup.def)
SET_TARGET_PROPERTIES(maodbcs PROPERTIES LANGUAGE C)
# TARGET_LINK_LIBRARIES(maodbcs comctl32 ${MARIADB_LIB} ${ODBC_LIBS} ${ODBC_INSTLIBS} maodbca)
TARGET_LINK_LIBRARIES(maodbcs comctl32 ${MARIADB_LIB} maodbca)


SET(UNICODE "W")
SET(LIBRARY_NAME "maodbcu")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/mariadb-odbc-driver.def.in
               ${CMAKE_SOURCE_DIR}/mariadb-odbc-driver-uni.def)
ADD_LIBRARY(maodbcu SHARED ${MARIADB_ODBC_SOURCES} ${CMAKE_SOURCE_DIR}/mariadb-odbc-driver-uni.def maodbcu.rc)
#SET_TARGET_PROPERTIES(maodbcu PROPERTIES DEFINE_SYMBOL UNICODE)
SET_TARGET_PROPERTIES(maodbcu PROPERTIES LANGUAGE C)
TARGET_LINK_LIBRARIES(maodbcu ${MARIADB_LIB} ${ODBC_INSTLIBS} ws2_32)

IF(WIN32)
  ADD_EXECUTABLE(dsn_test WIN32 dsn_test.c)
  TARGET_LINK_LIBRARIES(dsn_test ${MARIADB_LIB})
ENDIF()

IF(WIN32)
  ADD_SUBDIRECTORY(wininstall)
ENDIF()
ADD_SUBDIRECTORY(test)

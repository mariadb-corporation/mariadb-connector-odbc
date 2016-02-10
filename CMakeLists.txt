# ************************************************************************************
#   Copyright (C) 2013,2015 MariaDB Corporation AB
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
PROJECT(mariadb_connector_odbc C)

cmake_minimum_required(VERSION 2.8)

SET(MARIADB_ODBC_VERSION_MAJOR 1)
SET(MARIADB_ODBC_VERSION_MINOR 0)
SET(MARIADB_ODBC_VERSION_PATCH 6)

SET(MARIADB_ODBC_VERSION "01.00.0006")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/ma_odbc_version.h.in
               ${CMAKE_SOURCE_DIR}/ma_odbc_version.h)

SET (MARIADB_ODBC_SOURCES odbc_3_api.c
                          ma_error.c
                          ma_connection.c
                          ma_helper.c
                          ma_debug.c
                          ma_dsn.c
                          ma_driver.c
                          ma_info.c
                          ma_environment.c
                          ma_parse.c
                          ma_statement.c
                          ma_desc.c
                          ma_string.c
                          ma_result.c)

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

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/SearchLibrary.cmake)

#Debug log is controlled by conenction option solely
ADD_DEFINITIONS(-DMAODBC_DEBUG)

IF(WIN32)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/win)
  SET(ODBC_LIBS odbc32)
  SET(ODBC_INSTLIBS odbccp32)
  SET(MARIADB_ODBC_SOURCES ${MARIADB_ODBC_SOURCES}
                          ma_dll.c
                          ma_platform_win32.c
                          ma_error.h
                          ma_connection.h
                          ma_helper.h
                          ma_debug.h
                          ma_dsn.h
                          ma_driver.h
                          ma_info.h
                          ma_environment.h
                          ma_parse.h
                          ma_statement.h
                          ma_desc.h
                          ma_string.h
                          ma_odbc.h
                          ma_odbc_version.h
                          ma_result.h)
  SET(DSN_DIALOG_FILES ${DSN_DIALOG_FILES}
                          ma_platform_win32.c)

  SET(MARIADB_CLIENT_FILENAME mariadbclient.lib)
  SET(PLATFORM_DEPENDENCIES ws2_32)
  IF (MSVC)
    MESSAGE(STATUS "MSVC_VERSION= ${MSVC_VERSION}") 
    IF (MSVC_VERSION EQUAL 1900)
      MESSAGE(STATUS "Configuring to link connector against legacy_stdio_definitions") 
      SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} legacy_stdio_definitions)
    ENDIF()
  ENDIF()
ELSE()
  SEARCH_LIBRARY(LIB_MATH floor m)
  SET(PLATFORM_DEPENDENCIES ${LIB_MATH})
  SET (MARIADB_ODBC_SOURCES ${MARIADB_ODBC_SOURCES} ma_platform_posix.c)
  SET(MARIADB_CLIENT_FILENAME libmariadbclient.a)
  SET(ODBC_LIBS odbc)
  SET(ODBC_INSTLIBS odbcinst)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
ENDIF()

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/FindMariaDB.cmake)
IF(MARIADB_FOUND)
  INCLUDE_DIRECTORIES(${MARIADB_INCLUDE_DIR})
  SET(MARIADB_LIB ${MARIADB_LIBRARY_DIR}/${MARIADB_CLIENT_FILENAME})
ELSE()
  MESSAGE(FATAL_ERROR "MariaDB not found")
ENDIF()

SET(CPACK_PACKAGE_VERSION ${MARIADB_ODBC_VERSION_MAJOR}.${MARIADB_ODBC_VERSION_MINOR}.${MARIADB_ODBC_VERSION_PATCH})

# TODO: Make it optional
# Disable dbug information for release builds 
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DDBUG_OFF")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DDBUG_OFF")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -DDBUG_OFF")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DDBUG_OFF")


# We need to determine the last parameter for SQLColAttribute:
# Older UnixODBC version expect SQLPOINTER while Windows expects SQLLEN *
TRY_COMPILE(COMPILE_OK ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake/sqlcolattribute.c)
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

SET(UNICODE "W")

SET(LIBRARY_NAME "maodbc")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/mariadb-odbc-driver.def.in
               ${CMAKE_SOURCE_DIR}/mariadb-odbc-driver-uni.def)
ADD_LIBRARY(maodbc SHARED ${MARIADB_ODBC_SOURCES} ${CMAKE_SOURCE_DIR}/mariadb-odbc-driver-uni.def maodbcu.rc)
SET_TARGET_PROPERTIES(maodbc PROPERTIES LANGUAGE C)
TARGET_LINK_LIBRARIES(maodbc ${MARIADB_LIB} ${ODBC_INSTLIBS} ${PLATFORM_DEPENDENCIES})


# Currently on Windows only
IF(WIN32)
  ADD_LIBRARY(maodbcs SHARED ${DSN_DIALOG_FILES} ${CMAKE_SOURCE_DIR}/dsn/mariadb_odbc_setup.def)
  SET_TARGET_PROPERTIES(maodbcs PROPERTIES LANGUAGE C)
  TARGET_LINK_LIBRARIES(maodbcs comctl32 ${MARIADB_LIB} maodbc ${ODBC_INSTLIBS})

  ADD_EXECUTABLE(dsn_test WIN32 dsn_test.c)
  TARGET_LINK_LIBRARIES(dsn_test ${MARIADB_LIB})
ENDIF()

IF(WIN32)
  ADD_SUBDIRECTORY(wininstall)
ENDIF()

# Tests
ADD_SUBDIRECTORY(test)

# Packaging
SET(CPACK_PACKAGE_VERSION_MAJOR "${MARIADB_ODBC_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${MARIADB_ODBC_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${MARIADB_ODBC_VERSION_PATCH}")
SET(CPACK_PACKAGE_VENDOR "MariaDB Corporation Ab")
SET(CPACK_PACKAGE_DESCRIPTION "MariaDB Connector/ODBC. ODBC driver library for connecting to MariaDB and MySQL servers")
SET(CPACK_PACKAGE_NAME "mariadb_connector_odbc")
STRING(TOLOWER ${CMAKE_SYSTEM_NAME} system_name)
SET(CPACK_PACKAGE_FILE_NAME "mariadb-connector-odbc-${CPACK_PACKAGE_VERSION}-${system_name}-${CMAKE_SYSTEM_PROCESSOR}")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "mariadb-connector-odbc-${CPACK_PACKAGE_VERSION}-src")

SET(CPACK_SOURCE_IGNORE_FILES
.git/
.gitignore
.gitattributes
CMakeCache.txt
cmake_dist.cmake
CPackSourceConfig.cmake
CPackConfig.cmake
.build/
/cmake_install.cmake
/CTestTestfile.cmake
/CMakeFiles/
/version_resources/
/_CPack_Packages/
.*vcxproj
.*gz$
.*zip$
.*so$
.*so.*$
.*dll$
.*a$
.*pdb$
.*sln$
.*sdf$
/CMakeFiles/
/version_resources/
/_CPack_Packages/
Makefile$
/autom4te.cache/
)

# Build source packages
IF(GIT_BUILD_SRCPKG)
  # get branch name
  EXECUTE_PROCESS(COMMAND git show-branch OUTPUT_VARIABLE git_branch)
  STRING(REGEX MATCH "\\[([^]]+)\\]" git_branch ${git_branch})
  STRING(REGEX REPLACE "\\[|\\]" "" GIT_BRANCH ${git_branch})
  MESSAGE(STATUS "Git branch: ${GIT_BRANCH}")
  IF(WIN32)
    EXECUTE_PROCESS(COMMAND git archive ${GIT_BRANCH} --format=zip --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.zip)
  ELSE()
    EXECUTE_PROCESS(COMMAND git archive ${GIT_BRANCH} --format=zip --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.zip)
    EXECUTE_PROCESS(COMMAND git archive ${GIT_BRANCH} --format=tar --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar)
    EXECUTE_PROCESS(COMMAND gzip -9 -f ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar)
  ENDIF()
ENDIF()

IF(WIN32)
  SET(DEFAULT_GENERATOR "ZIP")
ELSE()
  SET(DEFAULT_GENERATOR "TGZ")
ENDIF()

IF(NOT CPACK_GENERATOR)
  SET(CPACK_GENERATOR "${DEFAULT_GENERATOR}")
ENDIF()
IF(NOT CPACK_SOURCE_GENERATOR)
  SET(CPACK_SOURCE_GENERATOR "${DEFAULT_GENERATOR}")
ENDIF()

MESSAGE(STATUS "License File: ${CPACK_RESOURCE_FILE_LICENSE}")
MESSAGE(STATUS "ReadMe File: ${CPACK_PACKAGE_DESCRIPTION_FILE}")
MESSAGE(STATUS "Source Package Filename: ${CPACK_SOURCE_PACKAGE_FILE_NAME}.${CPACK_SOURCE_GENERATOR}")

INCLUDE(CPack)


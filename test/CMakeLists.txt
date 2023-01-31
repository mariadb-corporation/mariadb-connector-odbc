# ************************************************************************************
#   Copyright (C) 2013,2021 MariaDB Corporation AB
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
# ************************************************************************************

IF (DIRECT_LINK_TESTS)
  MESSAGE(STATUS "Configuring tests for direct linking against the connector")
ELSE()
  MESSAGE(STATUS "Configuring tests for linking against the DM library")
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/driver)

# Seems like libodbc needs it
IF(NOT WIN32)
  SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} "pthread" "dl")
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ENABLE_TESTING()

SET (ODBC_TESTS
     "basic" "types" "blob" "desc" "info" "tran" "catalog1" "catalog2"
     "use_result" "scroll" "bulk" "prepare" "datetime" "keys"
     "curext" "relative" "unicode" "cursor" "dyn_cursor"
     "error" "param" "result1" "result2" "multistatement")
SET(COMMON_TEST_SOURCES tap.h)
# Interactive makes sense on WIN32 only atm
IF (BUILD_INTERACTIVE_TESTS)
  MESSAGE(STATUS "Configuring to build interactive test")
  SET (ODBC_TESTS ${ODBC_TESTS} "interactive")
ENDIF()
IF(NOT WIN32)
  SET(COMMON_TEST_SOURCES ${COMMON_TEST_SOURCES} "${CMAKE_SOURCE_DIR}/driver/ma_conv_charset.cpp")
ENDIF()
# iOdbc has 
IF(WITH_IODBC)
  SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wno-pointer-sign")
ENDIF()
FOREACH (ODBC_TEST ${ODBC_TESTS})
  ADD_EXECUTABLE(odbc_${ODBC_TEST} "${ODBC_TEST}.c" ${COMMON_TEST_SOURCES})
  SET_TARGET_PROPERTIES(odbc_${ODBC_TEST} PROPERTIES
                        LANGUAGE C)
  IF (DIRECT_LINK_TESTS)
    TARGET_LINK_LIBRARIES(odbc_${ODBC_TEST} maodbc ${PLATFORM_DEPENDENCIES})
  ELSE()
    TARGET_LINK_LIBRARIES(odbc_${ODBC_TEST} ${ODBC_LIBS} ${PLATFORM_DEPENDENCIES})
  ENDIF()
  IF (NOT ${ODBC_TEST} STREQUAL "interactive" OR USE_INTERACTIVE_TESTS)
    ADD_TEST(odbc_${ODBC_TEST} ${EXECUTABLE_OUTPUT_PATH}/odbc_${ODBC_TEST})
    SET_TESTS_PROPERTIES(odbc_${ODBC_TEST} PROPERTIES TIMEOUT 600)
  ENDIF()
ENDFOREACH()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/driver)
ADD_EXECUTABLE(odbc_connstring connstring.c ${CMAKE_SOURCE_DIR}/driver/ma_dsn.c ${COMMON_TEST_SOURCES})
TARGET_LINK_LIBRARIES(odbc_connstring ${ODBC_LIBS} ${ODBC_INSTLIBS} mariadbclient ${PLATFORM_DEPENDENCIES})
ADD_TEST(odbc_connstring ${EXECUTABLE_OUTPUT_PATH}/odbc_connstring)
SET_TESTS_PROPERTIES(odbc_connstring PROPERTIES TIMEOUT 120)
SET_TESTS_PROPERTIES(odbc_basic odbc_catalog2 odbc_cursor odbc_multistatement  PROPERTIES TIMEOUT 450)

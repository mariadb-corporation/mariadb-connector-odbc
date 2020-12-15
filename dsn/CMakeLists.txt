# ************************************************************************************
#   Copyright (C) 2020 MariaDB Corporation AB
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


# Currently on Windows only
IF(WIN32)
  MESSAGE(STATUS "Configuring build of the setup library")
  SET(DSN_DIALOG_FILES ${CMAKE_SOURCE_DIR}/dsn/odbc_dsn.c
                       ${CMAKE_SOURCE_DIR}/dsn/odbc_dsn.rc
                       ${CMAKE_SOURCE_DIR}/dsn/resource.h
                       ${CMAKE_SOURCE_DIR}/driver/ma_dsn.c
                       ${CMAKE_SOURCE_DIR}/driver/ma_common.c)

  ####### MAODBCS #######
  ADD_LIBRARY(maodbcs SHARED ${DSN_DIALOG_FILES} ${CMAKE_SOURCE_DIR}/dsn/mariadb_odbc_setup.def)
  SET_TARGET_PROPERTIES(maodbcs PROPERTIES
	                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
                        # Specifically on Windows, to have implib in the same place as the dll
	                      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
                        LANGUAGE C)
  TARGET_LINK_LIBRARIES(maodbcs comctl32 ${LEGACY_STDIO} Shlwapi ${ODBC_LIBS} ${ODBC_INSTLIBS})
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/driver)
  INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/driver)
ELSE()
  MESSAGE(STATUS "Setup library is not supported on this platform")
ENDIF()

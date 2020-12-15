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


CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/driver/ma_odbc_version.h.in
               ${CMAKE_BINARY_DIR}/driver/ma_odbc_version.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/driver/maodbcu.rc.in
               ${CMAKE_BINARY_DIR}/driver/maodbcu.rc)

SET (MARIADB_ODBC_SOURCES odbc_3_api.cpp
                          ma_api_internal.cpp   
                          ma_error.cpp
                          ma_connection.cpp
                          ma_helper.cpp
                          ma_debug.cpp
                          ma_dsn.c
                          ma_driver.cpp
                          ma_info.cpp
                          ma_environment.cpp
                          ma_parse.cpp
                          ma_statement.cpp
                          ma_desc.cpp
                          ma_string.cpp
                          ma_result.cpp
                          ma_common.c
                          ma_server.cpp
                          ma_legacy_helpers.cpp
                          ma_typeconv.cpp
                          ma_bulk.cpp)

IF(WIN32)
  SET(ODBC_LIBS odbc32)
  SET(ODBC_INSTLIBS odbccp32)
  SET(MARIADB_ODBC_SOURCES ${MARIADB_ODBC_SOURCES}
                          ma_dll.c
                          ma_platform_win32.cpp
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
                          ma_api_internal.h
                          ma_odbc_version.h
                          ma_result.h
                          ma_server.h
                          ma_legacy_helpers.h
                          ma_typeconv.h
                          ma_bulk.h
                          ma_c_stuff.h)

ELSE()
  SET (MARIADB_ODBC_SOURCES ${MARIADB_ODBC_SOURCES}
                        ma_platform_posix.cpp
                        ma_conv_charset.cpp)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/driver)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/driver)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/driver/mariadb-odbc-driver.def.in
               ${CMAKE_BINARY_DIR}/driver/mariadb-odbc-driver-uni.def)

IF(MARIADB_LINK_DYNAMIC)# OR USE_SYSTEM_INSTALLED_LIB)
  IF(USE_SYSTEM_INSTALLED_LIB)
    SET(MARIADB_CLIENT_TARGET_NAME mariadb)
  ELSE()
    SET(MARIADB_CLIENT_TARGET_NAME libmariadb)
  ENDIF()
  MESSAGE(STATUS "Linking Connector/C library dynamically(${MARIADB_CLIENT_TARGET_NAME})")
ELSE()
  SET(MARIADB_CLIENT_TARGET_NAME mariadbclient)
  MESSAGE(STATUS "Linking Connector/C library statically(${MARIADB_CLIENT_TARGET_NAME})")
ENDIF()

IF(WIN32)
  ADD_LIBRARY(${LIBRARY_NAME} SHARED ${MARIADB_ODBC_SOURCES} ${CMAKE_BINARY_DIR}/driver/mariadb-odbc-driver-uni.def ${CMAKE_BINARY_DIR}/driver/maodbcu.rc)
ELSE()
  MESSAGE(STATUS "Version script: ${CMAKE_SOURCE_DIR}/driver/maodbc.def")
  ADD_LIBRARY(${LIBRARY_NAME} SHARED ${MARIADB_ODBC_SOURCES} maodbcu.rc)
  
  IF(APPLE)
    SET(MAODBC_INSTALL_RPATH "${ODBC_LIB_DIR}" "@loader_path" "/usr/local/opt/libiodbc" "/usr/local/iODBC/lib" "/usr/local/opt/openssl@1.1/lib" "/usr/local/opt/libressl/lib")
    SET_TARGET_PROPERTIES(${LIBRARY_NAME}
                          PROPERTIES LINK_FLAGS "-Wl"
                                     INSTALL_RPATH_USE_LINK_PATH 0
                                     BUILD_WITH_INSTALL_RPATH 1
                                     INSTALL_RPATH "${MAODBC_INSTALL_RPATH}")
  ELSE()
    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_SOURCE_DIR}/driver/maodbc.def")
  ENDIF()
ENDIF()

SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES
	                    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
                      # Specifically on Windows, to have implib in the same place as the dll
	                    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
                      LANGUAGE CXX)
MESSAGE(STATUS "All linked targets/external dependencies: ${MARIADB_CLIENT_TARGET_NAME} ${ODBC_INSTLIBS} ${PLATFORM_DEPENDENCIES}")
TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${MARIADB_CLIENT_TARGET_NAME} ${ODBC_INSTLIBS} ${PLATFORM_DEPENDENCIES})

# Currently on Windows only
IF(WIN32)
  ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_DIR=$<TARGET_FILE_DIR:maodbc>
                                  -DPLUGINS_LIB_DIR=$<TARGET_FILE_DIR:dialog>
                                  # Implib may be in diff directory as dll. We currently care to have it in the same folder, but let this just stay
                                  -DIMPORT_LIB_DIR=$<TARGET_LINKER_FILE_DIR:maodbc>
                                  -DINSTALLER_TOOLS_DIR=$<TARGET_FILE_DIR:change_dsns_driver>
                                  -DPLUGINS_SUBDIR_NAME=${MARIADB_DEFAULT_PLUGINS_SUBDIR}
                                  -DFILE_IN=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml.in
                                  -DFILE_OUT=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml
                                  -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                     )
ELSEIF(APPLE)
  ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/osxpostbuild.sh ARGS $<TARGET_FILE:${LIBRARY_NAME}>
  )
ENDIF()

# Configuring ini files for testing
IF(EXISTS "${CMAKE_SOURCE_DIR}/test/CMakeLists.txt")
  IF(WITH_UNIT_TESTS)
    IF(NOT WIN32)
      # Configuring ini files for testing with UnixODBC
      SET_VALUE(TEST_DRIVER "maodbc_test")
      SET_VALUE(TEST_DSN "maodbc_test")
      SET_VALUE(TEST_PORT "3306")
      SET_VALUE(TEST_SERVER "localhost")
      SET_VALUE(TEST_SOCKET "")
      SET_VALUE(TEST_SCHEMA "test")
      SET_VALUE(TEST_UID "root")
      SET_VALUE(TEST_PASSWORD "")

      MESSAGE(STATUS "Configurig Test Driver: ${TEST_DRIVER}, Test DSN: ${TEST_DSN}, tcp://${TEST_UID}@${TEST_SERVER}:${TEST_PORT}/${TEST_SCHEMA} socket: ${TEST_SOCKET}")

      ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_LOCATION=$<TARGET_FILE:maodbc>
                                      -DTEST_DRIVER=${TEST_DRIVER}
                                      -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbcinst.ini.in
                                      -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbcinst.ini
                                      -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                         )

      ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -DTEST_DRIVER=${TEST_DRIVER}
                                      -DTEST_DSN=${TEST_DSN}
                                      -DTEST_PORT=${TEST_PORT}
                                      -DTEST_SERVER=${TEST_SERVER}
                                      -DTEST_SOCKET=${TEST_SOCKET}
                                      -DTEST_SCHEMA=${TEST_SCHEMA}
                                      -DTEST_UID=${TEST_UID}
                                      -DTEST_PASSWORD=${TEST_PASSWORD}
                                      -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbc.ini.in
                                      -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbc.ini
                                      -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                         )
    ENDIF()
  ENDIF()
ENDIF()
INSTALL(TARGETS ${LIBRARY_NAME}
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        COMPONENT ODBCLibs)


# ************************************************************************************
#   Copyright (C) 2020,2025 MariaDB Corporation plc
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

IF(NOT ${CMAKE_VERSION} VERSION_LESS "3.20.0")
  CMAKE_POLICY(SET CMP0115 NEW)
ENDIF()
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/ma_odbc_version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/ma_odbc_version.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/maodbcu.rc.in
               ${CMAKE_CURRENT_BINARY_DIR}/maodbcu.rc)

# We need to update submodule if files are not present and still have the oportunity to build using arbitrary commit
INCLUDE(FindGit)
IF(NOT GIT_EXECUTABLE)
  MESSAGE(STATUS "Git has nobeen found")
ENDIF()
IF(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cppmini/class AND GIT_EXECUTABLE)
  MESSAGE(STATUS "C/C++ files are not found - updating submodules")
  EXECUTE_PROCESS(COMMAND "${GIT_EXECUTABLE}" submodule init
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  EXECUTE_PROCESS(COMMAND "${GIT_EXECUTABLE}" submodule update
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
ENDIF()
IF(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cppmini/class)
  MESSAGE(FATAL_ERROR "No MariaDB Connector/C++ mini branch! Run
  git submodule init
  git submodule update
  Then restart the build.
")
ENDIF()

SET(MARIADB_CPPMINI_FILES
                          cppmini/class/Results.cpp
                          cppmini/class/TextRow.cpp
                          cppmini/class/BinRow.cpp
                          cppmini/class/ClientSidePreparedStatement.cpp
                          cppmini/class/ServerSidePreparedStatement.cpp
                          cppmini/class/SSPSDirectExec.cpp
                          cppmini/class/ClientPrepareResult.cpp
                          cppmini/class/ServerPrepareResult.cpp
                          cppmini/class/CmdInformationSingle.cpp
                          cppmini/class/CmdInformationMultiple.cpp
                          cppmini/class/CmdInformationBatch.cpp
                          cppmini/class/ColumnDefinition.cpp
                          cppmini/class/ResultSetText.cpp
                          cppmini/class/ResultSetBin.cpp
                          cppmini/class/ResultSetMetaData.cpp
                          cppmini/class/Parameter.cpp
                          cppmini/class/Protocol.cpp
                          cppmini/interface/PreparedStatement.cpp
                          cppmini/interface/Row.cpp
                          cppmini/interface/ResultSet.cpp
                          cppmini/interface/PrepareResult.cpp
                          cppmini/interface/Exception.cpp
                          cppmini/template/CArray.cpp
                          )
# Propogating it to the parent level as it needed for source package generation. If we wet it for parent_scope in first place,
# it won't be visible in this scope                        
SET(MARIADB_CPPMINI_FILES ${MARIADB_CPPMINI_FILES} PARENT_SCOPE)

SET(MARIADB_CPPMINI_HEADERS
                          cppmini/class/SQLString.h
                          cppmini/class/Results.h
                          cppmini/class/TextRow.h
                          cppmini/class/BinRow.h
                          cppmini/class/ClientSidePreparedStatement.h
                          cppmini/class/ServerSidePreparedStatement.h
                          cppmini/class/SSPSDirectExec.h
                          cppmini/class/ClientPrepareResult.h
                          cppmini/class/ServerPrepareResult.h
                          cppmini/class/CmdInformationSingle.h
                          cppmini/class/CmdInformationMultiple.h
                          cppmini/class/CmdInformationBatch.h
                          cppmini/class/ColumnDefinition.h
                          cppmini/class/ResultSetText.h
                          cppmini/class/ResultSetBin.h
                          cppmini/class/ResultSetMetaData.h
                          cppmini/class/Parameter.h
                          cppmini/class/pimpls.h
                          cppmini/class/Protocol.h
                          cppmini/class/lru/lrucache.h
                          cppmini/class/lru/pscache.h
                          cppmini/interface/PreparedStatement.h
                          cppmini/interface/PrepareResult.h
                          cppmini/interface/Row.h
                          cppmini/interface/ResultSet.h
                          cppmini/interface/CmdInformation.h
                          cppmini/interface/Exception.h
                          cppmini/template/CArray.h
                          )
SET(MARIADB_CPPMINI_HEADERS ${MARIADB_CPPMINI_HEADERS} PARENT_SCOPE)

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
                          ma_catalog.cpp
                          ma_statement.cpp
                          ma_desc.cpp
                          ma_string.cpp
                          ma_result.cpp
                          ma_common.c
                          ma_server.cpp
                          ma_legacy_helpers.cpp
                          ma_typeconv.cpp
                          ma_bulk.cpp
                          ma_codec.cpp
                          ${MARIADB_CPPMINI_FILES}
                          )
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
                          ma_catalog.h
                          ma_statement.h
                          ma_desc.h
                          ma_string.h
                          ma_odbc.h
                          ma_api_internal.h
                          ${CMAKE_CURRENT_BINARY_DIR}/ma_odbc_version.h
                          ma_result.h
                          ma_server.h
                          ma_legacy_helpers.h
                          ma_typeconv.h
                          ma_bulk.h
                          ma_codec.h
                          ma_c_stuff.h
                          ${MARIADB_CPPMINI_HEADERS}
                          )

  SOURCE_GROUP(Classes REGULAR_EXPRESSION "class/")
  SOURCE_GROUP(Interfaces REGULAR_EXPRESSION "interface/")
  SOURCE_GROUP(Templates REGULAR_EXPRESSION "template/")

ELSE()
  SET (MARIADB_ODBC_SOURCES ${MARIADB_ODBC_SOURCES}
                        ma_platform_posix.cpp
                        ma_conv_charset.cpp)
ENDIF()

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cppmini" "${CMAKE_CURRENT_SOURCE_DIR}/cppmini/interface" "${CMAKE_CURRENT_SOURCE_DIR}/cppmini/template" "${CMAKE_CURRENT_SOURCE_DIR}/cppmini/class" "${CMAKE_CURRENT_BINARY_DIR}")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/driver/mariadb-odbc-driver.def.in
               ${CMAKE_BINARY_DIR}/driver/mariadb-odbc-driver-uni.def)

IF(MARIADB_LINK_DYNAMIC)# OR USE_SYSTEM_INSTALLED_LIB)
  IF(USE_SYSTEM_INSTALLED_LIB)
    FIND_LIBRARY(CCLIB libmariadb.so)
    IF (${CCLIB} STREQUAL "CCLIB-NOTFOUND")
      # It won't be found by linker either unless user does some magic before build, so we actually could stop here with error
      MESSAGE(STATUS "C/C library has not been found")
      SET(MARIADB_CLIENT_TARGET_NAME mariadb)
    ELSE()
      MESSAGE(STATUS "C/C library found here ${CCLIB}")
      SET(MARIADB_CLIENT_TARGET_NAME ${CCLIB})
    ENDIF()
    FIND_FILE(CCHEADER NAMES "mysql.h"
              PATHS /usr/include/mariadb
                    /usr/include/mysql
                    /usr/local/include/mariadb
                    /usr/local/include/mysql)
              #PATH_SUFFIXES mariadb mysql)
    IF (${CCHEADER} STREQUAL "CCHEADER-NOTFOUND")
      # Again should probably stop
      MESSAGE(STATUS "C/C headers have not been found")
      INCLUDE_DIRECTORIES(/usr/include/mariadb /usr/include/mysql)
      SET(CCHEADERDIR "/usr/include/mariadb /usr/include/mysql")
    ELSE()
      GET_FILENAME_COMPONENT(CCHEADERDIR ${CCHEADER} DIRECTORY)
      MESSAGE(STATUS "C/C headers have been found here ${CCHEADERDIR}")
      INCLUDE_DIRECTORIES(${CCHEADERDIR})
    ENDIF()
    SET(CCHEADERDIR "${CCHEADERDIR}" PARENT_SCOPE)
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
                                     XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                                     XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp -f"
                                     INSTALL_RPATH "${MAODBC_INSTALL_RPATH}")
    IF(WITH_SIGNCODE)
      SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: ${DEVELOPER_ID}")
    ELSE()
      SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
    ENDIF()
    ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
                       COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/osxpostbuild.sh ARGS $<TARGET_FILE:${LIBRARY_NAME}>
    )
  ELSEIF(NOT CMAKE_SYSTEM_NAME MATCHES AIX)
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
ADD_DEPENDENCIES(${LIBRARY_NAME} DEPENDENCIES_FOR_PACKAGE)

IF(WITH_MSI)
  IF(ALL_PLUGINS_STATIC)
    ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_DIR=$<TARGET_FILE_DIR:${LIBRARY_NAME}>
                                    -DPLUGINS_LIB_DIR=""
                                    # Implib may be in diff directory as dll. We currently care to have it in the same folder, but let this just stay
                                    -DIMPORT_LIB_DIR=$<TARGET_LINKER_FILE_DIR:${LIBRARY_NAME}>
                                    -DINSTALLER_TOOLS_DIR=$<TARGET_FILE_DIR:change_dsns_driver>
                                    -DPLUGINS_SUBDIR_NAME=${MARIADB_DEFAULT_PLUGINS_SUBDIR}
                                    -DSOURCE_ROOT_DIR=${CMAKE_SOURCE_DIR}
                                    -DFILE_IN=${CMAKE_SOURCE_DIR}/packaging/windows/binaries_dir.xml.in
                                    -DFILE_OUT=${CMAKE_BINARY_DIR}/packaging/windows/binaries_dir.xml
                                    -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake)
  ELSE()
    ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_DIR=$<TARGET_FILE_DIR:${LIBRARY_NAME}>
                                    -DPLUGINS_LIB_DIR=$<TARGET_FILE_DIR:dialog>
                                    # Implib may be in diff directory as dll. We currently care to have it in the same folder, but let this just stay
                                    -DIMPORT_LIB_DIR=$<TARGET_LINKER_FILE_DIR:${LIBRARY_NAME}>
                                    -DINSTALLER_TOOLS_DIR=$<TARGET_FILE_DIR:change_dsns_driver>
                                    -DPLUGINS_SUBDIR_NAME=${MARIADB_DEFAULT_PLUGINS_SUBDIR}
                                    -DSOURCE_ROOT_DIR=${CMAKE_SOURCE_DIR}
                                    -DFILE_IN=${CMAKE_SOURCE_DIR}/packaging/windows/binaries_dir.xml.in
                                    -DFILE_OUT=${CMAKE_BINARY_DIR}/packaging/windows/binaries_dir.xml
                                    -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake)
                                  
  ENDIF()
ELSEIF(APPLE)
  ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/osxpostbuild.sh ARGS $<TARGET_FILE:${LIBRARY_NAME}>
  )
ENDIF()

# Configuring ini files for testing
IF(EXISTS "${CMAKE_SOURCE_DIR}/test/CMakeLists.txt")
  IF(WITH_UNIT_TESTS)
    IF(NOT WIN32)
      IF(DIRECT_LINK_TESTS)
        ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} ARGS -E copy $<TARGET_FILE:${LIBRARY_NAME}> test)
      ENDIF()
      # Configuring ini files for testing with UnixODBC
      SET_VALUE(TEST_DRIVER "maodbc_test")
      SET_VALUE(TEST_DSN "maodbc_test")
      SET_VALUE(TEST_PORT "3306")
      SET_VALUE(TEST_SERVER "localhost")
      SET_VALUE(TEST_SOCKET "")
      SET_VALUE(TEST_SCHEMA "test")
      SET_VALUE(TEST_UID "root")
      SET_VALUE(TEST_PASSWORD "")

      # Configuring ini files for testing with UnixODBC
      MESSAGE(STATUS "Configurig Test Driver: ${TEST_DRIVER}, Test DSN: ${TEST_DSN}, tcp://${TEST_UID}@${TEST_SERVER}:${TEST_PORT}/${TEST_SCHEMA} socket: ${TEST_SOCKET}")

      # If deb or rpm package is built, we configure tests to use the driver installed with the packages
      IF(DEB OR RPM)
        ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_LOCATION=${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}/lib${LIBRARY_NAME}.so
                                        -DTEST_DRIVER=${TEST_DRIVER}
                                        -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbcinst.ini.in
                                        -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbcinst.ini
                                        -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                           )

        ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} ARGS -DTEST_DRIVER=${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}/lib${LIBRARY_NAME}.so
                                        -DTEST_DSN=${TEST_DSN}
                                        -DTEST_PORT=${TEST_PORT}
                                        -DTEST_SERVER=${TEST_SERVER}
                                        -DTEST_SOCKET=${TEST_SOCKET}
                                        -DTEST_SCHEMA=${TEST_SCHEMA}
                                        -DTEST_UID=${TEST_UID}
                                        -DTEST_PASSWORD="${TEST_PASSWORD}"
                                        -DTEST_USETLS=${TEST_USETLS}
                                        #-DTEST_SSLVERIFY=${TEST_SSLVERIFY}
                                        -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbc.ini.in
                                        -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbc.ini
                                        -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                           )
      ELSE()
        ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_LOCATION=$<TARGET_FILE:${LIBRARY_NAME}>
                                        -DTEST_DRIVER=${TEST_DRIVER}
                                        -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbcinst.ini.in
                                        -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbcinst.ini
                                        -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                           )

        ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} ARGS -DTEST_DRIVER=$<TARGET_FILE:${LIBRARY_NAME}>
                                        -DTEST_DSN=${TEST_DSN}
                                        -DTEST_PORT=${TEST_PORT}
                                        -DTEST_SERVER=${TEST_SERVER}
                                        -DTEST_SOCKET=${TEST_SOCKET}
                                        -DTEST_SCHEMA=${TEST_SCHEMA}
                                        -DTEST_UID=${TEST_UID}
                                        -DTEST_PASSWORD=${TEST_PASSWORD}
                                        -DTEST_USETLS=${TEST_USETLS}
                                        #-DTEST_SSLVERIFY=${TEST_SSLVERIFY}
                                        -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbc.ini.in
                                        -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbc.ini
                                        -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                        )
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()
INSTALL(TARGETS ${LIBRARY_NAME}
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        COMPONENT ODBCLibs)


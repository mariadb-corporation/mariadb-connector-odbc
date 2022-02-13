# ************************************************************************************
#   Copyright (C) 2013,2022 MariaDB Corporation AB
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

CMAKE_POLICY(SET CMP0048 NEW)
PROJECT(mariadb_connector_odbc C)

cmake_minimum_required(VERSION 2.8)

SET(MARIADB_ODBC_VERSION_MAJOR 3)
SET(MARIADB_ODBC_VERSION_MINOR 1)
SET(MARIADB_ODBC_VERSION_PATCH 16)
SET(MARIADB_ODBC_VERSION_QUALITY "ga")

SET(MARIADB_ODBC_VERSION "03.01.0016")

SET(MARIADB_DEFAULT_PLUGINS_SUBDIR "plugin")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/ma_odbc_version.h.in
               ${CMAKE_SOURCE_DIR}/ma_odbc_version.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/maodbcu.rc.in
               ${CMAKE_SOURCE_DIR}/maodbcu.rc)

SET (MARIADB_ODBC_SOURCES odbc_3_api.c
                          ma_api_internal.c
                          ma_error.c
                          ma_connection.c
                          ma_helper.c
                          ma_debug.c
                          ma_dsn.c
                          ma_driver.c
                          ma_info.c
                          ma_environment.c
                          ma_parse.c
                          ma_catalog.c
                          ma_statement.c
                          ma_desc.c
                          ma_string.c
                          ma_result.c
                          ma_common.c
                          ma_server.c
                          ma_legacy_helpers.c
                          ma_typeconv.c
                          ma_bulk.c)

SET(DSN_DIALOG_FILES ${CMAKE_SOURCE_DIR}/dsn/odbc_dsn.c
                     ${CMAKE_SOURCE_DIR}/dsn/odbc_dsn.rc
                     ${CMAKE_SOURCE_DIR}/dsn/resource.h
                     ma_dsn.c
                     ma_common.c)

MACRO(ADD_OPTION _name _text _default)
  IF(NOT DEFINED ${_name})
    OPTION(${OPT}${_name} "${_text}" "${_default}")
  ELSE()
    OPTION(${OPT}${_name} "${_text}" "${${_name}}")
  ENDIF()
ENDMACRO()

# This has to be before C/C's cmake run, or it will build with /MD
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
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)# -DWIN32_LEAN_AND_MEAN)
  SET(INSTALL_PLUGINDIR "${MARIADB_DEFAULT_PLUGINS_SUBDIR}")
ENDIF()

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/SearchLibrary.cmake)
INCLUDE(${CMAKE_SOURCE_DIR}/cmake/SetValueMacro.cmake)

### Build options, initial settings and platform defaults
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/options_defaults.cmake")

### Setting installation paths - should go before C/C subproject sets its own. We need to have control over those
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/install.cmake")


IF(WIN32 OR WITH_OPENSSL OR "${WITH_SSL}" STREQUAL  "OPENSSL")
  # If C/C is linked dynamically, we don't need link C/ODBC against encryption library
  IF (NOT MARIADB_LINK_DYNAMIC)
    IF(WITH_OPENSSL OR "${WITH_SSL}" STREQUAL  "OPENSSL")
      FIND_PACKAGE(OpenSSL)

      IF(OPENSSL_FOUND)
        MESSAGE(STATUS "Configuring to build with OpenSSL ${OPENSSL_LIBRARIES}")

        ADD_DEFINITIONS(-DHAVE_OPENSSL)
        INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
        SET(SSL_LIBRARIES ${OPENSSL_LIBRARIES})

        SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${SSL_LIBRARIES})
      ELSE()
        MESSAGE(FATAL_ERROR "OpenSSL not found. Please install OpenSSL or disable SSL support via option -DWITH_OPENSSL=Off")
      ENDIF()
    ELSE()
      MESSAGE(STATUS "Configuring SSL support using SChannel")
      SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} version.lib)
    ENDIF()
  ENDIF()
ELSE()
  MESSAGE(STATUS "Configuring to build without SSL support")
ENDIF()

ADD_CUSTOM_TARGET(DEPENDENCIES_FOR_PACKAGE)
### Including C/C subproject
IF(NOT USE_SYSTEM_INSTALLED_LIB)
  IF(GIT_BUILD_SRCPKG)
    # We don't want with conn/c (wrong) src pkg to be built.
    SET(GIT_BUILD_SRCPKG FALSE)
    SET(ODBC_GIT_BUILD_SRCPKG TRUE)
  ENDIF()
  MESSAGE(STATUS "Running C/C cmake scripts")
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/connector_c.cmake)

  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libmariadb/include)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/libmariadb/include)
  SET(PLUGINS_LIST dialog caching_sha2_password auth_gssapi_client sha256_password mysql_clear_password client_ed25519)
  IF(APPLE)
    SET_TARGET_PROPERTIES(${PLUGINS_LIST}
                          PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                                     XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp -f"
                         )
    IF(WITH_SIGNCODE)
      SET_TARGET_PROPERTIES(${PLUGINS_LIST} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: ${DEVELOPER_ID}")
    ELSE()
      SET_TARGET_PROPERTIES(${PLUGINS_LIST} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
    ENDIF()
  ELSE()
    INSTALL(FILES
            $<TARGET_FILE:libmariadb>
            DESTINATION ${INSTALL_LIBDIR}
            COMPONENT ConCLib)
    MESSAGE(STATUS "Configuring to install libmariadb to ${INSTALL_LIBDIR}")
    IF(NOT ALL_PLUGINS_STATIC)
      SET(OWN_PLUGINS_LIST mysql_clear_password dialog client_ed25519 sha256_password caching_sha2_password)
      IF (PLUGINS_DYNAMIC)
        # The list from CC is visible for us
        SET(PLUGINS_LIST ${PLUGINS_DYNAMIC})
      ELSE()
        SET(PLUGINS_LIST ${OWN_PLUGINS_LIST})
      ENDIF()
      FOREACH(CC_PLUGIN ${PLUGINS_LIST})
        IF(NOT PLUGINS_DYNAMIC OR "${PLUGIN_${CC_PLUGIN}_TYPE}" STREQUAL "MARIADB_CLIENT_PLUGIN_AUTH")
          MESSAGE(STATUS "Configuring to install ${CC_PLUGIN} to ${INSTALL_PLUGINDIR}")
          ADD_DEPENDENCIES(DEPENDENCIES_FOR_PACKAGE ${CC_PLUGIN})
          INSTALL(FILES
                  $<TARGET_FILE:${CC_PLUGIN}>
                  DESTINATION ${INSTALL_PLUGINDIR}
                  COMPONENT ConCPlugins)
        ENDIF()
      ENDFOREACH()
    ENDIF()
  ENDIF()
ELSE()
  # Adding mariadb subdirs of standard include locations
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "/usr/local/include/mariadb")
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "/usr/include/mariadb")
  MESSAGE(STATUS "Linking against libmariadb installed on the system")
ENDIF()

IF(WITH_SIGNCODE)
  IF(WIN32 AND NOT SIGN_OPTIONS)
    SET(SIGN_OPTIONS /a /t http://timestamp.verisign.com/scripts/timstamp.dll)
  ELSE()
    SEPARATE_ARGUMENTS(SIGN_OPTIONS)
  ENDIF()
  MARK_AS_ADVANCED(SIGN_OPTIONS)
ENDIF()

#Debug log is controlled by conenction option solely
ADD_DEFINITIONS(-DMAODBC_DEBUG)

IF(WIN32)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/dsn)
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
                          ma_catalog.h
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
                          ma_bulk.h)

			SET(PLATFORM_DEPENDENCIES ws2_32 Shlwapi Pathcch)
  IF (MSVC)
    MESSAGE(STATUS "MSVC_VERSION= ${MSVC_VERSION}")
    IF (NOT(MSVC_VERSION LESS 1900))
      MESSAGE(STATUS "Configuring to link connector against legacy_stdio_definitions")
      SET(LEGACY_STDIO legacy_stdio_definitions)
      SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${LEGACY_STDIO})
    ENDIF()
  ENDIF()
ELSE()
  SEARCH_LIBRARY(LIB_MATH floor m)
  SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${LIB_MATH})
  SET (MARIADB_ODBC_SOURCES ${MARIADB_ODBC_SOURCES}
                        ma_platform_posix.c
                        ma_conv_charset.c)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
ENDIF()

IF(NOT WIN32)
  # Looking for DM(UnixODBC) files
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/FindDM.cmake)

  IF(DM_FOUND)
    INCLUDE_DIRECTORIES(${ODBC_INCLUDE_DIR})
    LINK_DIRECTORIES(${ODBC_LIB_DIR})
  ELSE()
    MESSAGE(FATAL_ERROR "Driver Manager was not found")
  ENDIF()
ENDIF()

IF(APPLE OR CMAKE_SYSTEM_NAME MATCHES AIX)
  IF(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_HOST_SYSTEM_VERSION VERSION_GREATER_EQUAL 20)
    SET(ICONV_LIBRARIES "-liconv")
    SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${ICONV_LIBRARIES})
  ELSE()
    # Looking for iconv files
    INCLUDE(${CMAKE_SOURCE_DIR}/cmake/FindIconv.cmake)
    IF(ICONV_FOUND)
      INCLUDE_DIRECTORIES(${ICONV_INCLUDE_DIR})
      SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${ICONV_LIBRARIES})
    ELSE()
      MESSAGE(FATAL_ERROR "iconv was not found")
    ENDIF()
  ENDIF()
ENDIF()

# By now we have everything needed by tests. If we need to build them only - firing config now and exit
# There is "normal" tests config below
IF(BUILD_TESTS_ONLY)
  ADD_SUBDIRECTORY(test)
  IF(NOT WIN32)
    # Configuring ini files for testing with UnixODBC
    MESSAGE(STATUS "Configurig Test Driver: ${TEST_DRIVER}, Test DSN: ${TEST_DSN}, tcp://${TEST_UID}@${TEST_SERVER}:${TEST_PORT}/${TEST_SCHEMA} socket: ${TEST_SOCKET}")

    SET(DRIVER_LIB_LOCATION "${libmaodbc_prefix}/${INSTALL_LIBDIR}")
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/test/odbcinst.ini.in ${CMAKE_BINARY_DIR}/test/odbcinst.ini)

    SET(TEST_DRIVER "${DRIVER_LIB_LOCATION}")
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/test/odbc.ini.in ${CMAKE_BINARY_DIR}/test/odbc.ini)
  ENDIF()

  RETURN()
ENDIF()

SET(CPACK_PACKAGE_VERSION ${MARIADB_ODBC_VERSION_MAJOR}.${MARIADB_ODBC_VERSION_MINOR}.${MARIADB_ODBC_VERSION_PATCH})

SET(CPACK_COMPONENTS_ALL ClientPlugins ODBCLibs Documentation)
SET(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)

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
  SET(UNICODE "W")
ELSE()
  IF (DIRECT_LINK_TESTS)
    ADD_DEFINITIONS(-DHAVE_UNICODE)
  ENDIF()
ENDIF()

SET(LIBRARY_NAME "maodbc")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/mariadb-odbc-driver.def.in
               ${CMAKE_SOURCE_DIR}/mariadb-odbc-driver-uni.def)

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
  ADD_LIBRARY(${LIBRARY_NAME} SHARED ${MARIADB_ODBC_SOURCES} ${CMAKE_SOURCE_DIR}/mariadb-odbc-driver-uni.def maodbcu.rc)
ELSE()
  MESSAGE(STATUS "Version script: ${CMAKE_SOURCE_DIR}/maodbc.def")
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
  ELSEIF(NOT CMAKE_SYSTEM_NAME MATCHES AIX)
    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_SOURCE_DIR}/maodbc.def")
  ENDIF()
ENDIF()

SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LANGUAGE C)
MESSAGE(STATUS "All linked targets/external dependencies: ${MARIADB_CLIENT_TARGET_NAME} ${ODBC_INSTLIBS} ${PLATFORM_DEPENDENCIES}")
TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${MARIADB_CLIENT_TARGET_NAME} ${ODBC_INSTLIBS} ${PLATFORM_DEPENDENCIES})
ADD_DEPENDENCIES(${LIBRARY_NAME} DEPENDENCIES_FOR_PACKAGE)

# Currently on Windows only
IF(WIN32)
  ####### MAODBCS #######
  ADD_LIBRARY(maodbcs SHARED ${DSN_DIALOG_FILES} ${CMAKE_SOURCE_DIR}/dsn/mariadb_odbc_setup.def)
  SET_TARGET_PROPERTIES(maodbcs PROPERTIES LANGUAGE C)
  TARGET_LINK_LIBRARIES(maodbcs comctl32 ${LEGACY_STDIO} Shlwapi ${ODBC_LIBS} ${ODBC_INSTLIBS})

  ADD_EXECUTABLE(dsn_test  dsn_test.c)
  TARGET_LINK_LIBRARIES(dsn_test ${MARIADB_LIB})
ELSEIF(APPLE)
  ADD_CUSTOM_COMMAND(TARGET maodbc POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/osxpostbuild.sh ARGS $<TARGET_FILE:${LIBRARY_NAME}>
  )
ENDIF()

SET(CPACK_PACKAGE_NAME "mariadb-connector-odbc")
IF(WIN32)
  IF(WITH_MSI)
    IF(ALL_PLUGINS_STATIC)
      ADD_CUSTOM_COMMAND(TARGET maodbc POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_DIR=$<TARGET_FILE_DIR:maodbc>
                                      -DPLUGINS_LIB_DIR=""
                                      -DINSTALLER_TOOLS_DIR=$<TARGET_FILE_DIR:change_dsns_driver>
                                      -DPLUGINS_SUBDIR_NAME=${MARIADB_DEFAULT_PLUGINS_SUBDIR}
                                      -DFILE_IN=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml.in
                                      -DFILE_OUT=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml
                                      -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake)
    ELSE()
      ADD_CUSTOM_COMMAND(TARGET maodbc POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_DIR=$<TARGET_FILE_DIR:maodbc>
                                      -DPLUGINS_LIB_DIR=$<TARGET_FILE_DIR:dialog>
                                      -DINSTALLER_TOOLS_DIR=$<TARGET_FILE_DIR:change_dsns_driver>
                                      -DPLUGINS_SUBDIR_NAME=${MARIADB_DEFAULT_PLUGINS_SUBDIR}
                                      -DFILE_IN=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml.in
                                      -DFILE_OUT=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml
                                      -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake)
                                    
    ENDIF()
    ADD_SUBDIRECTORY(wininstall)
  ELSE()
    MESSAGE(STATUS "MSI package won't be generated - WITH_MSI=${WITH_MSI}")
  ENDIF()
  ADD_EXECUTABLE(install_driver osxinstall/install_driver.c)
  TARGET_LINK_LIBRARIES(install_driver ${PLATFORM_DEPENDENCIES} ${ODBC_INSTLIBS})
ELSE()
  IF(APPLE)
    MESSAGE(STATUS "Configuring to generate PKG package")
    ADD_SUBDIRECTORY(osxinstall)
  ELSE()
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/libmaodbc.pc.in
                   ${CMAKE_CURRENT_BINARY_DIR}/libmaodbc.pc @ONLY)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libmaodbc.pc
            DESTINATION "${INSTALL_PCDIR}"
            COMPONENT Development)
  ENDIF()
  INSTALL(TARGETS
          ${LIBRARY_NAME}
          LIBRARY DESTINATION ${INSTALL_LIBDIR}
          COMPONENT ODBCLibs)

  INSTALL(FILES
          ${CMAKE_SOURCE_DIR}/README
          DESTINATION
          "${INSTALL_DOCDIR}"
          COMPONENT Documentation)
  INSTALL(FILES
          ${CMAKE_SOURCE_DIR}/COPYING
          DESTINATION
          "${INSTALL_LICENSEDIR}"
          COMPONENT Documentation)
ENDIF()

# Tests. Should be verified by now, if we have them.
IF(WITH_UNIT_TESTS)
  ADD_SUBDIRECTORY(test)
  IF(NOT WIN32)
    # Configuring ini files for testing with UnixODBC
    MESSAGE(STATUS "Configurig Test Driver: ${TEST_DRIVER}, Test DSN: ${TEST_DSN}, tcp://${TEST_UID}@${TEST_SERVER}:${TEST_PORT}/${TEST_SCHEMA} socket: ${TEST_SOCKET}")

    ADD_CUSTOM_COMMAND(TARGET maodbc POST_BUILD
      COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_LOCATION=$<TARGET_FILE:maodbc>
                                    -DTEST_DRIVER=${TEST_DRIVER}
                                    -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbcinst.ini.in
                                    -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbcinst.ini
                                    -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                       )

    ADD_CUSTOM_COMMAND(TARGET maodbc POST_BUILD
      COMMAND ${CMAKE_COMMAND} ARGS -DTEST_DRIVER=$<TARGET_FILE:maodbc>
                                    -DTEST_DSN=${TEST_DSN}
                                    -DTEST_PORT=${TEST_PORT}
                                    -DTEST_SERVER=${TEST_SERVER}
                                    -DTEST_SOCKET=${TEST_SOCKET}
                                    -DTEST_SCHEMA=${TEST_SCHEMA}
                                    -DTEST_UID=${TEST_UID}
                                    -DTEST_PASSWORD="${TEST_PASSWORD}"
                                    -DTEST_USETLS=${TEST_USETLS}
                                    -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbc.ini.in
                                    -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbc.ini
                                    -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                       )
  ENDIF()
ENDIF()

# Packaging
SET(CPACK_PACKAGE_VENDOR "MariaDB Corporation Ab")
SET(CPACK_PACKAGE_DESCRIPTION "MariaDB Connector/ODBC. ODBC driver library for connecting to MariaDB and MySQL servers")
IF(NOT SYSTEM_NAME)
  STRING(TOLOWER ${CMAKE_SYSTEM_NAME} system_name)
ENDIF()
SET(QUALITY_SUFFIX "")
IF (MARIADB_ODBC_VERSION_QUALITY AND NOT "${MARIADB_ODBC_VERSION_QUALITY}" STREQUAL "ga" AND NOT "${MARIADB_ODBC_VERSION_QUALITY}" STREQUAL "GA")
  SET(QUALITY_SUFFIX "-${MARIADB_ODBC_VERSION_QUALITY}")
ENDIF()
IF(PACKAGE_PLATFORM_SUFFIX)
  SET(CPACK_PACKAGE_FILE_NAME "mariadb-connector-odbc-${CPACK_PACKAGE_VERSION}-${PACKAGE_PLATFORM_SUFFIX}")
ELSE()
 SET(CPACK_PACKAGE_FILE_NAME "mariadb-connector-odbc-${CPACK_PACKAGE_VERSION}${QUALITY_SUFFIX}-${system_name}-${CMAKE_SYSTEM_PROCESSOR}")
ENDIF()
MESSAGE(STATUS "Package Name: ${CPACK_PACKAGE_FILE_NAME}")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "mariadb-connector-odbc-${CPACK_PACKAGE_VERSION}${QUALITY_SUFFIX}-src")
SET(CPACK_PACKAGE_CONTACT "info@mariadb.com")
SET(CPACK_PACKAGE_VENDOR "MariaDB Corporation AB")
SET(CPACK_SOURCE_IGNORE_FILES
/test/
/.git/
.gitignore
.gitmodules
.gitattributes
CMakeCache.txt
cmake_dist.cmake
CPackSourceConfig.cmake
CPackConfig.cmake
/.build/
cmake_install.cmake
CTestTestfile.cmake
/CMakeFiles/
/version_resources/
.*vcxproj
.*gz$
.*zip$
.*so$
.*so.2
.*so.3
.*dll$
.*a$
.*pdb$
.*sln$
.*sdf$
Makefile$
/autom4te.cache/
/.travis/
.travis.yml
/libmariadb/
/_CPack_Packages/
)

# Build source packages
IF(GIT_BUILD_SRCPKG OR ODBC_GIT_BUILD_SRCPKG)
  IF(WIN32)
    EXECUTE_PROCESS(COMMAND git archive --format=zip --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.zip --worktree-attributes -v HEAD)
  ELSE()
    EXECUTE_PROCESS(COMMAND git archive ${GIT_BRANCH} --format=zip --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.zip -v HEAD)
    EXECUTE_PROCESS(COMMAND git archive ${GIT_BRANCH} --format=tar --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar -v HEAD)
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


# ************************************************************************************
#   Copyright (C) 2013,2019 MariaDB Corporation AB
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

SET(MARIADB_ODBC_VERSION_MAJOR 3)
SET(MARIADB_ODBC_VERSION_MINOR 1)
SET(MARIADB_ODBC_VERSION_PATCH 10)
SET(MARIADB_ODBC_VERSION_QUALITY "ga")

SET(MARIADB_ODBC_VERSION "03.01.0010")

SET(MARIADB_DEFAULT_PLUGINS_SUBDIR "plugin")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/ma_odbc_version.h.in
               ${CMAKE_SOURCE_DIR}/ma_odbc_version.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/maodbcu.rc.in
               ${CMAKE_SOURCE_DIR}/maodbcu.rc)

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
#                     ma_error.c
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
ELSE()
  # This has been done before C/C cmake scripts are included
  IF(NOT DEFINED INSTALL_LIB_SUFFIX)
    SET(INSTALL_LIB_SUFFIX "lib" CACHE STRING "Directory, under which to install libraries, e.g. lib or lib64")
    IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8" AND EXISTS "/usr/lib64/")
      SET(INSTALL_LIB_SUFFIX "lib64")
    ENDIF()
  ENDIF()
  MESSAGE(STATUS "Libraries installation dir: ${INSTALL_LIB_SUFFIX}")
  SET(INSTALL_PLUGINDIR "${INSTALL_LIB_SUFFIX}/mariadb/plugin")
  MESSAGE(STATUS "Authentication Plugins installation dir: ${INSTALL_PLUGINDIR}")
ENDIF()

IF(WIN32 OR WITH_OPENSSL OR "${WITH_SSL}" STREQUAL  "OPENSSL")
  IF(WITH_OPENSSL OR "${WITH_SSL}" STREQUAL  "OPENSSL")
    FIND_PACKAGE(OpenSSL)

    IF(OPENSSL_FOUND)
      MESSAGE(STATUS "Configuring to build with OpenSSL ${OPENSSL_LIBRARIES}")

      ADD_DEFINITIONS(-DHAVE_OPENSSL)
      INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
      SET(SSL_LIBRARIES ${OPENSSL_LIBRARIES})

      #IF(OPENSSL_CRYPTO_LIBRARIES)
      #  SET(SSL_LIBRARIES ${SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES})
      #ENDIF()

      SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${SSL_LIBRARIES})
    ELSE()
      MESSAGE(FATAL_ERROR "OpenSSL not found. Please install OpenSSL or disable SSL support via option -DWITH_OPENSSL=Off")
    ENDIF()
  ELSE()
    MESSAGE(STATUS "Configuring SSL support using SChannel")
    SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} version.lib)
  ENDIF()
ELSE()
  MESSAGE(STATUS "Configuring to build without SSL support")
ENDIF()

IF(APPLE)
  SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
  OPTION(WITH_IODBC "Build with iOdbc" ON)
  CMAKE_POLICY(SET CMP0042 NEW)
  CMAKE_POLICY(SET CMP0068 NEW)
  set(CMAKE_INSTALL_RPATH "")
  SET(CMAKE_INSTALL_NAME_DIR "")
  SET(CMAKE_MACOSX_RPATH ON)
ELSE()
  OPTION(WITH_IODBC "Build with iOdbc" OFF)
ENDIF()

ADD_OPTION(WITH_UNIT_TESTS "build test suite" ON)

IF(EXISTS ${CMAKE_SOURCE_DIR}/libmariadb)
  IF(GIT_BUILD_SRCPKG)
    # We don't want conn/c (wrong) src pkg to be built.
    SET(GIT_BUILD_SRCPKG FALSE)
    SET(ODBC_GIT_BUILD_SRCPKG TRUE)
  ENDIF()
  MESSAGE(STATUS "Running C/C cmake scripts")
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/connector_c.cmake)

  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libmariadb/include)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/libmariadb/include)
ELSE()
  SET(USE_SYSTEM_INSTALLED_LIB TRUE)
  # Adding mariadb subdirs of standard include locations
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "/usr/local/include/mariadb")
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "/usr/include/mariadb")
  MESSAGE(STATUS "There is no Connector/C sub-project folder, linking against libmariadb installed on the system")
ENDIF()

IF(WIN32)
  #ADD_OPTION(WITH_MSI "Build MSI installation package" OFF)
  ADD_OPTION(WITH_SIGNCODE "Digitally sign files" OFF)
ENDIF()

#ADD_OPTION(WITH_SSL "Enables use of TLS/SSL library" ON)

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

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/SearchLibrary.cmake)
INCLUDE(${CMAKE_SOURCE_DIR}/cmake/SetValueMacro.cmake)

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
                          ma_statement.h
                          ma_desc.h
                          ma_string.h
                          ma_odbc.h
                          ma_odbc_version.h
                          ma_result.h
                          ma_server.h
                          ma_legacy_helpers.h
                          ma_typeconv.h
                          ma_bulk.h)
                        #  SET(DSN_DIALOG_FILES ${DSN_DIALOG_FILES}
                        #  ma_platform_win32.c)

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
  SET(PLATFORM_DEPENDENCIES ${LIB_MATH})
  SET (MARIADB_ODBC_SOURCES ${MARIADB_ODBC_SOURCES} ma_platform_posix.c)
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

IF(APPLE)
  # Looking for iconv files
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/FindIconv.cmake)
  IF(ICONV_FOUND)
    INCLUDE_DIRECTORIES(${ICONV_INCLUDE_DIR})
    SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${ICONV_LIBRARIES})
  ELSE()
    MESSAGE(FATAL_ERROR "iconv was not found")
  ENDIF()
ENDIF()

SET(CPACK_PACKAGE_VERSION ${MARIADB_ODBC_VERSION_MAJOR}.${MARIADB_ODBC_VERSION_MINOR}.${MARIADB_ODBC_VERSION_PATCH})

SET(CPACK_COMPONENTS_ALL ClientPlugins ODBCLibs Documentation)
#SET(CPACK_COMPONENTS_ALL ODBCLibs Documentation)
SET(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)

# TODO: Make it optional
# Disable dbug information for release builds 
#SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DDBUG_OFF")
#SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DDBUG_OFF")
#SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -DDBUG_OFF")
#SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DDBUG_OFF")


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
                                     INSTALL_RPATH "${MAODBC_INSTALL_RPATH}")
  ELSE()
    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_SOURCE_DIR}/maodbc.def")
  ENDIF()
ENDIF()

SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LANGUAGE C)
MESSAGE(STATUS "All linked targets/external dependencies: ${MARIADB_CLIENT_TARGET_NAME} ${ODBC_INSTLIBS} ${PLATFORM_DEPENDENCIES}")
TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${MARIADB_CLIENT_TARGET_NAME} ${ODBC_INSTLIBS} ${PLATFORM_DEPENDENCIES})

# Currently on Windows only
IF(WIN32)
  ADD_CUSTOM_COMMAND(TARGET maodbc POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_DIR=$<TARGET_FILE_DIR:maodbc>
                                  -DPLUGINS_LIB_DIR=$<TARGET_FILE_DIR:dialog>
                                  -DINSTALLER_TOOLS_DIR=$<TARGET_FILE_DIR:change_dsns_driver>
                                  -DPLUGINS_SUBDIR_NAME=${MARIADB_DEFAULT_PLUGINS_SUBDIR}
                                  -DFILE_IN=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml.in
                                  -DFILE_OUT=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml
                                  -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                     )
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

INCLUDE("${CMAKE_SOURCE_DIR}/cmake/install.cmake")

SET(CPACK_PACKAGE_NAME "mariadb-connector-odbc")
IF(WIN32)
  ADD_SUBDIRECTORY(wininstall)
  ADD_EXECUTABLE(install_driver osxinstall/install_driver.c)
  TARGET_LINK_LIBRARIES(install_driver ${PLATFORM_DEPENDENCIES} ${ODBC_INSTLIBS})
ELSE()
  IF(APPLE)
    MESSAGE(STATUS "Configuring to generate PKG package")
    ADD_SUBDIRECTORY(osxinstall)
  ENDIF()
  INSTALL(TARGETS
          ${LIBRARY_NAME}
          LIBRARY DESTINATION ${INSTALL_LIBDIR}
          COMPONENT ODBCLibs)

  #IF(NOT DEFINED INSTALL_DOC_DIR)
  #  SET(INSTALL_DOC_DIR "${CMAKE_INSTALL_PREFIX}/share/doc/${CPACK_PACKAGE_NAME}/" CACHE STRING "Directory, where to install documentation(README)")
  #ENDIF()

  #IF(NOT DEFINED INSTALL_LICENSE_DIR)
  #  SET(INSTALL_LICENSE_DIR "${CMAKE_INSTALL_PREFIX}/share/doc/${CPACK_PACKAGE_NAME}/" CACHE STRING "Directory, where to install license file")   
  #ENDIF()
  MESSAGE(STATUS "Documentation and License file are installed to ${INSTALL_DOCDIR}")
  #  MESSAGE(STATUS "License file installed to ${INSTALL_LICENSE_DIR}")

  INSTALL(FILES
          ${CMAKE_SOURCE_DIR}/README
          DESTINATION
          "${INSTALL_DOCDIR}"
          COMPONENT Documentation)
  INSTALL(FILES
          ${CMAKE_SOURCE_DIR}/COPYING
          DESTINATION
          "${INSTALL_DOCDIR}"
          COMPONENT Documentation)
ENDIF()

# Tests. Checking if we have them. May be not the case if we are building from source package
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  IF(WITH_UNIT_TESTS)
    ADD_SUBDIRECTORY(test)
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

      ADD_CUSTOM_COMMAND(TARGET maodbc POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_LOCATION=$<TARGET_FILE:maodbc>
                                      -DTEST_DRIVER=${TEST_DRIVER}
                                      -DFILE_IN=${CMAKE_SOURCE_DIR}/test/odbcinst.ini.in
                                      -DFILE_OUT=${CMAKE_BINARY_DIR}/test/odbcinst.ini
                                      -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                         )

      ADD_CUSTOM_COMMAND(TARGET maodbc POST_BUILD
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

# Packaging
SET(CPACK_PACKAGE_VENDOR "MariaDB Corporation Ab")
SET(CPACK_PACKAGE_DESCRIPTION "MariaDB Connector/ODBC. ODBC driver library for connecting to MariaDB and MySQL servers")
IF(NOT SYSTEM_NAME)
  STRING(TOLOWER ${CMAKE_SYSTEM_NAME} system_name)
ENDIF()
IF(PACKAGE_PLATFORM_SUFFIX)
  SET(CPACK_PACKAGE_FILE_NAME "mariadb-connector-odbc-${CPACK_PACKAGE_VERSION}-${PACKAGE_PLATFORM_SUFFIX}")
ELSE()
 SET(CPACK_PACKAGE_FILE_NAME "mariadb-connector-odbc-${CPACK_PACKAGE_VERSION}-${MARIADB_ODBC_VERSION_QUALITY}-${system_name}-${CMAKE_SYSTEM_PROCESSOR}")
ENDIF()
MESSAGE(STATUS "Package Name: ${CPACK_PACKAGE_FILE_NAME}")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "mariadb-connector-odbc-${CPACK_PACKAGE_VERSION}-${MARIADB_ODBC_VERSION_QUALITY}-src")
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


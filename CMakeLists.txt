# ************************************************************************************
#   Copyright (C) 2013,2025 MariaDB Corporation plc
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

CMAKE_MINIMUM_REQUIRED(VERSION 3.12.0)

IF(NOT ${CMAKE_VERSION} VERSION_LESS "3.20.0")
  CMAKE_POLICY(SET CMP0115 NEW)
ENDIF()
CMAKE_POLICY(SET CMP0048 NEW)
CMAKE_POLICY(SET CMP0040 NEW)
CMAKE_POLICY(SET CMP0057 NEW)
CMAKE_POLICY(SET CMP0054 NEW)

PROJECT(mariadb_connector_odbc 
        VERSION 3.2.8
        LANGUAGES CXX C)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

GET_PROPERTY(MAODBC_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
# We don't need RC for what we need MAODBC_LANGUAGES for
LIST(REMOVE_ITEM MAODBC_LANGUAGES "RC")

SET(MARIADB_ODBC_VERSION_QUALITY "ga")
SET(MARIADB_ODBC_VERSION "03.02.0008")
IF(CMAKE_VERSION VERSION_LESS "3.1")
  IF(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    SET (CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-std=gnu99 ${CMAKE_C_FLAGS}")
  ENDIF()
ELSE()
  SET(CMAKE_C_STANDARD 99)
ENDIF()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

SET(MARIADB_DEFAULT_PLUGINS_SUBDIR "plugin")

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

INCLUDE(check_compiler_flag)

IF(WITH_ASAN)
  IF(MSVC)
    MA_SET_COMPILER_FLAG("-fsanitize=address" DEBUG RELWITHDEBINFO)
    SET(MAODBC_LINKER_FLAGS ${MAODBC_LINKER_FLAGS} /INCREMENTAL:NO)
    MA_SET_LINKER_FLAG("${MAODBC_LINKER_FLAGS}" DEBUG RELWITHDEBINFO)
  ELSE()
    MY_CHECK_AND_SET_COMPILER_FLAG("-U_FORTIFY_SOURCE" DEBUG RELWITHDEBINFO)
    MY_CHECK_AND_SET_COMPILER_FLAG("-fsanitize=address -fPIC" DEBUG RELWITHDEBINFO)

    SET(WITH_ASAN_OK 1)
    FOREACH(lang ${MAODBC_LANGUAGES})
      IF(NOT ${have_${lang}__fsanitize_address__fPIC})
        SET(WITH_ASAN_OK 0)
      ENDIF()
    ENDFOREACH()
    IF(WITH_ASAN_OK)
      OPTION(WITH_ASAN_SCOPE "Enable -fsanitize-address-use-after-scope" OFF)
      IF(WITH_ASAN_SCOPE)
        MY_CHECK_AND_SET_COMPILER_FLAG("-fsanitize=address -fsanitize-address-use-after-scope" DEBUG RELWITHDEBINFO)
      ENDIF()
    ELSE()
      MESSAGE(FATAL_ERROR "Do not know how to enable address sanitizer")
    ENDIF()
  ENDIF()
ENDIF()

IF (WITH_UBSAN)
  MY_CHECK_AND_SET_COMPILER_FLAG("-fsanitize=undefined -fno-sanitize=alignment -U_FORTIFY_SOURCE -DWITH_UBSAN" DEBUG RELWITHDEBINFO)
ENDIF()

IF (WITH_MSAN)
  MY_CHECK_AND_SET_COMPILER_FLAG("-fsanitize=memory -fsanitize-memory-track-origins -U_FORTIFY_SOURCE" DEBUG RELWITHDEBINFO)
ENDIF()

# This has to be before C/C's cmake run, or it will build with /MD
IF(WIN32)
  IF (MSVC)
    SET(CONFIG_TYPES "DEBUG" "RELEASE" "RELWITHDEBINFO" "MINSIZEREL")
    FOREACH(BUILD_TYPE ${CONFIG_TYPES})
      FOREACH(COMPILER ${MAODBC_LANGUAGES})
        SET(COMPILER_FLAGS "${CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE}}")
        IF (NOT COMPILER_FLAGS STREQUAL "")
          IF(NOT WITH_ASAN)
            STRING(REPLACE "/MD" "/MT" COMPILER_FLAGS ${COMPILER_FLAGS})
            IF (BUILD_TYPE STREQUAL "DEBUG")#OR BUILD_TYPE STREQUAL "RELWITHDEBINFO")
              SET(COMPILER_FLAGS "${COMPILER_FLAGS} /RTC1") # STL does not like /RTCc
            ENDIF()
            STRING(REPLACE "/Zi" "/ZI" COMPILER_FLAGS ${COMPILER_FLAGS})
          ENDIF(NOT WITH_ASAN)
          MESSAGE (STATUS "CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE}= ${COMPILER_FLAGS}")
          SET(CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE} ${COMPILER_FLAGS} CACHE
               STRING "Overwritten by mariadb-odbc" FORCE)
        ENDIF()
      ENDFOREACH()
    ENDFOREACH()
  ENDIF()
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)# -DWIN32_LEAN_AND_MEAN)
  SET(INSTALL_PLUGINDIR "${MARIADB_DEFAULT_PLUGINS_SUBDIR}")
ENDIF()

INCLUDE(SearchLibrary)
INCLUDE(SetValueMacro)

### Build options, initial settings and platform defaults
INCLUDE("options_defaults")

### Setting installation paths - should go before C/C subproject sets its own. We need to have control over those
INCLUDE("install")

IF(WIN32 OR WITH_OPENSSL OR "${WITH_SSL}" STREQUAL  "OPENSSL")
  # If C/C is linked dynamically, we don't need link C/ODBC against encryption library
  IF (NOT MARIADB_LINK_DYNAMIC)
    IF(WITH_OPENSSL OR "${WITH_SSL}" STREQUAL  "OPENSSL")
      FIND_PACKAGE(OpenSSL)

      IF(OPENSSL_FOUND)
        MESSAGE(STATUS "Configuring to build with OpenSSL ${OPENSSL_LIBRARIES}")

        ADD_DEFINITIONS(-DHAVE_OPENSSL)
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
  INCLUDE(connector_c)

  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/driver ${CMAKE_SOURCE_DIR}/libmariadb/include)
  INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/driver ${CMAKE_BINARY_DIR}/libmariadb/include)
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
            DESTINATION "${INSTALL_LIBDIR}"
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
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "/usr/include/mariadb")
  INCLUDE_DIRECTORIES("/usr/local/include/mariadb")
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

#Debug log is controlled by connection option solely
ADD_DEFINITIONS(-DMAODBC_DEBUG)

IF(WIN32)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/dsn)
  SET(ODBC_LIBS odbc32)
  SET(ODBC_INSTLIBS odbccp32)
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
ENDIF()

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
ENDIF()

IF(NOT WIN32)
  # Looking for DM(UnixODBC) files
  INCLUDE(FindDM)

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
    INCLUDE(FindIconv)
    IF(ICONV_FOUND)
      INCLUDE_DIRECTORIES(${ICONV_INCLUDE_DIR})
      SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${ICONV_LIBRARIES})
    ELSE()
      MESSAGE(FATAL_ERROR "iconv was not found")
    ENDIF()
  ENDIF()
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

# By now we have everything needed by tests. If we need to build them only - firing config now and exit
# There is "normal" tests config below
IF(BUILD_TESTS_ONLY)
  ADD_SUBDIRECTORY(test)
  IF(NOT WIN32)
    # Configuring ini files for testing with UnixODBC
    MESSAGE(STATUS "Configurig Test Driver: ${TEST_DRIVER}, Test DSN: ${TEST_DSN}, tcp://${TEST_UID}@${TEST_SERVER}:${TEST_PORT}/${TEST_SCHEMA} socket: ${TEST_SOCKET}")

    # Should it really libmaodbc_prefix be here?
    SET(DRIVER_LIB_LOCATION "${libmaodbc_prefix}/${INSTALL_LIBDIR}")
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/test/odbcinst.ini.in ${CMAKE_BINARY_DIR}/test/odbcinst.ini)

    SET(TEST_DRIVER "${DRIVER_LIB_LOCATION}")
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/test/odbc.ini.in ${CMAKE_BINARY_DIR}/test/odbc.ini)
  ENDIF()

  RETURN()
ENDIF()

# We need to determine the last parameter for SQLColAttribute:
# Older UnixODBC version expect SQLPOINTER while Windows expects SQLLEN *
# Windows DM uses SQLPOINTER form 32b, but SQLLEN* for 64b, which is... nice :-\
TRY_COMPILE(COMPILE_OK ${CMAKE_BINARY_DIR} SOURCES ${CMAKE_SOURCE_DIR}/cmake/sqlcolattribute.c OUTPUT_VARIABLE TRYCOMPILEOUTPUT)

IF (WIN32)
  STRING(FIND "${TRYCOMPILEOUTPUT}" "warning C4028: formal parameter 7 different from declaration" WARNINGFOUND)
  IF (NOT WARNINGFOUND EQUAL -1)
    SET(COMPILE_OK FALSE)
  ENDIF()
ENDIF()
MESSAGE(STATUS "Checking if SQLColAttribute expects SQLPOINTER ${COMPILE_OK}")
#MESSAGE(STATUS "${TRYCOMPILEOUTPUT} ${WARNINGFOUND}")
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
ADD_SUBDIRECTORY(driver)
ADD_SUBDIRECTORY(include)

####### MAODBCS #######
ADD_SUBDIRECTORY(dsn)

ADD_SUBDIRECTORY("dsn_test")

# "set/append array" - append a set of strings, separated by a space
MACRO(SETA var)
  FOREACH(v ${ARGN})
    SET(${var} "${${var}} ${v}")
  ENDFOREACH()
ENDMACRO(SETA)

IF(WIN32)
  IF(WITH_MSI)
    ADD_SUBDIRECTORY(packaging/windows)
  ELSE()
    MESSAGE(STATUS "MSI package won't be generated - WITH_MSI=${WITH_MSI}")
  ENDIF()
  ADD_EXECUTABLE(install_driver packaging/macos/install_driver.c)
  TARGET_LINK_LIBRARIES(install_driver ${PLATFORM_DEPENDENCIES} ${ODBC_INSTLIBS})
ELSE()
  IF(APPLE)
    MESSAGE(STATUS "Configuring to generate PKG package")
    ADD_SUBDIRECTORY(packaging/macos)
  ELSE()
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/libmaodbc.pc.in
                   ${CMAKE_CURRENT_BINARY_DIR}/libmaodbc.pc @ONLY)
    # RPM adds dependency on pkg-config, that we do not want. With later versions of cmake looks like we can deal with that            
    MESSAGE(STATUS "Configuring to install libmaodbc.pc")
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libmaodbc.pc
            DESTINATION "${INSTALL_PCDIR}"
            COMPONENT Development)
    # Not doing this with MacOS(iOdbc). So far, at least
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/packaging/linux/maodbc.ini.in
                   ${CMAKE_BINARY_DIR}/packaging/linux/maodbc.ini @ONLY)
    INSTALL(FILES
            ${CMAKE_BINARY_DIR}/packaging/linux/maodbc.ini
            DESTINATION
            "${INSTALL_DOCDIR}"
            COMPONENT Documentation)
    IF(RPM OR DEB)
      CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/packaging/linux/postinstall.in
                     ${CMAKE_BINARY_DIR}/packaging/linux/postinstall @ONLY)
    ENDIF()
  ENDIF()

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

# Tests. Checking if we have them. May be not the case if we are building from source package
IF(EXISTS "${CMAKE_SOURCE_DIR}/test/CMakeLists.txt")
  IF(WITH_UNIT_TESTS)
    ADD_SUBDIRECTORY(test)
    #!!! Creation of ini files for testing is configured in the driver's cmake, as that is custom command for that target
  ENDIF()
ENDIF()

# Packaging
INCLUDE(packaging)
MESSAGE(STATUS "License File: ${CPACK_RESOURCE_FILE_LICENSE}")
MESSAGE(STATUS "ReadMe File: ${CPACK_PACKAGE_DESCRIPTION_FILE}")
MESSAGE(STATUS "Source Package Filename: ${CPACK_SOURCE_PACKAGE_FILE_NAME}.${CPACK_SOURCE_GENERATOR}")


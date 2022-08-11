#
#  Copyright (C) 2021 MariaDB Corporation AB
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the COPYING-CMAKE-SCRIPTS file.
#

OPTION(BUILD_INTERACTIVE_TESTS "Build test(s) requiring user interaction" OFF)
OPTION(USE_INTERACTIVE_TESTS "Run interactive test(s) with ctest" OFF)

IF(WIN32)
  OPTION(WITH_MSI "Build MSI installation package" ON)
  OPTION(WITH_SIGNCODE "Digitally sign files" OFF)

  OPTION(MARIADB_LINK_DYNAMIC "Link Connector/C library dynamically" OFF)
  OPTION(ALL_PLUGINS_STATIC "Compile all plugins in, i.e. make them static" ON)
ELSE()
  OPTION(WITH_MSI "Build MSI installation package" OFF)
  IF(APPLE)
    OPTION(MARIADB_LINK_DYNAMIC "Link Connector/C library dynamically" OFF)
  ELSE()
    OPTION(MARIADB_LINK_DYNAMIC "Link Connector/C library dynamically" ON)
  ENDIF()
  SET(BUILD_INTERACTIVE_TESTS OFF)
  SET(USE_INTERACTIVE_TESTS OFF)
  OPTION(ALL_PLUGINS_STATIC "Compile all plugins in, i.e. make them static" OFF)
ENDIF()

IF(USE_INTERACTIVE_TESTS)
  SET(BUILD_INTERACTIVE_TESTS ON)
ENDIF()

OPTION(WITH_SSL "Enables use of TLS/SSL library" ON)
OPTION(USE_SYSTEM_INSTALLED_LIB "Use installed in the syctem C/C library and do not build one" OFF)
OPTION(DIRECT_LINK_TESTS "Link tests directly against driver library(bypass DM)" OFF)
# This is to be used for some testing scenarious, obviously. e.g. testing of the connector installation. 
OPTION(BUILD_TESTS_ONLY "Build only tests and nothing else" OFF)
OPTION(WITH_UNIT_TESTS "Build unit tests" ON)

IF(BUILD_TESTS_ONLY)
  SET(WITH_UNIT_TESTS ON)
ENDIF()
IF(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  SET(WITH_UNIT_TESTS OFF)
ENDIF()

IF(NOT EXISTS ${CMAKE_SOURCE_DIR}/libmariadb)
  SET(USE_SYSTEM_INSTALLED_LIB ON)
ENDIF()

IF(APPLE)
  SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
  OPTION(WITH_IODBC "Build with iOdbc" ON)
  CMAKE_POLICY(SET CMP0042 NEW)
  CMAKE_POLICY(SET CMP0068 NEW)
  SET(CMAKE_INSTALL_RPATH "")
  SET(CMAKE_INSTALL_NAME_DIR "")
  SET(CMAKE_MACOSX_RPATH ON)
ELSE()
  OPTION(WITH_IODBC "Build with iOdbc" OFF)
ENDIF()

IF(WIN32) 
  # Currently limiting this feature to Windows only, where it's most probably going to be only used
  IF(ALL_PLUGINS_STATIC)
    SET(CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT "STATIC")
    SET(CLIENT_PLUGIN_DIALOG "STATIC")
    SET(CLIENT_PLUGIN_CLIENT_ED25519 "STATIC")
    SET(CLIENT_PLUGIN_CACHING_SHA2_PASSWORD "STATIC")
    SET(CLIENT_PLUGIN_SHA256_PASSWORD "STATIC")
    SET(CLIENT_PLUGIN_MYSQL_CLEAR_PASSWORD "STATIC")
    SET(CLIENT_PLUGIN_MYSQL_OLD_PASSWORD "STATIC")
    SET(MARIADB_LINK_DYNAMIC OFF)
  ENDIF()
ELSE()
  # Defaults for creating odbc(inst).ini for tests with Unix/iOdbc
  IF(WITH_UNIT_TESTS)
    SET_VALUE(TEST_DRIVER "maodbc_test")
    SET_VALUE(TEST_DSN "maodbc_test")
    SET_VALUE(TEST_PORT "3306")
    SET_VALUE(TEST_SERVER "localhost")
    SET_VALUE(TEST_SOCKET "")
    SET_VALUE(TEST_SCHEMA "test")
    SET_VALUE(TEST_UID "root")
    SET_VALUE(TEST_PASSWORD "")
    SET_VALUE(TEST_USETLS "0")
  ENDIF()
ENDIF()

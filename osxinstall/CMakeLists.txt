SET(ODBC_SOURCE_PKG_DIR ${CMAKE_SOURCE_DIR}/osxinstall)

# Get revision number
IF(WITH_REVNO)
  EXECUTE_PROCESS(COMMAND git log HEAD^^..HEAD
                  COMMAND FINDSTR commit
  OUTPUT_VARIABLE revno)
  STRING(REPLACE "commit " "" revno ${revno})
  STRING(REPLACE "\n" "" revno ${revno})
ENDIF()
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(PLATFORM "x86_64")
  SET(PRODUCT_ARCH_BITS "64")
ELSE()
  SET(PLATFORM "32bit")
  SET(PRODUCT_ARCH_BITS "32")
endif()
SET(PRODUCT_NAME "MariaDB ODBC Driver")
SET(PRODUCT_MANUFACTURER "MariaDB")
SET(PRODUCT_VERSION "${MARIADB_ODBC_VERSION_MAJOR}.${MARIADB_ODBC_VERSION_MINOR}.${MARIADB_ODBC_VERSION_PATCH}")
SET(PRODUCT_SERIES "${MARIADB_ODBC_VERSION_MAJOR}.${MARIADB_ODBC_VERSION_MINOR}")
SET(PRODUCT_IDENTIFIER "com.mariadb.connector.odbc")

IF(${revno})
  SET(PKG_PACKAGE "mariadb-connector-odbc-${PRODUCT_VERSION}-r${revno}-osx-${CMAKE_SYSTEM_PROCESSOR}.pkg")
ELSE()
  IF(PACKAGE_PLATFORM_SUFFIX)
    SET(PKG_PACKAGE "mariadb-connector-odbc-${PRODUCT_VERSION}-osx-${CMAKE_SYSTEM_PROCESSOR}-${PACKAGE_PLATFORM_SUFFIX}.pkg")
  ELSE()
    SET(PKG_PACKAGE "mariadb-connector-odbc-${PRODUCT_VERSION}-osx-${CMAKE_SYSTEM_PROCESSOR}.pkg")
  ENDIF()
ENDIF()

MESSAGE(STATUS "PKG package file name ${PKG_PACKAGE}")
IF(WITH_SIGNCODE)
  SET(SIGN_WITH_DEVID "--sign \"${DEVELOPER_ID}\"")
ELSE()
  SET(SIGN_WITH_DEVID "")
ENDIF()

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/WELCOME.html.in
               ${CMAKE_CURRENT_BINARY_DIR}/WELCOME.html @ONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinstall.in
               ${CMAKE_CURRENT_BINARY_DIR}/scripts/postinstall @ONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/distribution.plist.in
               ${CMAKE_CURRENT_BINARY_DIR}/distribution.plist @ONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/build_package.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/build_package.sh @ONLY)

ADD_EXECUTABLE(install_driver install_driver.c)
TARGET_LINK_LIBRARIES(install_driver ${ODBC_INSTLIBS})#${PLATFORM_DEPENDENCIES})
# On Windows should look like
#TARGET_LINK_LIBRARIES(install_driver ${ODBC_INSTLIBS} legacy_stdio_definitions Shlwapi)


IF(USE_SYSTEM_INSTALLED_LIB)
  ADD_CUSTOM_TARGET(copypkgfiles
                  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy_package_files.sh $<TARGET_FILE_DIR:maodbc>
                  DEPENDS maodbc install_driver
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
ELSE()
  SET(GNUTLS_LIB "")
  IF("${WITH_SSL}" STREQUAL "GNUTLS")
    SET(GNUTLS_LIB "${GNUTLS_LIBRARY}")
    MESSAGE(STATUS "Configuring to include gnutls library(${GNUTLS_LIB}) into the package")
  ENDIF()
  ADD_CUSTOM_TARGET(copypkgfiles
                  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy_package_files.sh $<TARGET_FILE_DIR:maodbc> $<TARGET_FILE_DIR:dialog> ${GNUTLS_LIB}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
ENDIF()

ADD_CUSTOM_TARGET(maodbcpkg
                  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/build_package.sh ${PRODUCT_IDENTIFIER} ${PRODUCT_VERSION} ${PKG_PACKAGE}
                  DEPENDS copypkgfiles ${CMAKE_CURRENT_BINARY_DIR}/scripts/postinstall ${CMAKE_CURRENT_BINARY_DIR}/distribution.plist README.html ${CMAKE_CURRENT_BINARY_DIR}/WELCOME.html LICENSE.html
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

SET_TARGET_PROPERTIES(maodbcpkg PROPERTIES EXCLUDE_FROM_ALL OFF)

ADD_DEPENDENCIES(maodbcpkg copypkgfiles)
ADD_DEPENDENCIES(copypkgfiles maodbc)
IF(NOT USE_SYSTEM_INSTALLED_LIB)
  ADD_DEPENDENCIES(copypkgfiles maodbc dialog caching_sha2_password auth_gssapi_client sha256_password mysql_clear_password client_ed25519)
ENDIF()


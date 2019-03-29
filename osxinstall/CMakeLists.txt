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
ELSE()
  SET(PLATFORM "32bit")
endif()
SET(PRODUCT_NAME "MariaDB ODBC Driver")
SET(PRODUCT_MANUFACTURER "MariaDB")
SET(PRODUCT_VERSION "${MARIADB_ODBC_VERSION_MAJOR}.${MARIADB_ODBC_VERSION_MINOR}.${MARIADB_ODBC_VERSION_PATCH}")
SET(PRODUCT_IDENTIFIER "com.mariadb.connector.odbc")

IF(${revno})
  SET(PKG_PACKAGE "mariadb-connector-odbc-${PRODUCT_VERSION}-r${revno}-${PLATFORM}.pkg")
ELSE()
  SET(PKG_PACKAGE "mariadb-connector-odbc-${PRODUCT_VERSION}-${PLATFORM}.pkg")
ENDIF()

MESSAGE(STATUS "PKG package file name ${PKG_PACKAGE}")

ADD_EXECUTABLE(install_driver install_driver.c)
TARGET_LINK_LIBRARIES(install_driver ${ODBC_INSTLIBS})#${PLATFORM_DEPENDENCIES})
# On Windows should look like
#TARGET_LINK_LIBRARIES(install_driver ${ODBC_INSTLIBS} legacy_stdio_definitions Shlwapi)


IF(USE_SYSTEM_INSTALLED_LIB)
  ADD_CUSTOM_TARGET(copypkgfiles
                  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy_package_files.sh $<TARGET_FILE_DIR:maodbc>
                  DEPENDS maodbc install_driver
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
ELSE()
  ADD_CUSTOM_TARGET(copypkgfiles
                  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy_package_files.sh $<TARGET_FILE_DIR:maodbc> $<TARGET_FILE_DIR:dialog>
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
ENDIF()

ADD_CUSTOM_TARGET(maodbcpkg
                  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build_package.sh ${PRODUCT_IDENTIFIER} ${PRODUCT_VERSION} ${PKG_PACKAGE}
                  DEPENDS copypkgfiles ${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinstall distribution.plist README.html WELCOME.html LICENSE.html
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

SET_TARGET_PROPERTIES(maodbcpkg PROPERTIES EXCLUDE_FROM_ALL OFF)

ADD_DEPENDENCIES(maodbcpkg copypkgfiles)
ADD_DEPENDENCIES(copypkgfiles maodbc)
IF(NOT USE_SYSTEM_INSTALLED_LIB)
  ADD_DEPENDENCIES(copypkgfiles maodbc dialog caching_sha2_password auth_gssapi_client sha256_password mysql_clear_password)
ENDIF()


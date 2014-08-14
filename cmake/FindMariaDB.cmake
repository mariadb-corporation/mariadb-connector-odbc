#
# FindMariaDB.cmake
# Note: Windows only
#
# Try to find the include directory

IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(PFILES $ENV{ProgramW6432})   
ELSE()
  SET(PFILES $ENV{ProgramFiles})   
ENDIF()


FIND_PATH(MARIADB_INCLUDE_DIR mysql.h
    $ENV{MARIADB_INCLUDE_DIR}
    $ENV{MARIADB_DIR}/include
    $ENV{MARIADB_DIR}/include/mariadb
    ${PFILES}/MariaDB/*/include)

IF(MARIADB_INCLUDE_DIR)
  MESSAGE(STATUS "Found MariaDB includes: ${MARIADB_INCLUDE_DIR}")
ENDIF()

IF(WIN32)
  SET (LIB_NAME mariadbclient.lib)
ELSE()
  SET (LIB_NAME libmariadbclient.a)
ENDIF()


# Try to find mariadb client libraries
FIND_PATH(MARIADB_LIBRARY_DIR ${LIB_NAME}
    $ENV{MARIADB_LIBRARY}
    ${PFILES}/MariaDB/*/lib
    $ENV{MARIADB_DIR}/lib/mariadb
    $ENV{MARIADB_DIR}/lib
    $ENV{MARIADB_DIR}/libmariadb)

IF(MARIADB_LIBRARY)
  GET_FILENAME_COMPONENT(MARIADB_LIBRARY_DIR ${MARIADB_LIBRARY} PATH)
ENDIF()

IF(MARIADB_LIBRARY_DIR AND MARIADB_INCLUDE_DIR)
  MESSAGE(STATUS "Found MariaDB libraries: ${MARIADB_LIBRARY_DIR}")
  SET(MARIADB_FOUND TRUE)
ENDIF()

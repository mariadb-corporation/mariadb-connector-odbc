#
# FindMariaDB.cmake
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

ELSE()
  SET(MARIADB_LIB libmariadbclient.a)
  FIND_PATH(MARIADB_BIN_DIR mariadb_config
            $ENV{MARIADB_DIR}/bin
            ${MARIADB_DIR}/bin)
  IF(MARIADB_BIN_DIR)
    EXEC_PROGRAM(${MARIADB_BIN_DIR}/mariadb_config 
                 ARGS "--include"
                 OUTPUT_VARIABLE MARIADB_INCLUDE_DIR)
    EXEC_PROGRAM(${MARIADB_BIN_DIR}/mariadb_config 
                 ARGS "--libs"
                 OUTPUT_VARIABLE MARIADB_LIBRARY_DIR)
    # since we use the static library we need the directory part only
    STRING(SUBSTRING ${MARIADB_LIBRARY_DIR} 2 -1 MARIADB_LIBRARY_DIR)
    STRING(FIND ${MARIADB_LIBRARY_DIR} " -l" MY_LENGTH)
    STRING(SUBSTRING ${MARIADB_LIBRARY_DIR} 0 ${MY_LENGTH} MARIADB_LIBRARY_DIR)
    ADD_DEFINITIONS(${MARIADB_INCLUDE_DIR})
  ENDIF()
ENDIF()

IF(MARIADB_LIBRARY_DIR AND MARIADB_INCLUDE_DIR)
  MESSAGE(STATUS "Found MariaDB libraries: ${MARIADB_LIBRARY_DIR}")
  SET(MARIADB_FOUND TRUE)
ENDIF()

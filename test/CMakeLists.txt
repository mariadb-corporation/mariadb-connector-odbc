IF (DIRECT_LINK_TESTS)
  MESSAGE(STATUS "Configuring tests for direct linking against the connector")
ELSE()
  MESSAGE(STATUS "Configuring tests for linking against the DM library")
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ENABLE_TESTING()

SET (ODBC_TESTS
     "basic" "types" "blob" "desc" "info" "tran" "catalog"
     "use_result" "scroll" "bulk" "prepare" "datetime" "keys"
     "curext" "relative" "unicode" "cursor" "dyn_cursor"
     "error" "param" "result1" "result2" "multistatement")

IF (BUILD_INTERACTIVE_TESTS OR USE_INTERACTIVE_TESTS)
  SET (ODBC_TESTS ${ODBC_TESTS} "interactive")
ENDIF()

FOREACH (ODBC_TEST ${ODBC_TESTS})
  ADD_EXECUTABLE(${ODBC_TEST} ${ODBC_TEST}.c tap.h)
  IF (DIRECT_LINK_TESTS)
    TARGET_LINK_LIBRARIES(${ODBC_TEST} maodbc ${MARIADB_LIB} ${PLATFORM_DEPENDENCIES})
  ELSE()
    TARGET_LINK_LIBRARIES(${ODBC_TEST} ${ODBC_LIBS} ${MARIADB_LIB} ${PLATFORM_DEPENDENCIES})
  ENDIF()
  IF (NOT ${ODBC_TEST} STREQUAL "interactive" OR USE_INTERACTIVE_TESTS)
    ADD_TEST(${ODBC_TEST} ${EXECUTABLE_OUTPUT_PATH}/${ODBC_TEST})
  ENDIF()
  SET_TESTS_PROPERTIES(${ODBC_TEST} PROPERTIES TIMEOUT 120)
ENDFOREACH()  


ADD_EXECUTABLE(connstring connstring.c ${CMAKE_SOURCE_DIR}/ma_dsn.c tap.h)
TARGET_LINK_LIBRARIES(connstring ${ODBC_LIBS} ${ODBC_INSTLIBS} ${MARIADB_LIB} ${PLATFORM_DEPENDENCIES})
ADD_TEST(connstring ${EXECUTABLE_OUTPUT_PATH}/connstring)
SET_TESTS_PROPERTIES(connstring PROPERTIES TIMEOUT 120)

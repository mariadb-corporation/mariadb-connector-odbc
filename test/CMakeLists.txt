IF (DIRECT_LINK_TESTS)
  MESSAGE(STATUS "Configuring tests for direct linking against the connector")
ELSE()
  MESSAGE(STATUS "Configuring tests for linking against the DM library")
ENDIF()

# Seems like libodbc needs it
IF(NOT WIN32)
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/FindIconv.cmake)
  IF (ICONV_EXTERNAL)
    SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${ICONV_LIBRARIES})
  ENDIF()
  SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} "pthread" "dl")
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ENABLE_TESTING()

SET (ODBC_TESTS
     "basic" "types" "blob" "desc" "info" "tran" "catalog1" "catalog2"
     "use_result" "scroll" "bulk" "prepare" "datetime" "keys"
     "curext" "relative" "unicode" "cursor" "dyn_cursor"
     "error" "param" "result1" "result2" "multistatement")

# Interactive makes sense on WIN32 only atm
IF (WIN32 AND (BUILD_INTERACTIVE_TESTS OR USE_INTERACTIVE_TESTS))
  MESSAGE(STATUS "Configuring to build interactive test")
  SET (ODBC_TESTS ${ODBC_TESTS} "interactive")
ENDIF()
# iOdbc has 
IF(WITH_IODBC)
  SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wno-pointer-sign")
ENDIF()
FOREACH (ODBC_TEST ${ODBC_TESTS})
  IF (${ODBC_TEST} STREQUAL "interactive")
    ADD_EXECUTABLE(odbc_${ODBC_TEST} ${ODBC_TEST}.c tap.h)
  ELSE()
    ADD_EXECUTABLE(odbc_${ODBC_TEST} ${ODBC_TEST}.c tap.h)
  ENDIF()
  IF (DIRECT_LINK_TESTS)
    TARGET_LINK_LIBRARIES(odbc_${ODBC_TEST} maodbc mariadbclient ${PLATFORM_DEPENDENCIES})
  ELSE()
    TARGET_LINK_LIBRARIES(odbc_${ODBC_TEST} ${ODBC_LIBS} mariadbclient ${PLATFORM_DEPENDENCIES})
  ENDIF()
  IF (NOT ${ODBC_TEST} STREQUAL "interactive" OR USE_INTERACTIVE_TESTS)
    ADD_TEST(odbc_${ODBC_TEST} ${EXECUTABLE_OUTPUT_PATH}/odbc_${ODBC_TEST})
    SET_TESTS_PROPERTIES(odbc_${ODBC_TEST} PROPERTIES TIMEOUT 120)
  ENDIF()
ENDFOREACH()

ADD_EXECUTABLE(odbc_connstring connstring.c ${CMAKE_SOURCE_DIR}/ma_dsn.c tap.h)
TARGET_LINK_LIBRARIES(odbc_connstring ${ODBC_LIBS} ${ODBC_INSTLIBS} mariadbclient ${PLATFORM_DEPENDENCIES})
ADD_TEST(odbc_connstring ${EXECUTABLE_OUTPUT_PATH}/odbc_connstring)
SET_TESTS_PROPERTIES(odbc_connstring PROPERTIES TIMEOUT 120)

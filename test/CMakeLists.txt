IF (DIRECT_LINK_TESTS)
  MESSAGE(STATUS "Configuring tests for direct linking against the connector")
ELSE()
  MESSAGE(STATUS "Configuring tests for linking against the DM library")
ENDIF()

IF(NOT WIN32)
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/FindIconv.cmake)
  IF (ICONV_EXTERNAL)
    SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} ${ICONV_LIBRARIES})
  ENDIF()
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ENABLE_TESTING()

SET (ODBC_TESTS
     "basic" "types" "blob" "desc" "info" "tran" "catalog"
     "use_result" "scroll" "bulk" "prepare" "datetime" "keys"
     "curext" "relative" "unicode" "cursor" "dyn_cursor"
     "error" "param" "result1" "result2" "multistatement")

FOREACH (ODBC_TEST ${ODBC_TESTS})
  ADD_EXECUTABLE(${ODBC_TEST} ${ODBC_TEST}.c tap.h)
  IF (DIRECT_LINK_TESTS)
    TARGET_LINK_LIBRARIES(${ODBC_TEST} maodbc ${PLATFORM_DEPENDENCIES})
  ELSE()
    TARGET_LINK_LIBRARIES(${ODBC_TEST} ${ODBC_LIBS} ${PLATFORM_DEPENDENCIES})
  ENDIF()
  ADD_TEST(${ODBC_TEST} ${EXECUTABLE_OUTPUT_PATH}/${ODBC_TEST})
  SET_TESTS_PROPERTIES(${ODBC_TEST} PROPERTIES TIMEOUT 120)
ENDFOREACH()  



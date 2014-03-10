INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ENABLE_TESTING()

SET (ODBC_TESTS
     "basic" "types" "blob" "desc" "info" "tran" "catalog"
     "use_result" "scroll" "bulk" "prepare" "datetime" "keys"
     "curext" "relative" "unicode" "cursor" "dyn_cursor"
     "error" "param" "result1" "result2" "multistatement")

FOREACH (ODBC_TEST ${ODBC_TESTS})
  ADD_EXECUTABLE(${ODBC_TEST} ${ODBC_TEST}.c tap.h)
  TARGET_LINK_LIBRARIES(${ODBC_TEST} ${ODBC_LIBS} ${MARIADB_LIB})
  ADD_TEST(${ODBC_TEST} ${EXECUTABLE_OUTPUT_PATH}/${ODBC_TEST})
  SET_TESTS_PROPERTIES(${ODBC_TEST} PROPERTIES TIMEOUT 120)
ENDFOREACH()  



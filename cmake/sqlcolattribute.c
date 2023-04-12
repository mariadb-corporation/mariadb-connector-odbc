#ifdef _WIN32
# include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

#ifdef __cplusplus
extern "C" {
#endif 


SQLRETURN SQL_API SQLColAttribute(SQLHSTMT  StatementHandle,
                                  SQLUSMALLINT ColumnNumber,
                                  SQLUSMALLINT FieldIdentifier,
                                  SQLPOINTER  CharacterAttributePtr,
                                  SQLSMALLINT BufferLength,
                                  SQLSMALLINT *StringLengthPtr,
//                                  SQLLEN *NumericAttributePtr )
                                  SQLPOINTER NumericAttributePtr )
{
  return SQL_SUCCESS;
}

int main() {
  return 0;
}

#ifdef __cplusplus
}
#endif 

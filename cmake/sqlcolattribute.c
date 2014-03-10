#include <sql.h>
#include <sqlext.h>

SQLRETURN SQL_API SQLColAttribute(SQLHSTMT  StatementHandle,
                                  SQLUSMALLINT ColumnNumber,
                                  SQLUSMALLINT FieldIdentifier,
                                  SQLPOINTER  CharacterAttributePtr,
                                  SQLSMALLINT BufferLength,
                                  SQLSMALLINT *StringLengthPtr,
                                  SQLPOINTER  NumericAttributePtr )
{
  return SQL_SUCCESS;
}

int main() {
  return 0;
}

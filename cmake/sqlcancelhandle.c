#ifdef _WIN32
# include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

#ifdef __cplusplus
extern "C" {
#endif 


SQLRETURN Cancel(HDBC Dbc)
{
  return SQLCancelHandle(SQL_HANDLE_DBC, Dbc);
}

int main() {
  return 0;
}

#ifdef __cplusplus
}
#endif 

/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2016 MariaDB Corporation AB

  The MySQL Connector/ODBC is licensed under the terms of the GPLv2
  <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
  MySQL Connectors. There are special exceptions to the terms and
  conditions of the GPLv2 as it is applied to this software, see the
  FLOSS License Exception
  <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; version 2 of the License.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "tap.h"
#include "ma_dsn.h"

MADB_Dsn   *Dsn;
char        CreatedDSN[4][32];
int         CreatedDsnCount=  0;
const char *DsnName=          "madb_connstring_test";
const char *DsnConnStr=       "DSN=madb_connstring_test";
const char *Description=      "MariaDB C/ODBC test DSN for automatic testing";

/****************************** Helpers ****************************/
#define RESET_DSN(dsn) MADB_DSN_Free(dsn);memset(dsn, 0, sizeof(MADB_Dsn))

BOOL VerifyOptionFields(MADB_Dsn *Dsn)
{
  int i= 0;

  while (DsnKeys[i].DsnKey != NULL)
  {
    if (DsnKeys[i].Type == DSN_TYPE_OPTION && DsnKeys[i].IsAlias == 0)
    {
      if (*(my_bool *)((char *)Dsn + DsnKeys[i].DsnOffset) != (DSN_OPTION(Dsn, DsnKeys[i].FlagValue) != 0 ? 1 : 0))
      {
        diag("Dsn field for %s do not match %d bit in the Options", DsnKeys[i].DsnKey,DsnKeys[i].FlagValue);
        return FALSE;
      }
    }
    ++i;
  }
  return TRUE;
}

#define VERIFY_OPTIONS(dsn, value) is_num(dsn->Options,value);IS(VerifyOptionFields(dsn))

/* Copy of the function from connector - required for (easier) linking */
char *trim(char *Str)
{
  char *end;
  /* I am not sure using iswspace, and not isspace makes any sense here. But probably does not hurt either */
  while (Str && iswspace(Str[0]))
    Str++;
  end= Str + strlen(Str) - 1;
  while (iswspace(*end))
    *end--= 0;
  return Str;
}


int CreateTestDsn(MADB_Dsn *dsn)
{
  if (CreatedDsnCount == sizeof(CreatedDSN)/sizeof(CreatedDSN[0]))
  {
    diag("Max number of test DSN's has to be increased");
    return FAIL;
  }

  if (MADB_SaveDSN(dsn))
  {
    strcpy_s(CreatedDSN[CreatedDsnCount], sizeof(CreatedDSN[0]), dsn->DSNName);
    ++CreatedDsnCount;
    return OK;
  }
  diag("An error occured while saved DSN %s", dsn->DSNName);
  return FAIL;
}


int CleanUp()
{
  int i, ret= OK;

  for (i=0; i < CreatedDsnCount; ++i)
  {
    if (SQLRemoveDSNFromIni(CreatedDSN[i]) == FALSE)
    {
      diag("Failed to remove DSN %s", CreatedDSN[i]);
      ret= FAIL;
    }
    else
    {
      --CreatedDsnCount;
      CreatedDSN[i][0]= 0;
    }
  }

  return OK;
}

/****************************** Tests ********************************/

ODBC_TEST(connstring_test)
{
  char connstr4dsn[512];

  IS(SQLRemoveDSNFromIni(DsnName));
  FAIL_IF(MADB_DSN_Exists(DsnName), "DSN exsists!");

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DESCRIPTION={%s};DRIVER=%s;UID=%s;PWD=%s;SERVER=%s%s;OPTIONS=%u;NO_PROMPT=1", DsnName,
    Description, my_drivername, my_uid, my_pwd, my_servername, ma_strport, MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));

  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT|MADB_OPT_FLAG_NO_PROMPT);
  IS_STR(Dsn->Description, Description, strlen(Description) + 1);

  CreateTestDsn(Dsn);
  IS(MADB_DSN_Exists(DsnName));

  RESET_DSN(Dsn);

  IS(MADB_ReadDSN(Dsn, DsnConnStr, TRUE));

  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT|MADB_OPT_FLAG_NO_PROMPT);
  is_num(Dsn->Port, my_port);
  IS_STR(Dsn->Description, Description, sizeof(Description));
  IS_STR(Dsn->Driver, my_drivername, strlen(my_drivername) + 1);
  FAIL_IF(strncmp(Dsn->UserName, my_uid, strlen(my_uid) + 1), "Uid stored in/read from DSN incorrectly")
  FAIL_IF(strncmp(Dsn->Password, my_pwd, strlen(my_pwd) + 1), "Pwd stored in/read from DSN incorrectly")
  IS_STR(Dsn->ServerName, my_servername, strlen(my_servername) + 1);

  /* Checking that value in the connection string prevails over value in the dsn */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;OPTIONS=%u;DESCRIPTION=%s", DsnConnStr, MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS,
    "Changed description");

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS);
  IS_STR(Dsn->Description, "Changed description", sizeof("Changed description"));

  RESET_DSN(Dsn);

  /* Same as previous, but also that last ent*/
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;OPTIONS=%u;NO_PROMPT=0;AUTO_RECONNECT=1", DsnConnStr,
    MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS|MADB_OPT_FLAG_FORWARD_CURSOR|MADB_OPT_FLAG_NO_CACHE);

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_FOUND_ROWS|MADB_OPT_FLAG_FORWARD_CURSOR|MADB_OPT_FLAG_NO_CACHE|MADB_OPT_FLAG_AUTO_RECONNECT);
  IS_STR(Dsn->Description, Description, sizeof(Description));

  return OK;
}


ODBC_TEST(options_test)
{
  char connstr4dsn[512];
  unsigned int bit= 0;

  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  while(1)
  {
    _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;OPTIONS=%u", DsnConnStr, bit);

    IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));

    VERIFY_OPTIONS(Dsn, bit);

    IS(MADB_SaveDSN(Dsn));
    IS(MADB_DSN_Exists(DsnName));

    RESET_DSN(Dsn);

    IS(MADB_ReadDSN(Dsn, DsnConnStr, TRUE));

    VERIFY_OPTIONS(Dsn, bit);

    if (bit == 0x80000000)
      break;
    bit= bit != 0 ? bit << 1 : 1;
  }

  return OK;
}


unsigned int CharsSum(const char *str)
{
  unsigned int res= 0;
  while (*str)
  {
    res+= *str++;
  }
  return res;
}

ODBC_TEST(all_other_fields_test)
{
  char connstr4dsn[512], *opt_value, IntValue[20], *BoolValue= "1";
  int i= 5; /* After Options. Assuming that fields before Options are tested in other test(s) */

  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  while(DsnKeys[i].DsnKey != NULL)
  {
    if (DsnKeys[i].IsAlias)
    {
      ++i;
      continue;
    }
    opt_value= DsnKeys[i].DsnKey;
    if (DsnKeys[i].Type == DSN_TYPE_BOOL || DsnKeys[i].Type == DSN_TYPE_OPTION)
    {
      opt_value= BoolValue;
    }
    else if (DsnKeys[i].Type == DSN_TYPE_INT)
    {
      _snprintf(IntValue, sizeof(IntValue), "%u", CharsSum(DsnKeys[i].DsnKey));
      opt_value= IntValue;
    }

    _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;%s=%s", DsnConnStr, DsnKeys[i].DsnKey, opt_value);

    IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
    IS(MADB_SaveDSN(Dsn));
    ++i;
  }

  /* This fields were out of the loop */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;DESCRIPTION=DESCRIPTION;DRIVER=%s", DsnConnStr, my_drivername);
  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  IS(MADB_SaveDSN(Dsn));

  /* We saved all posible options. If DsnKeys contains erroneous info, some of values would have got corrupted */
  RESET_DSN(Dsn);
  IS(MADB_ReadDSN(Dsn, DsnConnStr, TRUE));
  /* 2 special fields */
  IS_STR(Dsn->Driver, my_drivername, strlen(my_drivername) + 1);
  IS_STR(Dsn->Description, "DESCRIPTION", sizeof("DESCRIPTION"));
  i= 5;
  while(DsnKeys[i].DsnKey != NULL)
  {
    if (DsnKeys[i].IsAlias)
    {
      ++i;
      continue;
    }

    switch (DsnKeys[i].Type)
    {
    case DSN_TYPE_COMBO:
    case DSN_TYPE_STRING:
      IS_STR(*(char**)((char*)Dsn + DsnKeys[i].DsnOffset), DsnKeys[i].DsnKey, strlen(DsnKeys[i].DsnKey) + 1);
      break;
    case DSN_TYPE_INT:
      is_num(*(unsigned int*)((char*)Dsn + DsnKeys[i].DsnOffset), CharsSum(DsnKeys[i].DsnKey));
      break;
    case DSN_TYPE_BOOL:
    case DSN_TYPE_OPTION:
      /* IsNamedPipe is switched off when TcpIp is switched on(since TcpIp goes after NamedPipe in the DsnKeys.
         Do detect IsNamedPipe, comparing its offset in the MADB_Dsn with offset recorded in the DsnKeys */
      is_num(*(my_bool*)((char*)Dsn + DsnKeys[i].DsnOffset), DsnKeys[i].DsnOffset == (size_t)&((MADB_Dsn*)NULL)->IsNamedPipe ? 0 : 1);
    }

    ++i;
  }

  RESET_DSN(Dsn);

  return OK;
}


ODBC_TEST(aliases_tests)
{
  char connstr4dsn[512];
  unsigned int options= 0xf0f0f0f0;
  unsigned int option= ~options;

  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  RESET_DSN(Dsn);

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;UID=%s;PWD=%s;SERVER=%s%s;DATABASE=%s;OPTIONS=%u", DsnName,
    my_drivername, my_uid, my_pwd, my_servername, ma_strport, my_schema, options);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  IS(MADB_SaveDSN(Dsn));

  RESET_DSN(Dsn);
  IS(MADB_ReadConnString(Dsn, DsnConnStr, SQL_NTS, TRUE));

  FAIL_IF(Dsn->UserName == NULL || strncmp(Dsn->UserName, my_uid, strlen(my_uid) + 1), "Uid stored in/read from DSN incorrectly")
  FAIL_IF(Dsn->Password == NULL || strncmp(Dsn->Password, my_pwd, strlen(my_pwd) + 1), "Pwd stored in/read from DSN incorrectly")

  IS_STR(Dsn->ServerName, my_servername, strlen(my_servername) + 1);
  IS_STR(Dsn->Catalog,    my_schema,     strlen(my_schema) + 1);
  is_num(Dsn->Options,    options);

  /* Now set all values via aliases. In fact we could generate the string automatically, but I guess there is not much need  */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;USER=user;PASSWORD=password;SERVERNAME=servername;DB=randomdbname;OPTION=%u", DsnName, option);

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, TRUE));

  IS_STR(Dsn->UserName,   "user",         sizeof("user"));
  IS_STR(Dsn->Password,   "password",     sizeof("password"));
  IS_STR(Dsn->ServerName, "servername",   sizeof("servername"));
  IS_STR(Dsn->Catalog,    "randomdbname", sizeof("randomdbname"));
  is_num(Dsn->Options,    option);

  return OK;
}


ODBC_TEST(dependent_fields)
{
  char connstr4dsn[512];

  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  RESET_DSN(Dsn);

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;UID=%s;PWD=%s;SERVER=%s%s;DB=%s;OPTIONS=%u;TCPIP=1", DsnName,
    my_drivername, my_uid, my_pwd, my_servername, ma_strport, my_schema, MADB_OPT_FLAG_NAMED_PIPE);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  IS(MADB_SaveDSN(Dsn));

  RESET_DSN(Dsn);
  IS(MADB_ReadConnString(Dsn, DsnConnStr, SQL_NTS, TRUE));

  is_num(Dsn->IsTcpIp,     1);
  is_num(Dsn->IsNamedPipe, 0);
  is_num(Dsn->Options,     0);

  /* Now set all values via aliases. In fact we could generate the string automatically, but I guess there is not much need  */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;NamedPipe=1", DsnName);

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, TRUE));
  is_num(Dsn->IsTcpIp,     0);
  is_num(Dsn->IsNamedPipe, 1);
  is_num(Dsn->Options,     MADB_OPT_FLAG_NAMED_PIPE);

  return OK;
}


/* I render following text from https://msdn.microsoft.com/en-us/library/ms715433%28v=vs.85%29.aspx
   "The driver checks whether the connection string passed to it by the Driver Manager contains the DSN or DRIVER keyword.
   If the connection string contains the DRIVER keyword, the driver cannot retrieve information about the data source from
   the system information. If the connection string contains the DSN keyword or does not contain either the DSN or the DRIVER
   keyword, the driver can retrieve information about the data source from the system information as follows:
    - If the connection string contains the DSN keyword, the driver retrieves the information for the specified data source.
    - If the connection string does not contain the DSN keyword, the specified data source is not found, or the DSN keyword is
    set to "DEFAULT", the driver retrieves the information for the Default data source."
  
    as DRIVER keyword should prevent driver from expanding DSN data. The testcase verifies that. */
ODBC_TEST(driver_vs_dsn)
{
  char connstr4dsn[512];

  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  RESET_DSN(Dsn);

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;UID=%s;PWD=%s;SERVER=%s%s;DB=%s;OPTIONS=%u", DsnName,
    my_drivername, my_uid, my_pwd, my_servername, ma_strport, my_schema, MADB_OPT_FLAG_NAMED_PIPE);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  IS(MADB_SaveDSN(Dsn));

  RESET_DSN(Dsn);
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;SERVER=%s;", DsnName, my_drivername, "some.other.host");

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, TRUE));

  /* Natural in any case*/
  IS_STR(Dsn->ServerName, "some.other.host", sizeof("some.other.host"));
  /* CHeck that we have nothing from the DSN */
  is_num(Dsn->IsTcpIp,     0); /* It should not be set either */
  is_num(Dsn->IsNamedPipe, 0);
  is_num(Dsn->Options,     0);
  FAIL_IF(Dsn->Port != 0 && Dsn->Port == my_port, "Port value from DSN!");
  FAIL_IF(Dsn->UserName!= NULL && strncmp(Dsn->UserName, my_uid, strlen(my_uid) + 1), "Uid value from DSN!");
  FAIL_IF(Dsn->Password!= NULL && strncmp(Dsn->Password, my_pwd, strlen(my_pwd) + 1), "Pwd value from DSN!");
  FAIL_IF(Dsn->Catalog!= NULL && strncmp(Dsn->Catalog, my_schema, strlen(my_schema) + 1), "DB value from DSN!");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {connstring_test,       "connstring_parsing_test", NORMAL},
  {options_test,          "options_test",            NORMAL},
  {all_other_fields_test, "all_other_fields_test",   NORMAL},
  {aliases_tests,         "aliases_tests",           NORMAL},
  {dependent_fields,      "dependent_fields_tests",  NORMAL},
  {driver_vs_dsn,         "driver_vs_dsn",           NORMAL},
  
  {NULL, NULL, 0}
};


int main(int argc, char **argv)
{
  int ret, tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;

  get_options(argc, argv);
  plan(tests);

  Dsn= MADB_DSN_Init();
  Dsn->FreeMe= FALSE; /* Let tests only reset dsn struct, but do not free it */

  ret= run_tests(my_tests);

  Dsn->FreeMe= TRUE;
  MADB_DSN_Free(Dsn);

  if (CleanUp() == FAIL)
  {
    ret= 1;
    diag("Clean-up after tests failed - %d test DSN's stay in the system:", CreatedDsnCount);
  }

  return ret;
}

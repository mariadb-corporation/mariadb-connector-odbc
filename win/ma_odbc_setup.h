/*
 *
 */
#ifndef _ma_odbc_setup_h_
#define _ma_odbc_setup_h_

#define MA_WIN_SET_VALUE(Page, Field, Value) \
 Edit_SetText(GetDlgItem(hwndTab[(Page)],(Field)), (Value));

#define MA_WIN_SET_MAXLEN(Page, Field, Value) \
 Edit_LimitText(GetDlgItem(hwndTab[(Page)],(Field)), (Value));



typedef struct 
{
  MADB_DsnKey *Key;
  int Page;
  int Item;
  int MaxLength;
  my_bool Manadatory;
} MADB_DsnMap;

#endif /* _ma_odbc_setup_h_ */
